#include "Execution/DreamFlowExecutorComponent.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "Net/UnrealNetwork.h"

namespace DreamFlowExecutorComponentPrivate
{
    static const FDreamFlowValue* FindReplicatedVariableValue(const FDreamFlowReplicatedExecutionState& State, const FName VariableName)
    {
        if (VariableName.IsNone())
        {
            return nullptr;
        }

        for (const FDreamFlowReplicatedVariableValue& Variable : State.Variables)
        {
            if (Variable.Name == VariableName)
            {
                return &Variable.Value;
            }
        }

        return nullptr;
    }

    static bool GetStoredVariableValue(const UDreamFlowExecutor* Executor, const FDreamFlowReplicatedExecutionState& State, const FName VariableName, FDreamFlowValue& OutValue)
    {
        if (Executor != nullptr && Executor->GetVariableValue(VariableName, OutValue))
        {
            return true;
        }

        if (const FDreamFlowValue* ReplicatedValue = FindReplicatedVariableValue(State, VariableName))
        {
            OutValue = *ReplicatedValue;
            return true;
        }

        return false;
    }

    static bool GetConvertedStoredVariableValue(
        const UDreamFlowExecutor* Executor,
        const FDreamFlowReplicatedExecutionState& State,
        const FName VariableName,
        const EDreamFlowValueType TargetType,
        FDreamFlowValue& OutValue)
    {
        FDreamFlowValue StoredValue;
        if (!GetStoredVariableValue(Executor, State, VariableName, StoredValue))
        {
            return false;
        }

        return DreamFlowVariable::TryConvertValue(StoredValue, TargetType, OutValue);
    }
}

UDreamFlowExecutorComponent::UDreamFlowExecutorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UDreamFlowExecutorComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoStart && IsServerAuthority())
    {
        StartFlowLocal();
    }
}

void UDreamFlowExecutorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, FlowAsset);
    DOREPLIFETIME(ThisClass, ExecutorClass);
    DOREPLIFETIME(ThisClass, bAutoStart);
    DOREPLIFETIME(ThisClass, ReplicatedExecutionState);
}

UDreamFlowExecutor* UDreamFlowExecutorComponent::CreateExecutor()
{
    if (IsServerAuthority())
    {
        return ResetExecutorToCurrentConfig();
    }

    if (UDreamFlowExecutor* MirrorExecutor = GetOrCreateExecutor(false))
    {
        MirrorExecutor->InitializeReplicatedMirror(FlowAsset, GetOwner());
        MirrorExecutor->ApplyReplicatedState(ReplicatedExecutionState);
    }

    return Executor;
}

bool UDreamFlowExecutorComponent::StartFlow()
{
    if (IsServerAuthority())
    {
        return StartFlowLocal();
    }

    ServerStartFlow();
    return true;
}

bool UDreamFlowExecutorComponent::RestartFlow()
{
    if (IsServerAuthority())
    {
        return RestartFlowLocal();
    }

    ServerRestartFlow();
    return true;
}

void UDreamFlowExecutorComponent::FinishFlow()
{
    if (IsServerAuthority())
    {
        FinishFlowLocal();
        return;
    }

    ServerFinishFlow();
}

bool UDreamFlowExecutorComponent::Advance()
{
    if (IsServerAuthority())
    {
        return AdvanceLocal();
    }

    ServerAdvance();
    return true;
}

bool UDreamFlowExecutorComponent::MoveToChildByIndex(int32 ChildIndex)
{
    if (IsServerAuthority())
    {
        return MoveToChildByIndexLocal(ChildIndex);
    }

    ServerMoveToChildByIndex(ChildIndex);
    return true;
}

bool UDreamFlowExecutorComponent::MoveToOutputPin(FName OutputPinName)
{
    if (IsServerAuthority())
    {
        return MoveToOutputPinLocal(OutputPinName);
    }

    ServerMoveToOutputPin(OutputPinName);
    return true;
}

bool UDreamFlowExecutorComponent::ChooseChild(UDreamFlowNode* ChildNode)
{
    const FGuid ChildNodeGuid = ChildNode != nullptr ? ChildNode->NodeGuid : FGuid();
    if (!ChildNodeGuid.IsValid())
    {
        return false;
    }

    if (IsServerAuthority())
    {
        return ChooseChildByGuidLocal(ChildNodeGuid);
    }

    ServerChooseChildByGuid(ChildNodeGuid);
    return true;
}

UDreamFlowExecutor* UDreamFlowExecutorComponent::GetExecutor() const
{
    return Executor;
}

UDreamFlowNode* UDreamFlowExecutorComponent::GetCurrentNode() const
{
    if (Executor != nullptr)
    {
        return Executor->GetCurrentNode();
    }

    return FlowAsset != nullptr ? FlowAsset->FindNodeByGuid(ReplicatedExecutionState.CurrentNodeGuid) : nullptr;
}

bool UDreamFlowExecutorComponent::HasVariable(FName VariableName) const
{
    if (Executor != nullptr)
    {
        return Executor->HasVariable(VariableName);
    }

    return DreamFlowExecutorComponentPrivate::FindReplicatedVariableValue(ReplicatedExecutionState, VariableName) != nullptr;
}

bool UDreamFlowExecutorComponent::GetVariableValue(FName VariableName, FDreamFlowValue& OutValue) const
{
    return DreamFlowExecutorComponentPrivate::GetStoredVariableValue(Executor.Get(), ReplicatedExecutionState, VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableBoolValue(FName VariableName, bool& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorComponentPrivate::GetConvertedStoredVariableValue(
            Executor.Get(),
            ReplicatedExecutionState,
            VariableName,
            EDreamFlowValueType::Bool,
            ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.BoolValue;
    return true;
}

bool UDreamFlowExecutorComponent::GetVariableIntValue(FName VariableName, int32& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorComponentPrivate::GetConvertedStoredVariableValue(
            Executor.Get(),
            ReplicatedExecutionState,
            VariableName,
            EDreamFlowValueType::Int,
            ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.IntValue;
    return true;
}

bool UDreamFlowExecutorComponent::GetVariableFloatValue(FName VariableName, float& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorComponentPrivate::GetConvertedStoredVariableValue(
            Executor.Get(),
            ReplicatedExecutionState,
            VariableName,
            EDreamFlowValueType::Float,
            ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.FloatValue;
    return true;
}

bool UDreamFlowExecutorComponent::GetVariableNameValue(FName VariableName, FName& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorComponentPrivate::GetConvertedStoredVariableValue(
            Executor.Get(),
            ReplicatedExecutionState,
            VariableName,
            EDreamFlowValueType::Name,
            ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.NameValue;
    return true;
}

bool UDreamFlowExecutorComponent::GetVariableStringValue(FName VariableName, FString& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorComponentPrivate::GetConvertedStoredVariableValue(
            Executor.Get(),
            ReplicatedExecutionState,
            VariableName,
            EDreamFlowValueType::String,
            ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.StringValue;
    return true;
}

bool UDreamFlowExecutorComponent::GetVariableTextValue(FName VariableName, FText& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorComponentPrivate::GetConvertedStoredVariableValue(
            Executor.Get(),
            ReplicatedExecutionState,
            VariableName,
            EDreamFlowValueType::Text,
            ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.TextValue;
    return true;
}

bool UDreamFlowExecutorComponent::GetVariableGameplayTagValue(FName VariableName, FGameplayTag& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorComponentPrivate::GetConvertedStoredVariableValue(
            Executor.Get(),
            ReplicatedExecutionState,
            VariableName,
            EDreamFlowValueType::GameplayTag,
            ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.GameplayTagValue;
    return true;
}

bool UDreamFlowExecutorComponent::GetVariableObjectValue(FName VariableName, UObject*& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorComponentPrivate::GetConvertedStoredVariableValue(
            Executor.Get(),
            ReplicatedExecutionState,
            VariableName,
            EDreamFlowValueType::Object,
            ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.ObjectValue.Get();
    return true;
}

bool UDreamFlowExecutorComponent::SetVariableValue(FName VariableName, const FDreamFlowValue& InValue)
{
    if (IsServerAuthority())
    {
        return SetVariableValueLocal(VariableName, InValue);
    }

    ServerSetVariableValue(VariableName, InValue);
    return true;
}

bool UDreamFlowExecutorComponent::SetVariableBoolValue(FName VariableName, bool InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Bool;
    Value.BoolValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutorComponent::SetVariableIntValue(FName VariableName, int32 InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Int;
    Value.IntValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutorComponent::SetVariableFloatValue(FName VariableName, float InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Float;
    Value.FloatValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutorComponent::SetVariableNameValue(FName VariableName, FName InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Name;
    Value.NameValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutorComponent::SetVariableStringValue(FName VariableName, const FString& InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::String;
    Value.StringValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutorComponent::SetVariableTextValue(FName VariableName, const FText& InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Text;
    Value.TextValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutorComponent::SetVariableGameplayTagValue(FName VariableName, FGameplayTag InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::GameplayTag;
    Value.GameplayTagValue = InValue;
    return SetVariableValue(VariableName, Value);
}

bool UDreamFlowExecutorComponent::SetVariableObjectValue(FName VariableName, UObject* InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Object;
    Value.ObjectValue = InValue;
    return SetVariableValue(VariableName, Value);
}

void UDreamFlowExecutorComponent::ResetVariablesToDefaults()
{
    if (IsServerAuthority())
    {
        ResetVariablesToDefaultsLocal();
        return;
    }

    ServerResetVariablesToDefaults();
}

void UDreamFlowExecutorComponent::HandleFlowStarted()
{
    OnFlowStarted.Broadcast();
}

void UDreamFlowExecutorComponent::HandleFlowFinished()
{
    OnFlowFinished.Broadcast();
}

void UDreamFlowExecutorComponent::HandleNodeEntered(UDreamFlowNode* Node)
{
    OnNodeEntered.Broadcast(Node);
}

void UDreamFlowExecutorComponent::HandleNodeExited(UDreamFlowNode* Node)
{
    OnNodeExited.Broadcast(Node);
}

void UDreamFlowExecutorComponent::OnRep_FlowAsset()
{
    if (ApplyReplicatedStateToMirror())
    {
        BroadcastReplicatedStateEvents(LastAppliedReplicatedState, ReplicatedExecutionState);
        LastAppliedReplicatedState = ReplicatedExecutionState;
    }
}

void UDreamFlowExecutorComponent::OnRep_ReplicatedExecutionState()
{
    if (ApplyReplicatedStateToMirror())
    {
        BroadcastReplicatedStateEvents(LastAppliedReplicatedState, ReplicatedExecutionState);
        LastAppliedReplicatedState = ReplicatedExecutionState;
    }
}

void UDreamFlowExecutorComponent::ServerStartFlow_Implementation()
{
    StartFlowLocal();
}

void UDreamFlowExecutorComponent::ServerRestartFlow_Implementation()
{
    RestartFlowLocal();
}

void UDreamFlowExecutorComponent::ServerFinishFlow_Implementation()
{
    FinishFlowLocal();
}

void UDreamFlowExecutorComponent::ServerAdvance_Implementation()
{
    AdvanceLocal();
}

void UDreamFlowExecutorComponent::ServerMoveToChildByIndex_Implementation(int32 ChildIndex)
{
    MoveToChildByIndexLocal(ChildIndex);
}

void UDreamFlowExecutorComponent::ServerMoveToOutputPin_Implementation(FName OutputPinName)
{
    MoveToOutputPinLocal(OutputPinName);
}

void UDreamFlowExecutorComponent::ServerChooseChildByGuid_Implementation(FGuid ChildNodeGuid)
{
    ChooseChildByGuidLocal(ChildNodeGuid);
}

void UDreamFlowExecutorComponent::ServerSetVariableValue_Implementation(FName VariableName, FDreamFlowValue InValue)
{
    SetVariableValueLocal(VariableName, InValue);
}

void UDreamFlowExecutorComponent::ServerResetVariablesToDefaults_Implementation()
{
    ResetVariablesToDefaultsLocal();
}

bool UDreamFlowExecutorComponent::IsServerAuthority() const
{
    return GetOwner() == nullptr || GetOwner()->HasAuthority();
}

UDreamFlowExecutor* UDreamFlowExecutorComponent::GetOrCreateExecutor(bool bInitializeFromCurrentConfig)
{
    UClass* EffectiveClass = ExecutorClass != nullptr ? ExecutorClass.Get() : UDreamFlowExecutor::StaticClass();
    if (Executor == nullptr || Executor->GetClass() != EffectiveClass)
    {
        UnbindExecutorDelegates(Executor);
        Executor = NewObject<UDreamFlowExecutor>(this, EffectiveClass);
        BindExecutorDelegates(Executor);
    }

    if (Executor != nullptr && bInitializeFromCurrentConfig)
    {
        if (IsServerAuthority())
        {
            Executor->Initialize(FlowAsset, GetOwner());
        }
        else
        {
            Executor->InitializeReplicatedMirror(FlowAsset, GetOwner());
        }
    }

    return Executor;
}

UDreamFlowExecutor* UDreamFlowExecutorComponent::ResetExecutorToCurrentConfig()
{
    UDreamFlowExecutor* RuntimeExecutor = GetOrCreateExecutor(false);
    if (RuntimeExecutor != nullptr)
    {
        if (IsServerAuthority())
        {
            RuntimeExecutor->Initialize(FlowAsset, GetOwner());
            SyncReplicatedStateFromExecutor();
        }
        else
        {
            RuntimeExecutor->InitializeReplicatedMirror(FlowAsset, GetOwner());
            RuntimeExecutor->ApplyReplicatedState(ReplicatedExecutionState);
        }
    }

    return RuntimeExecutor;
}

void UDreamFlowExecutorComponent::BindExecutorDelegates(UDreamFlowExecutor* InExecutor)
{
    if (InExecutor == nullptr)
    {
        return;
    }

    InExecutor->OnFlowStarted.AddUniqueDynamic(this, &ThisClass::HandleFlowStarted);
    InExecutor->OnFlowFinished.AddUniqueDynamic(this, &ThisClass::HandleFlowFinished);
    InExecutor->OnNodeEntered.AddUniqueDynamic(this, &ThisClass::HandleNodeEntered);
    InExecutor->OnNodeExited.AddUniqueDynamic(this, &ThisClass::HandleNodeExited);
    ExecutorRuntimeStateChangedHandle = InExecutor->OnRuntimeStateChangedNative().AddUObject(this, &ThisClass::HandleExecutorRuntimeStateChanged);
}

void UDreamFlowExecutorComponent::UnbindExecutorDelegates(UDreamFlowExecutor* InExecutor)
{
    if (InExecutor == nullptr)
    {
        return;
    }

    InExecutor->OnFlowStarted.RemoveDynamic(this, &ThisClass::HandleFlowStarted);
    InExecutor->OnFlowFinished.RemoveDynamic(this, &ThisClass::HandleFlowFinished);
    InExecutor->OnNodeEntered.RemoveDynamic(this, &ThisClass::HandleNodeEntered);
    InExecutor->OnNodeExited.RemoveDynamic(this, &ThisClass::HandleNodeExited);

    if (ExecutorRuntimeStateChangedHandle.IsValid())
    {
        InExecutor->OnRuntimeStateChangedNative().Remove(ExecutorRuntimeStateChangedHandle);
        ExecutorRuntimeStateChangedHandle.Reset();
    }
}

void UDreamFlowExecutorComponent::SyncReplicatedStateFromExecutor()
{
    if (!IsServerAuthority() || Executor == nullptr)
    {
        return;
    }

    Executor->BuildReplicatedState(ReplicatedExecutionState);
    LastAppliedReplicatedState = ReplicatedExecutionState;
}

bool UDreamFlowExecutorComponent::ApplyReplicatedStateToMirror()
{
    if (IsServerAuthority())
    {
        return false;
    }

    if (FlowAsset == nullptr)
    {
        if (Executor != nullptr)
        {
            Executor->InitializeReplicatedMirror(nullptr, GetOwner());
        }
        return false;
    }

    UDreamFlowExecutor* MirrorExecutor = GetOrCreateExecutor(false);
    if (MirrorExecutor == nullptr)
    {
        return false;
    }

    MirrorExecutor->InitializeReplicatedMirror(FlowAsset, GetOwner());
    MirrorExecutor->ApplyReplicatedState(ReplicatedExecutionState);
    return true;
}

bool UDreamFlowExecutorComponent::StartFlowLocal()
{
    UDreamFlowExecutor* RuntimeExecutor = ResetExecutorToCurrentConfig();
    const bool bStarted = RuntimeExecutor != nullptr && RuntimeExecutor->StartFlow();
    SyncReplicatedStateFromExecutor();
    return bStarted;
}

bool UDreamFlowExecutorComponent::RestartFlowLocal()
{
    UDreamFlowExecutor* RuntimeExecutor = ResetExecutorToCurrentConfig();
    const bool bRestarted = RuntimeExecutor != nullptr && RuntimeExecutor->StartFlow();
    SyncReplicatedStateFromExecutor();
    return bRestarted;
}

void UDreamFlowExecutorComponent::FinishFlowLocal()
{
    if (Executor != nullptr)
    {
        Executor->FinishFlow();
        SyncReplicatedStateFromExecutor();
    }
    else
    {
        ReplicatedExecutionState = FDreamFlowReplicatedExecutionState();
        LastAppliedReplicatedState = ReplicatedExecutionState;
    }
}

bool UDreamFlowExecutorComponent::AdvanceLocal()
{
    UDreamFlowExecutor* RuntimeExecutor = GetOrCreateExecutor(false);
    const bool bAdvanced = RuntimeExecutor != nullptr && RuntimeExecutor->Advance();
    SyncReplicatedStateFromExecutor();
    return bAdvanced;
}

bool UDreamFlowExecutorComponent::MoveToChildByIndexLocal(int32 ChildIndex)
{
    UDreamFlowExecutor* RuntimeExecutor = GetOrCreateExecutor(false);
    const bool bMoved = RuntimeExecutor != nullptr && RuntimeExecutor->MoveToChildByIndex(ChildIndex);
    SyncReplicatedStateFromExecutor();
    return bMoved;
}

bool UDreamFlowExecutorComponent::MoveToOutputPinLocal(FName OutputPinName)
{
    UDreamFlowExecutor* RuntimeExecutor = GetOrCreateExecutor(false);
    const bool bMoved = RuntimeExecutor != nullptr && RuntimeExecutor->MoveToOutputPin(OutputPinName);
    SyncReplicatedStateFromExecutor();
    return bMoved;
}

bool UDreamFlowExecutorComponent::ChooseChildByGuidLocal(const FGuid& ChildNodeGuid)
{
    if (!ChildNodeGuid.IsValid() || FlowAsset == nullptr)
    {
        return false;
    }

    UDreamFlowExecutor* RuntimeExecutor = GetOrCreateExecutor(false);
    UDreamFlowNode* ChildNode = FlowAsset->FindNodeByGuid(ChildNodeGuid);
    const bool bChosen = RuntimeExecutor != nullptr && ChildNode != nullptr && RuntimeExecutor->ChooseChild(ChildNode);
    SyncReplicatedStateFromExecutor();
    return bChosen;
}

bool UDreamFlowExecutorComponent::SetVariableValueLocal(FName VariableName, const FDreamFlowValue& InValue)
{
    UDreamFlowExecutor* RuntimeExecutor = GetOrCreateExecutor(true);
    const bool bSet = RuntimeExecutor != nullptr && RuntimeExecutor->SetVariableValue(VariableName, InValue);
    SyncReplicatedStateFromExecutor();
    return bSet;
}

void UDreamFlowExecutorComponent::ResetVariablesToDefaultsLocal()
{
    if (UDreamFlowExecutor* RuntimeExecutor = GetOrCreateExecutor(true))
    {
        RuntimeExecutor->ResetVariablesToDefaults();
        SyncReplicatedStateFromExecutor();
    }
}

void UDreamFlowExecutorComponent::BroadcastReplicatedStateEvents(
    const FDreamFlowReplicatedExecutionState& PreviousState,
    const FDreamFlowReplicatedExecutionState& NewState)
{
    if (FlowAsset == nullptr)
    {
        return;
    }

    const bool bWasRunning =
        PreviousState.DebugState == EDreamFlowExecutorDebugState::Running
        || PreviousState.DebugState == EDreamFlowExecutorDebugState::Paused;
    const bool bIsRunning =
        NewState.DebugState == EDreamFlowExecutorDebugState::Running
        || NewState.DebugState == EDreamFlowExecutorDebugState::Paused;

    if (!bWasRunning && bIsRunning)
    {
        OnFlowStarted.Broadcast();
    }

    if (PreviousState.CurrentNodeGuid != NewState.CurrentNodeGuid)
    {
        if (UDreamFlowNode* PreviousNode = FlowAsset->FindNodeByGuid(PreviousState.CurrentNodeGuid))
        {
            OnNodeExited.Broadcast(PreviousNode);
        }

        if (UDreamFlowNode* CurrentNode = FlowAsset->FindNodeByGuid(NewState.CurrentNodeGuid))
        {
            OnNodeEntered.Broadcast(CurrentNode);
        }
    }

    if (bWasRunning && !bIsRunning)
    {
        OnFlowFinished.Broadcast();
    }
}

void UDreamFlowExecutorComponent::HandleExecutorRuntimeStateChanged(UDreamFlowExecutor* InExecutor)
{
    if (IsServerAuthority() && InExecutor == Executor)
    {
        SyncReplicatedStateFromExecutor();
    }
}
