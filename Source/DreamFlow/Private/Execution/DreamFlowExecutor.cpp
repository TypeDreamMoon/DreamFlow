#include "Execution/DreamFlowExecutor.h"

#include "DreamFlowAsset.h"
#include "DreamFlowLog.h"
#include "DreamFlowNode.h"
#include "Engine/Engine.h"
#include "Execution/DreamFlowAsyncContext.h"
#include "Execution/DreamFlowDebuggerSubsystem.h"

namespace DreamFlowExecutorPrivate
{
    static bool TryGetConvertedValue(const UDreamFlowExecutor* Executor, const FName VariableName, const EDreamFlowValueType TargetType, FDreamFlowValue& OutValue)
    {
        if (Executor == nullptr)
        {
            return false;
        }

        FDreamFlowValue StoredValue;
        if (!Executor->GetVariableValue(VariableName, StoredValue))
        {
            return false;
        }

        return DreamFlowVariable::TryConvertValue(StoredValue, TargetType, OutValue);
    }
}

void UDreamFlowExecutor::Initialize(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext)
{
    ResetRuntimeState(InFlowAsset, InExecutionContext, true);
}

void UDreamFlowExecutor::InitializeReplicatedMirror(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext)
{
    ResetRuntimeState(InFlowAsset, InExecutionContext, false);
}

void UDreamFlowExecutor::ResetVariablesToDefaults()
{
    RuntimeVariables.Reset();

    if (FlowAsset != nullptr)
    {
        for (const FDreamFlowVariableDefinition& Variable : FlowAsset->Variables)
        {
            if (!Variable.Name.IsNone() && !RuntimeVariables.Contains(Variable.Name))
            {
                RuntimeVariables.Add(Variable.Name, Variable.DefaultValue);
            }
        }
    }

    DREAMFLOW_LOG(Variables, Verbose, "Reset %d runtime variables to flow defaults for asset '%s'.", RuntimeVariables.Num(), *GetNameSafe(FlowAsset));
}

bool UDreamFlowExecutor::StartFlow()
{
    if (FlowAsset == nullptr || FlowAsset->GetEntryNode() == nullptr)
    {
        DREAMFLOW_LOG(Execution, Warning, "StartFlow failed because asset '%s' is missing a valid entry node.", *GetNameSafe(FlowAsset));
        return false;
    }

    CurrentNode = nullptr;
    VisitedNodes.Reset();
    ClearAsyncExecutionState();
    bIsRunning = true;
    bIsPaused = false;
    bBreakOnNextNode = false;
    bHasFinished = false;
    RegisterWithDebugger();
    OnFlowStarted.Broadcast();
    BroadcastDebugStateChanged();

    DREAMFLOW_LOG(Execution, Log, "Started flow '%s'.", *GetNameSafe(FlowAsset));

    const bool bAutoExecuteEntryNode = FlowAsset->bAutoExecuteEntryNodeOnStart;
    if (!ActivateNode(FlowAsset->GetEntryNode(), bAutoExecuteEntryNode))
    {
        DREAMFLOW_LOG(Execution, Warning, "StartFlow failed while entering the entry node for asset '%s'.", *GetNameSafe(FlowAsset));
        FinishFlow();
        return false;
    }

    if (!bAutoExecuteEntryNode)
    {
        DREAMFLOW_LOG(Execution, Log, "StartFlow entered the entry node for flow '%s' without auto-executing it.", *GetNameSafe(FlowAsset));
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
        ClearAsyncExecutionState();
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
    ClearAsyncExecutionState();
    CurrentNode = nullptr;
    BroadcastDebugStateChanged();
    OnFlowFinished.Broadcast();
    UnregisterFromDebugger();

    DREAMFLOW_LOG(Execution, Log, "Finished flow '%s'.", *GetNameSafe(FlowAsset));
}

bool UDreamFlowExecutor::Advance()
{
    if (!bIsRunning || bIsPaused || bIsWaitingForAsyncNode || bHasQueuedAsyncCompletion || CurrentNode == nullptr)
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

bool UDreamFlowExecutor::Step()
{
    return Advance();
}

bool UDreamFlowExecutor::MoveToChildByIndex(int32 ChildIndex)
{
    if (!bIsRunning || bIsPaused || bIsWaitingForAsyncNode || bHasQueuedAsyncCompletion || CurrentNode == nullptr)
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

bool UDreamFlowExecutor::MoveToOutputPin(FName OutputPinName)
{
    if (!bIsRunning || bIsPaused || bIsWaitingForAsyncNode || bHasQueuedAsyncCompletion || CurrentNode == nullptr || OutputPinName.IsNone())
    {
        return false;
    }

    return EnterNode(CurrentNode->GetFirstChildForOutputPin(OutputPinName));
}

bool UDreamFlowExecutor::StepToOutputPin(FName OutputPinName)
{
    return MoveToOutputPin(OutputPinName);
}

bool UDreamFlowExecutor::ChooseChild(UDreamFlowNode* ChildNode)
{
    if (!bIsRunning || bIsPaused || bIsWaitingForAsyncNode || bHasQueuedAsyncCompletion || CurrentNode == nullptr || ChildNode == nullptr)
    {
        return false;
    }

    const TArray<UDreamFlowNode*> Children = CurrentNode->GetChildrenCopy();
    return Children.Contains(ChildNode) ? EnterNode(ChildNode) : false;
}

bool UDreamFlowExecutor::EnterNode(UDreamFlowNode* Node)
{
    return ActivateNode(Node, true);
}

bool UDreamFlowExecutor::ActivateNode(UDreamFlowNode* Node, const bool bExecuteNode)
{
    if (!bIsRunning || bIsPaused || bIsWaitingForAsyncNode || bHasQueuedAsyncCompletion || FlowAsset == nullptr || Node == nullptr)
    {
        DREAMFLOW_LOG(Execution, Warning, "EnterNode rejected for asset '%s'. Running=%d Paused=%d Node=%s", *GetNameSafe(FlowAsset), bIsRunning, bIsPaused, *GetNameSafe(Node));
        return false;
    }

    if (!FlowAsset->GetNodes().Contains(Node) || !Node->CanEnterNode(ExecutionContext))
    {
        DREAMFLOW_LOG(Execution, Warning, "EnterNode rejected node '%s' because it is not owned by flow '%s' or CanEnterNode returned false.", *GetNameSafe(Node), *GetNameSafe(FlowAsset));
        return false;
    }

    if (CurrentNode != nullptr && CurrentNode != Node)
    {
        OnNodeExited.Broadcast(CurrentNode);
    }

    CurrentNode = Node;
    VisitedNodes.AddUnique(Node);
    OnNodeEntered.Broadcast(Node);
    DREAMFLOW_LOG(Execution, Log, "Entered node '%s' (%s).", *Node->GetNodeDisplayName().ToString(), *Node->NodeGuid.ToString(EGuidFormats::Short));
    bool bHitBreakpoint = false;
    if (ShouldPauseAtNode(Node, bHitBreakpoint))
    {
        bIsPaused = true;
        if (GEngine != nullptr)
        {
            if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
            {
                DebuggerSubsystem->NotifyExecutionPaused(this, Node, bHitBreakpoint);
            }
        }
        BroadcastDebugStateChanged();
        OnExecutionPaused.Broadcast(Node);
        DREAMFLOW_LOG(Execution, Display, "Paused flow '%s' at node '%s'%s.", *GetNameSafe(FlowAsset), *Node->GetNodeDisplayName().ToString(), bHitBreakpoint ? TEXT(" because of a breakpoint") : TEXT(""));
        return true;
    }

    if (bExecuteNode)
    {
        return ExecuteCurrentNode();
    }

    BroadcastDebugStateChanged();
    return true;
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
    if (GEngine != nullptr)
    {
        if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
        {
            DebuggerSubsystem->NotifyExecutionPaused(this, CurrentNode, false);
        }
    }
    BroadcastDebugStateChanged();
    OnExecutionPaused.Broadcast(CurrentNode);
    DREAMFLOW_LOG(Execution, Display, "Paused execution manually at node '%s'.", CurrentNode != nullptr ? *CurrentNode->GetNodeDisplayName().ToString() : TEXT("<none>"));
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
    DREAMFLOW_LOG(Execution, Display, "Continuing execution at node '%s'.", CurrentNode != nullptr ? *CurrentNode->GetNodeDisplayName().ToString() : TEXT("<none>"));

    if (bHasQueuedAsyncCompletion)
    {
        const FName QueuedOutputPinName = QueuedAsyncCompletionOutputPin;
        bHasQueuedAsyncCompletion = false;
        QueuedAsyncCompletionOutputPin = NAME_None;
        return TryConsumeCompletedAsyncNode(QueuedOutputPinName);
    }

    if (bIsWaitingForAsyncNode)
    {
        return true;
    }

    return ExecuteCurrentNode();
}

bool UDreamFlowExecutor::StepExecution()
{
    if (!bIsRunning)
    {
        return false;
    }

    bBreakOnNextNode = true;
    DREAMFLOW_LOG(Execution, Verbose, "Queued single-step execution for flow '%s'.", *GetNameSafe(FlowAsset));
    return bIsPaused ? ContinueExecution() : true;
}

void UDreamFlowExecutor::SetPauseOnBreakpoints(bool bEnabled)
{
    bPauseOnBreakpoints = bEnabled;
    BroadcastDebugStateChanged();
    DREAMFLOW_LOG(Execution, Verbose, "Pause on breakpoints %s for flow '%s'.", bEnabled ? TEXT("enabled") : TEXT("disabled"), *GetNameSafe(FlowAsset));
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
    return (CurrentNode != nullptr && !bIsWaitingForAsyncNode && !bHasQueuedAsyncCompletion)
        ? CurrentNode->GetChildrenCopy()
        : TArray<UDreamFlowNode*>();
}

TArray<FDreamFlowNodeOutputPin> UDreamFlowExecutor::GetAvailableOutputPins() const
{
    if (CurrentNode == nullptr || bIsWaitingForAsyncNode || bHasQueuedAsyncCompletion)
    {
        return TArray<FDreamFlowNodeOutputPin>();
    }

    TArray<FDreamFlowNodeOutputPin> AvailablePins;
    for (const FDreamFlowNodeOutputPin& OutputPin : CurrentNode->GetOutputPins())
    {
        if (CurrentNode->GetFirstChildForOutputPin(OutputPin.PinName) != nullptr)
        {
            AvailablePins.Add(OutputPin);
        }
    }

    return AvailablePins;
}

bool UDreamFlowExecutor::IsCurrentNodeAutomatic() const
{
    return CurrentNode != nullptr
        && CurrentNode->GetTransitionMode() == EDreamFlowNodeTransitionMode::Automatic;
}

bool UDreamFlowExecutor::IsWaitingForManualStep() const
{
    return bIsRunning
        && !bIsPaused
        && !bIsWaitingForAsyncNode
        && !bHasQueuedAsyncCompletion
        && CurrentNode != nullptr
        && CurrentNode->GetTransitionMode() == EDreamFlowNodeTransitionMode::Manual;
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
        return bIsWaitingForAsyncNode
            ? EDreamFlowExecutorDebugState::Waiting
            : EDreamFlowExecutorDebugState::Running;
    }

    return bHasFinished ? EDreamFlowExecutorDebugState::Finished : EDreamFlowExecutorDebugState::Idle;
}

bool UDreamFlowExecutor::IsWaitingForAsyncNode() const
{
    return bIsWaitingForAsyncNode;
}

UDreamFlowNode* UDreamFlowExecutor::GetPendingAsyncNode() const
{
    return PendingAsyncNode;
}

UDreamFlowAsyncContext* UDreamFlowExecutor::BeginAsyncNode(UDreamFlowNode* Node)
{
    if (!bIsRunning || bIsPaused || CurrentNode == nullptr || Node == nullptr || CurrentNode != Node)
    {
        DREAMFLOW_LOG(Execution, Warning, "BeginAsyncNode rejected for '%s'. Current='%s'.", *GetNameSafe(Node), *GetNameSafe(CurrentNode));
        return nullptr;
    }

    if (bIsWaitingForAsyncNode && PendingAsyncNode == Node && ActiveAsyncContext != nullptr)
    {
        return ActiveAsyncContext;
    }

    PendingAsyncNode = Node;
    bIsWaitingForAsyncNode = true;
    bHasQueuedAsyncCompletion = false;
    QueuedAsyncCompletionOutputPin = NAME_None;

    if (ActiveAsyncContext == nullptr)
    {
        ActiveAsyncContext = NewObject<UDreamFlowAsyncContext>(this);
    }

    ActiveAsyncContext->Initialize(this, Node);
    DREAMFLOW_LOG(Execution, Log, "Node '%s' entered async waiting mode on flow '%s'.", *Node->GetNodeDisplayName().ToString(), *GetNameSafe(FlowAsset));
    BroadcastDebugStateChanged();
    return ActiveAsyncContext;
}

bool UDreamFlowExecutor::CompleteAsyncNode(FName OutputPinName)
{
    return CompleteAsyncNodeForNode(PendingAsyncNode, OutputPinName);
}

bool UDreamFlowExecutor::CompleteAsyncNodeForNode(UDreamFlowNode* Node, FName OutputPinName)
{
    if (!bIsRunning || PendingAsyncNode == nullptr || Node == nullptr || PendingAsyncNode != Node)
    {
        return false;
    }

    const FName ResolvedOutputPinName = ResolveCompletedAsyncOutputPin(PendingAsyncNode, OutputPinName);
    const bool bCanFinishWithoutTransition = PendingAsyncNode->GetChildrenCopy().Num() == 0;
    if ((ResolvedOutputPinName.IsNone() && !bCanFinishWithoutTransition)
        || (!ResolvedOutputPinName.IsNone()
            && !bCanFinishWithoutTransition
            && PendingAsyncNode->GetFirstChildForOutputPin(ResolvedOutputPinName) == nullptr))
    {
        DREAMFLOW_LOG(Execution, Warning, "CompleteAsyncNode rejected for node '%s' because no valid completion output could be resolved.", *PendingAsyncNode->GetNodeDisplayName().ToString());
        return false;
    }

    if (bIsPaused)
    {
        bIsWaitingForAsyncNode = false;
        bHasQueuedAsyncCompletion = true;
        QueuedAsyncCompletionOutputPin = ResolvedOutputPinName;
        ActiveAsyncContext = nullptr;
        BroadcastDebugStateChanged();
        DREAMFLOW_LOG(Execution, Verbose, "Queued async completion for node '%s' while execution is paused.", *PendingAsyncNode->GetNodeDisplayName().ToString());
        return true;
    }

    return TryConsumeCompletedAsyncNode(ResolvedOutputPinName);
}

void UDreamFlowExecutor::BuildReplicatedState(FDreamFlowReplicatedExecutionState& OutState) const
{
    OutState = FDreamFlowReplicatedExecutionState();
    OutState.DebugState = GetDebugState();
    OutState.CurrentNodeGuid = CurrentNode != nullptr ? CurrentNode->NodeGuid : FGuid();
    OutState.bPauseOnBreakpoints = bPauseOnBreakpoints;

    for (UDreamFlowNode* VisitedNode : VisitedNodes)
    {
        if (VisitedNode != nullptr && VisitedNode->NodeGuid.IsValid())
        {
            OutState.VisitedNodeGuids.Add(VisitedNode->NodeGuid);
        }
    }

    TArray<FName> VariableNames;
    RuntimeVariables.GenerateKeyArray(VariableNames);
    VariableNames.Sort(FNameLexicalLess());

    for (const FName VariableName : VariableNames)
    {
        if (const FDreamFlowValue* Value = RuntimeVariables.Find(VariableName))
        {
            FDreamFlowReplicatedVariableValue& ReplicatedVariable = OutState.Variables.AddDefaulted_GetRef();
            ReplicatedVariable.Name = VariableName;
            ReplicatedVariable.Value = *Value;
        }
    }

    DREAMFLOW_LOG(Replication, Verbose, "Built replicated execution state for flow '%s' with %d visited nodes and %d variables.", *GetNameSafe(FlowAsset), OutState.VisitedNodeGuids.Num(), OutState.Variables.Num());
}

void UDreamFlowExecutor::ApplyReplicatedState(const FDreamFlowReplicatedExecutionState& InState)
{
    bPauseOnBreakpoints = InState.bPauseOnBreakpoints;
    bBreakOnNextNode = false;
    bIsPaused = InState.DebugState == EDreamFlowExecutorDebugState::Paused;
    bIsRunning =
        InState.DebugState == EDreamFlowExecutorDebugState::Running
        || InState.DebugState == EDreamFlowExecutorDebugState::Waiting
        || InState.DebugState == EDreamFlowExecutorDebugState::Paused;
    bHasFinished = InState.DebugState == EDreamFlowExecutorDebugState::Finished;
    bIsWaitingForAsyncNode = InState.DebugState == EDreamFlowExecutorDebugState::Waiting;
    bHasQueuedAsyncCompletion = false;
    QueuedAsyncCompletionOutputPin = NAME_None;
    ActiveAsyncContext = nullptr;

    CurrentNode = FlowAsset != nullptr ? FlowAsset->FindNodeByGuid(InState.CurrentNodeGuid) : nullptr;
    PendingAsyncNode = bIsWaitingForAsyncNode ? CurrentNode : nullptr;

    VisitedNodes.Reset();
    if (FlowAsset != nullptr)
    {
        for (const FGuid& NodeGuid : InState.VisitedNodeGuids)
        {
            if (UDreamFlowNode* VisitedNode = FlowAsset->FindNodeByGuid(NodeGuid))
            {
                VisitedNodes.AddUnique(VisitedNode);
            }
        }
    }

    RuntimeVariables.Reset();
    ResetVariablesToDefaults();
    for (const FDreamFlowReplicatedVariableValue& ReplicatedVariable : InState.Variables)
    {
        if (!ReplicatedVariable.Name.IsNone())
        {
            RuntimeVariables.FindOrAdd(ReplicatedVariable.Name) = ReplicatedVariable.Value;
        }
    }

    DREAMFLOW_LOG(Replication, Verbose, "Applied replicated execution state for flow '%s'. Current node='%s', variables=%d.", *GetNameSafe(FlowAsset), CurrentNode != nullptr ? *CurrentNode->GetNodeDisplayName().ToString() : TEXT("<none>"), RuntimeVariables.Num());
}

FDreamFlowExecutorRuntimeStateChangedNativeSignature& UDreamFlowExecutor::OnRuntimeStateChangedNative()
{
    return RuntimeStateChangedNative;
}

bool UDreamFlowExecutor::HasVariable(FName VariableName) const
{
    return !VariableName.IsNone() && RuntimeVariables.Contains(VariableName);
}

bool UDreamFlowExecutor::GetVariableValue(FName VariableName, FDreamFlowValue& OutValue) const
{
    if (const FDreamFlowValue* StoredValue = RuntimeVariables.Find(VariableName))
    {
        OutValue = *StoredValue;
        return true;
    }

    return false;
}

bool UDreamFlowExecutor::GetVariableBoolValue(FName VariableName, bool& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedValue(this, VariableName, EDreamFlowValueType::Bool, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.BoolValue;
    return true;
}

bool UDreamFlowExecutor::GetVariableIntValue(FName VariableName, int32& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedValue(this, VariableName, EDreamFlowValueType::Int, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.IntValue;
    return true;
}

bool UDreamFlowExecutor::GetVariableFloatValue(FName VariableName, float& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedValue(this, VariableName, EDreamFlowValueType::Float, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.FloatValue;
    return true;
}

bool UDreamFlowExecutor::GetVariableNameValue(FName VariableName, FName& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedValue(this, VariableName, EDreamFlowValueType::Name, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.NameValue;
    return true;
}

bool UDreamFlowExecutor::GetVariableStringValue(FName VariableName, FString& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedValue(this, VariableName, EDreamFlowValueType::String, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.StringValue;
    return true;
}

bool UDreamFlowExecutor::GetVariableTextValue(FName VariableName, FText& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedValue(this, VariableName, EDreamFlowValueType::Text, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.TextValue;
    return true;
}

bool UDreamFlowExecutor::GetVariableGameplayTagValue(FName VariableName, FGameplayTag& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedValue(this, VariableName, EDreamFlowValueType::GameplayTag, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.GameplayTagValue;
    return true;
}

bool UDreamFlowExecutor::GetVariableObjectValue(FName VariableName, UObject*& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedValue(this, VariableName, EDreamFlowValueType::Object, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.ObjectValue.Get();
    return true;
}

bool UDreamFlowExecutor::SetVariableBoolValue(FName VariableName, bool InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Bool;
    Value.BoolValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutor::SetVariableIntValue(FName VariableName, int32 InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Int;
    Value.IntValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutor::SetVariableFloatValue(FName VariableName, float InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Float;
    Value.FloatValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutor::SetVariableNameValue(FName VariableName, FName InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Name;
    Value.NameValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutor::SetVariableStringValue(FName VariableName, const FString& InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::String;
    Value.StringValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutor::SetVariableTextValue(FName VariableName, const FText& InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Text;
    Value.TextValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutor::SetVariableGameplayTagValue(FName VariableName, FGameplayTag InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::GameplayTag;
    Value.GameplayTagValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutor::SetVariableObjectValue(FName VariableName, UObject* InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Object;
    Value.ObjectValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutor::SetVariableValue(FName VariableName, const FDreamFlowValue& InValue)
{
    const FDreamFlowVariableDefinition* VariableDefinition = FlowAsset != nullptr ? FlowAsset->FindVariableDefinition(VariableName) : nullptr;
    if (VariableDefinition == nullptr)
    {
        return false;
    }

    FDreamFlowValue ConvertedValue;
    if (!DreamFlowVariable::TryConvertValue(InValue, VariableDefinition->DefaultValue.Type, ConvertedValue))
    {
        DREAMFLOW_LOG(Variables, Warning, "Failed to convert value for variable '%s' on flow '%s'.", *VariableName.ToString(), *GetNameSafe(FlowAsset));
        return false;
    }

    const FString PreviousValue = RuntimeVariables.Contains(VariableName) ? RuntimeVariables.FindChecked(VariableName).Describe() : TEXT("<unset>");
    RuntimeVariables.FindOrAdd(VariableName) = ConvertedValue;
    DREAMFLOW_LOG(Variables, Log, "Variable '%s' changed from '%s' to '%s' on flow '%s'.", *VariableName.ToString(), *PreviousValue, *ConvertedValue.Describe(), *GetNameSafe(FlowAsset));
    NotifyDebuggerStateChanged();
    return true;
}

bool UDreamFlowExecutor::ResolveBindingValue(const FDreamFlowValueBinding& Binding, FDreamFlowValue& OutValue) const
{
    if (Binding.SourceType == EDreamFlowValueSourceType::FlowVariable)
    {
        return GetVariableValue(Binding.VariableName, OutValue);
    }

    OutValue = Binding.LiteralValue;
    return true;
}

bool UDreamFlowExecutor::ResolveBindingAsBool(const FDreamFlowValueBinding& Binding, bool& OutValue) const
{
    FDreamFlowValue ResolvedValue;
    if (!ResolveBindingValue(Binding, ResolvedValue))
    {
        return false;
    }

    FDreamFlowValue ConvertedValue;
    if (!DreamFlowVariable::TryConvertValue(ResolvedValue, EDreamFlowValueType::Bool, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.BoolValue;
    return true;
}

bool UDreamFlowExecutor::ExecuteCurrentNode()
{
    if (!bIsRunning || bIsPaused || CurrentNode == nullptr)
    {
        return false;
    }

    UDreamFlowNode* ExecutedNode = CurrentNode;
    ExecutedNode->ExecuteNodeWithExecutor(ExecutionContext, this);
    NotifyDebuggerStateChanged();

    if (!bIsRunning || bIsPaused || CurrentNode != ExecutedNode)
    {
        return true;
    }

    if (bIsWaitingForAsyncNode)
    {
        return true;
    }

    if (ExecutedNode->SupportsAutomaticTransition(ExecutionContext, this))
    {
        const FName AutoOutputPin = ExecutedNode->ResolveAutomaticTransitionOutputPin(ExecutionContext, this);
        if (!AutoOutputPin.IsNone())
        {
            return MoveToOutputPin(AutoOutputPin);
        }
    }

    if (CurrentNode->IsTerminalNode() && CurrentNode->GetChildrenCopy().Num() == 0)
    {
        FinishFlow();
    }

    return true;
}

bool UDreamFlowExecutor::TryConsumeCompletedAsyncNode(FName RequestedOutputPinName)
{
    if (!bIsRunning || PendingAsyncNode == nullptr || CurrentNode != PendingAsyncNode)
    {
        return false;
    }

    UDreamFlowNode* AsyncNode = PendingAsyncNode;
    const FName ResolvedOutputPinName = ResolveCompletedAsyncOutputPin(AsyncNode, RequestedOutputPinName);
    const bool bCanFinishWithoutTransition = AsyncNode->GetChildrenCopy().Num() == 0;
    UDreamFlowNode* NextNode = !ResolvedOutputPinName.IsNone() ? AsyncNode->GetFirstChildForOutputPin(ResolvedOutputPinName) : nullptr;
    if (ResolvedOutputPinName.IsNone() && !bCanFinishWithoutTransition)
    {
        return false;
    }

    if (!ResolvedOutputPinName.IsNone())
    {
        if (NextNode != nullptr)
        {
            ClearAsyncExecutionState();
            DREAMFLOW_LOG(Execution, Log, "Async node '%s' completed through output '%s'.", *AsyncNode->GetNodeDisplayName().ToString(), *ResolvedOutputPinName.ToString());
            return EnterNode(NextNode);
        }

        if (!bCanFinishWithoutTransition)
        {
            return false;
        }
    }

    if (bCanFinishWithoutTransition)
    {
        ClearAsyncExecutionState();
        DREAMFLOW_LOG(Execution, Log, "Async node '%s' completed and finished flow '%s'.", *AsyncNode->GetNodeDisplayName().ToString(), *GetNameSafe(FlowAsset));
        FinishFlow();
        return true;
    }

    if (!ResolvedOutputPinName.IsNone())
    {
        DREAMFLOW_LOG(Execution, Warning, "Async node '%s' completed with output '%s', but no child is connected to that output.", *AsyncNode->GetNodeDisplayName().ToString(), *ResolvedOutputPinName.ToString());
        return false;
    }

    return false;
}

FName UDreamFlowExecutor::ResolveCompletedAsyncOutputPin(const UDreamFlowNode* AsyncNode, FName RequestedOutputPinName) const
{
    if (AsyncNode == nullptr)
    {
        return NAME_None;
    }

    const TArray<FDreamFlowNodeOutputPin> OutputPins = AsyncNode->GetOutputPins();
    if (!RequestedOutputPinName.IsNone())
    {
        for (const FDreamFlowNodeOutputPin& OutputPin : OutputPins)
        {
            if (OutputPin.PinName == RequestedOutputPinName)
            {
                return RequestedOutputPinName;
            }
        }

        return NAME_None;
    }

    for (const FDreamFlowNodeOutputPin& OutputPin : OutputPins)
    {
        if (AsyncNode->GetFirstChildForOutputPin(OutputPin.PinName) != nullptr)
        {
            return OutputPin.PinName;
        }
    }

    return OutputPins.Num() > 0 ? OutputPins[0].PinName : NAME_None;
}

bool UDreamFlowExecutor::ShouldPauseAtNode(const UDreamFlowNode* Node, bool& bOutHitBreakpoint)
{
    bOutHitBreakpoint = false;

    if (Node == nullptr)
    {
        return false;
    }

    if (bBreakOnNextNode)
    {
        bBreakOnNextNode = false;
        return true;
    }

    bOutHitBreakpoint = bPauseOnBreakpoints && FlowAsset != nullptr && FlowAsset->HasBreakpointOnNode(Node->NodeGuid);
    return bOutHitBreakpoint;
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
    RuntimeStateChangedNative.Broadcast(this);

    if (GEngine == nullptr)
    {
        return;
    }

    if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
    {
        DebuggerSubsystem->NotifyExecutorChanged(this);
    }
}

void UDreamFlowExecutor::ClearAsyncExecutionState()
{
    PendingAsyncNode = nullptr;
    ActiveAsyncContext = nullptr;
    bIsWaitingForAsyncNode = false;
    bHasQueuedAsyncCompletion = false;
    QueuedAsyncCompletionOutputPin = NAME_None;
}

void UDreamFlowExecutor::ResetRuntimeState(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext, bool bNotifyDebugger)
{
    UnregisterFromDebugger();
    FlowAsset = InFlowAsset;
    ExecutionContext = InExecutionContext;
    CurrentNode = nullptr;
    VisitedNodes.Reset();
    RuntimeVariables.Reset();
    ClearAsyncExecutionState();
    bIsRunning = false;
    bIsPaused = false;
    bPauseOnBreakpoints = true;
    bBreakOnNextNode = false;
    bHasFinished = false;

    ResetVariablesToDefaults();

    if (bNotifyDebugger)
    {
        BroadcastDebugStateChanged();
    }
}
