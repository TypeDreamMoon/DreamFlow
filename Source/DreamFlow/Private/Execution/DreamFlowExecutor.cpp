#include "Execution/DreamFlowExecutor.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "Engine/Engine.h"
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

bool UDreamFlowExecutor::MoveToOutputPin(FName OutputPinName)
{
    if (!bIsRunning || bIsPaused || CurrentNode == nullptr || OutputPinName.IsNone())
    {
        return false;
    }

    return EnterNode(CurrentNode->GetFirstChildForOutputPin(OutputPinName));
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
    if (GEngine != nullptr)
    {
        if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
        {
            DebuggerSubsystem->NotifyExecutionPaused(this, CurrentNode, false);
        }
    }
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
}

void UDreamFlowExecutor::ApplyReplicatedState(const FDreamFlowReplicatedExecutionState& InState)
{
    bPauseOnBreakpoints = InState.bPauseOnBreakpoints;
    bBreakOnNextNode = false;
    bIsPaused = InState.DebugState == EDreamFlowExecutorDebugState::Paused;
    bIsRunning = InState.DebugState == EDreamFlowExecutorDebugState::Running || InState.DebugState == EDreamFlowExecutorDebugState::Paused;
    bHasFinished = InState.DebugState == EDreamFlowExecutorDebugState::Finished;

    CurrentNode = FlowAsset != nullptr ? FlowAsset->FindNodeByGuid(InState.CurrentNodeGuid) : nullptr;

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
        return false;
    }

    RuntimeVariables.FindOrAdd(VariableName) = ConvertedValue;
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

void UDreamFlowExecutor::ResetRuntimeState(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext, bool bNotifyDebugger)
{
    UnregisterFromDebugger();
    FlowAsset = InFlowAsset;
    ExecutionContext = InExecutionContext;
    CurrentNode = nullptr;
    VisitedNodes.Reset();
    RuntimeVariables.Reset();
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
