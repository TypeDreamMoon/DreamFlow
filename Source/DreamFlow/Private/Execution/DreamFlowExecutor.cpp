#include "Execution/DreamFlowExecutor.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "Engine/Engine.h"
#include "Execution/DreamFlowDebuggerSubsystem.h"

void UDreamFlowExecutor::Initialize(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext)
{
    UnregisterFromDebugger();
    FlowAsset = InFlowAsset;
    ExecutionContext = InExecutionContext;
    CurrentNode = nullptr;
    VisitedNodes.Reset();
    bIsRunning = false;
    bIsPaused = false;
    bBreakOnNextNode = false;
    bHasFinished = false;
    BroadcastDebugStateChanged();
}

bool UDreamFlowExecutor::StartFlow()
{
    if (FlowAsset == nullptr || FlowAsset->GetEntryNode() == nullptr)
    {
        return false;
    }

    CurrentNode = nullptr;
    VisitedNodes.Reset();
    bIsRunning = true;
    bIsPaused = false;
    bBreakOnNextNode = false;
    bHasFinished = false;
    RegisterWithDebugger();
    OnFlowStarted.Broadcast();
    BroadcastDebugStateChanged();

    if (!EnterNode(FlowAsset->GetEntryNode()))
    {
        FinishFlow();
        return false;
    }

    return true;
}

bool UDreamFlowExecutor::RestartFlow()
{
    FinishFlow();
    return StartFlow();
}

void UDreamFlowExecutor::FinishFlow()
{
    if (!bIsRunning)
    {
        CurrentNode = nullptr;
        VisitedNodes.Reset();
        bIsPaused = false;
        bBreakOnNextNode = false;
        return;
    }

    if (CurrentNode != nullptr)
    {
        OnNodeExited.Broadcast(CurrentNode);
    }

    bIsRunning = false;
    bIsPaused = false;
    bBreakOnNextNode = false;
    bHasFinished = true;
    CurrentNode = nullptr;
    BroadcastDebugStateChanged();
    OnFlowFinished.Broadcast();
    UnregisterFromDebugger();
}

bool UDreamFlowExecutor::Advance()
{
    if (!bIsRunning || bIsPaused || CurrentNode == nullptr)
    {
        return false;
    }

    const TArray<UDreamFlowNode*> Children = CurrentNode->GetChildrenCopy();
    if (Children.Num() == 0)
    {
        FinishFlow();
        return false;
    }

    return Children.Num() == 1 ? EnterNode(Children[0]) : false;
}

bool UDreamFlowExecutor::MoveToChildByIndex(int32 ChildIndex)
{
    if (!bIsRunning || bIsPaused || CurrentNode == nullptr)
    {
        return false;
    }

    const TArray<UDreamFlowNode*> Children = CurrentNode->GetChildrenCopy();
    if (!Children.IsValidIndex(ChildIndex))
    {
        return false;
    }

    return EnterNode(Children[ChildIndex]);
}

bool UDreamFlowExecutor::ChooseChild(UDreamFlowNode* ChildNode)
{
    if (!bIsRunning || bIsPaused || CurrentNode == nullptr || ChildNode == nullptr)
    {
        return false;
    }

    const TArray<UDreamFlowNode*> Children = CurrentNode->GetChildrenCopy();
    return Children.Contains(ChildNode) ? EnterNode(ChildNode) : false;
}

bool UDreamFlowExecutor::EnterNode(UDreamFlowNode* Node)
{
    if (!bIsRunning || bIsPaused || FlowAsset == nullptr || Node == nullptr)
    {
        return false;
    }

    if (!FlowAsset->GetNodes().Contains(Node) || !Node->CanEnterNode(ExecutionContext))
    {
        return false;
    }

    if (CurrentNode != nullptr && CurrentNode != Node)
    {
        OnNodeExited.Broadcast(CurrentNode);
    }

    CurrentNode = Node;
    VisitedNodes.AddUnique(Node);
    OnNodeEntered.Broadcast(Node);
    if (ShouldPauseAtNode(Node))
    {
        bIsPaused = true;
        BroadcastDebugStateChanged();
        OnExecutionPaused.Broadcast(Node);
        return true;
    }

    return ExecuteCurrentNode();
}

void UDreamFlowExecutor::SetExecutionContext(UObject* InExecutionContext)
{
    ExecutionContext = InExecutionContext;
}

bool UDreamFlowExecutor::PauseExecution()
{
    if (!bIsRunning || bIsPaused || CurrentNode == nullptr)
    {
        return false;
    }

    bIsPaused = true;
    BroadcastDebugStateChanged();
    OnExecutionPaused.Broadcast(CurrentNode);
    return true;
}

bool UDreamFlowExecutor::ContinueExecution()
{
    if (!bIsRunning || !bIsPaused || CurrentNode == nullptr)
    {
        return false;
    }

    bIsPaused = false;
    BroadcastDebugStateChanged();
    OnExecutionResumed.Broadcast(CurrentNode);
    return ExecuteCurrentNode();
}

bool UDreamFlowExecutor::StepExecution()
{
    if (!bIsRunning)
    {
        return false;
    }

    bBreakOnNextNode = true;
    return bIsPaused ? ContinueExecution() : true;
}

void UDreamFlowExecutor::SetPauseOnBreakpoints(bool bEnabled)
{
    bPauseOnBreakpoints = bEnabled;
    BroadcastDebugStateChanged();
}

UObject* UDreamFlowExecutor::GetExecutionContext() const
{
    return ExecutionContext;
}

UDreamFlowAsset* UDreamFlowExecutor::GetFlowAsset() const
{
    return FlowAsset;
}

UDreamFlowNode* UDreamFlowExecutor::GetCurrentNode() const
{
    return CurrentNode;
}

TArray<UDreamFlowNode*> UDreamFlowExecutor::GetAvailableChildren() const
{
    return CurrentNode ? CurrentNode->GetChildrenCopy() : TArray<UDreamFlowNode*>();
}

TArray<UDreamFlowNode*> UDreamFlowExecutor::GetVisitedNodes() const
{
    TArray<UDreamFlowNode*> Result;
    Result.Reserve(VisitedNodes.Num());

    for (UDreamFlowNode* Node : VisitedNodes)
    {
        Result.Add(Node);
    }

    return Result;
}

bool UDreamFlowExecutor::IsRunning() const
{
    return bIsRunning;
}

bool UDreamFlowExecutor::IsPaused() const
{
    return bIsPaused;
}

bool UDreamFlowExecutor::GetPauseOnBreakpoints() const
{
    return bPauseOnBreakpoints;
}

EDreamFlowExecutorDebugState UDreamFlowExecutor::GetDebugState() const
{
    if (bIsPaused)
    {
        return EDreamFlowExecutorDebugState::Paused;
    }

    if (bIsRunning)
    {
        return EDreamFlowExecutorDebugState::Running;
    }

    return bHasFinished ? EDreamFlowExecutorDebugState::Finished : EDreamFlowExecutorDebugState::Idle;
}

bool UDreamFlowExecutor::ExecuteCurrentNode()
{
    if (!bIsRunning || bIsPaused || CurrentNode == nullptr)
    {
        return false;
    }

    CurrentNode->ExecuteNode(ExecutionContext);
    NotifyDebuggerStateChanged();

    if (CurrentNode->IsTerminalNode() && CurrentNode->GetChildrenCopy().Num() == 0)
    {
        FinishFlow();
    }

    return true;
}

bool UDreamFlowExecutor::ShouldPauseAtNode(const UDreamFlowNode* Node)
{
    if (Node == nullptr)
    {
        return false;
    }

    if (bBreakOnNextNode)
    {
        bBreakOnNextNode = false;
        return true;
    }

    return bPauseOnBreakpoints && FlowAsset != nullptr && FlowAsset->HasBreakpointOnNode(Node->NodeGuid);
}

void UDreamFlowExecutor::BroadcastDebugStateChanged()
{
    OnDebugStateChanged.Broadcast(GetDebugState(), CurrentNode);
    NotifyDebuggerStateChanged();
}

void UDreamFlowExecutor::RegisterWithDebugger()
{
    if (GEngine == nullptr)
    {
        return;
    }

    if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
    {
        DebuggerSubsystem->RegisterExecutor(this);
    }
}

void UDreamFlowExecutor::UnregisterFromDebugger()
{
    if (GEngine == nullptr)
    {
        return;
    }

    if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
    {
        DebuggerSubsystem->UnregisterExecutor(this);
    }
}

void UDreamFlowExecutor::NotifyDebuggerStateChanged()
{
    if (GEngine == nullptr)
    {
        return;
    }

    if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
    {
        DebuggerSubsystem->NotifyExecutorChanged(this);
    }
}
