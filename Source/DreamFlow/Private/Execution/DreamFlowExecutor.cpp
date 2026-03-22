#include "Execution/DreamFlowExecutor.h"

#include "DreamFlowAsset.h"
#include "DreamFlowLog.h"
#include "DreamFlowNode.h"
#include "Algo/Sort.h"
#include "Engine/Engine.h"
#include "Execution/DreamFlowAsyncContext.h"
#include "Execution/DreamFlowDebuggerSubsystem.h"
#include "UObject/UnrealType.h"

namespace DreamFlowExecutorPrivate
{
    struct FResolvedPropertyPath
    {
        FProperty* Property = nullptr;
        void* ValuePtr = nullptr;
        UObject* LeafObject = nullptr;
    };

    static bool TryResolvePropertyPath(UObject* RootObject, const FString& PropertyPath, FResolvedPropertyPath& OutResolvedPath)
    {
        OutResolvedPath = FResolvedPropertyPath();

        if (RootObject == nullptr || PropertyPath.IsEmpty())
        {
            return false;
        }

        TArray<FString> Segments;
        PropertyPath.ParseIntoArray(Segments, TEXT("."), true);
        if (Segments.Num() == 0)
        {
            return false;
        }

        UObject* CurrentObject = RootObject;
        UStruct* CurrentStruct = RootObject->GetClass();
        void* CurrentContainer = RootObject;

        for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
        {
            const FName SegmentName(*Segments[SegmentIndex]);
            FProperty* Property = FindFProperty<FProperty>(CurrentStruct, SegmentName);
            if (Property == nullptr)
            {
                return false;
            }

            void* PropertyValuePtr = Property->ContainerPtrToValuePtr<void>(CurrentContainer);
            const bool bIsLeaf = SegmentIndex == Segments.Num() - 1;
            if (bIsLeaf)
            {
                OutResolvedPath.Property = Property;
                OutResolvedPath.ValuePtr = PropertyValuePtr;
                OutResolvedPath.LeafObject = CurrentObject;
                return true;
            }

            if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
            {
                CurrentStruct = StructProperty->Struct;
                CurrentContainer = PropertyValuePtr;
                continue;
            }

            if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
            {
                UObject* NextObject = ObjectProperty->GetObjectPropertyValue(PropertyValuePtr);
                if (NextObject == nullptr)
                {
                    return false;
                }

                CurrentObject = NextObject;
                CurrentStruct = NextObject->GetClass();
                CurrentContainer = NextObject;
                continue;
            }

            return false;
        }

        return false;
    }

    static bool TryReadPropertyValue(const FResolvedPropertyPath& ResolvedPath, FDreamFlowValue& OutValue)
    {
        if (ResolvedPath.Property == nullptr || ResolvedPath.ValuePtr == nullptr)
        {
            return false;
        }

        if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(ResolvedPath.Property))
        {
            OutValue.Type = EDreamFlowValueType::Bool;
            OutValue.BoolValue = BoolProperty->GetPropertyValue(ResolvedPath.ValuePtr);
            return true;
        }

        if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(ResolvedPath.Property))
        {
            if (NumericProperty->IsFloatingPoint())
            {
                OutValue.Type = EDreamFlowValueType::Float;
                OutValue.FloatValue = static_cast<float>(NumericProperty->GetFloatingPointPropertyValue(ResolvedPath.ValuePtr));
                return true;
            }

            OutValue.Type = EDreamFlowValueType::Int;
            OutValue.IntValue = static_cast<int32>(NumericProperty->GetSignedIntPropertyValue(ResolvedPath.ValuePtr));
            return true;
        }

        if (const FNameProperty* NameProperty = CastField<FNameProperty>(ResolvedPath.Property))
        {
            OutValue.Type = EDreamFlowValueType::Name;
            OutValue.NameValue = NameProperty->GetPropertyValue(ResolvedPath.ValuePtr);
            return true;
        }

        if (const FStrProperty* StringProperty = CastField<FStrProperty>(ResolvedPath.Property))
        {
            OutValue.Type = EDreamFlowValueType::String;
            OutValue.StringValue = StringProperty->GetPropertyValue(ResolvedPath.ValuePtr);
            return true;
        }

        if (const FTextProperty* TextProperty = CastField<FTextProperty>(ResolvedPath.Property))
        {
            OutValue.Type = EDreamFlowValueType::Text;
            OutValue.TextValue = TextProperty->GetPropertyValue(ResolvedPath.ValuePtr);
            return true;
        }

        if (const FStructProperty* StructProperty = CastField<FStructProperty>(ResolvedPath.Property))
        {
            if (StructProperty->Struct == FGameplayTag::StaticStruct())
            {
                OutValue.Type = EDreamFlowValueType::GameplayTag;
                OutValue.GameplayTagValue = *StructProperty->ContainerPtrToValuePtr<FGameplayTag>(ResolvedPath.ValuePtr, 0);
                return true;
            }
        }

        if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ResolvedPath.Property))
        {
            OutValue.Type = EDreamFlowValueType::Object;
            OutValue.ObjectValue = ObjectProperty->GetObjectPropertyValue(ResolvedPath.ValuePtr);
            return true;
        }

        return false;
    }

    static bool TryWritePropertyValue(const FResolvedPropertyPath& ResolvedPath, const FDreamFlowValue& InValue)
    {
        if (ResolvedPath.Property == nullptr || ResolvedPath.ValuePtr == nullptr)
        {
            return false;
        }

        if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(ResolvedPath.Property))
        {
            FDreamFlowValue ConvertedValue;
            if (!DreamFlowVariable::TryConvertValue(InValue, EDreamFlowValueType::Bool, ConvertedValue))
            {
                return false;
            }

            BoolProperty->SetPropertyValue(ResolvedPath.ValuePtr, ConvertedValue.BoolValue);
            return true;
        }

        if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(ResolvedPath.Property))
        {
            if (NumericProperty->IsFloatingPoint())
            {
                FDreamFlowValue ConvertedValue;
                if (!DreamFlowVariable::TryConvertValue(InValue, EDreamFlowValueType::Float, ConvertedValue))
                {
                    return false;
                }

                NumericProperty->SetFloatingPointPropertyValue(ResolvedPath.ValuePtr, ConvertedValue.FloatValue);
                return true;
            }

            FDreamFlowValue ConvertedValue;
            if (!DreamFlowVariable::TryConvertValue(InValue, EDreamFlowValueType::Int, ConvertedValue))
            {
                return false;
            }

            NumericProperty->SetIntPropertyValue(ResolvedPath.ValuePtr, static_cast<int64>(ConvertedValue.IntValue));
            return true;
        }

        if (FNameProperty* NameProperty = CastField<FNameProperty>(ResolvedPath.Property))
        {
            FDreamFlowValue ConvertedValue;
            if (!DreamFlowVariable::TryConvertValue(InValue, EDreamFlowValueType::Name, ConvertedValue))
            {
                return false;
            }

            NameProperty->SetPropertyValue(ResolvedPath.ValuePtr, ConvertedValue.NameValue);
            return true;
        }

        if (FStrProperty* StringProperty = CastField<FStrProperty>(ResolvedPath.Property))
        {
            FDreamFlowValue ConvertedValue;
            if (!DreamFlowVariable::TryConvertValue(InValue, EDreamFlowValueType::String, ConvertedValue))
            {
                return false;
            }

            StringProperty->SetPropertyValue(ResolvedPath.ValuePtr, ConvertedValue.StringValue);
            return true;
        }

        if (FTextProperty* TextProperty = CastField<FTextProperty>(ResolvedPath.Property))
        {
            FDreamFlowValue ConvertedValue;
            if (!DreamFlowVariable::TryConvertValue(InValue, EDreamFlowValueType::Text, ConvertedValue))
            {
                return false;
            }

            TextProperty->SetPropertyValue(ResolvedPath.ValuePtr, ConvertedValue.TextValue);
            return true;
        }

        if (FStructProperty* StructProperty = CastField<FStructProperty>(ResolvedPath.Property))
        {
            if (StructProperty->Struct == FGameplayTag::StaticStruct())
            {
                FDreamFlowValue ConvertedValue;
                if (!DreamFlowVariable::TryConvertValue(InValue, EDreamFlowValueType::GameplayTag, ConvertedValue))
                {
                    return false;
                }

                *StructProperty->ContainerPtrToValuePtr<FGameplayTag>(ResolvedPath.ValuePtr, 0) = ConvertedValue.GameplayTagValue;
                return true;
            }
        }

        if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ResolvedPath.Property))
        {
            FDreamFlowValue ConvertedValue;
            if (!DreamFlowVariable::TryConvertValue(InValue, EDreamFlowValueType::Object, ConvertedValue))
            {
                return false;
            }

            ObjectProperty->SetObjectPropertyValue(ResolvedPath.ValuePtr, ConvertedValue.ObjectValue.Get());
            return true;
        }

        return false;
    }

    static bool AreValuesEquivalent(const FDreamFlowValue& A, const FDreamFlowValue& B)
    {
        bool bIsEqual = false;
        return DreamFlowVariable::TryCompareValues(A, B, EDreamFlowComparisonOperation::Equal, bIsEqual) && bIsEqual;
    }

    static bool DoPropertyPathsOverlap(const FString& A, const FString& B)
    {
        if (A.IsEmpty() || B.IsEmpty())
        {
            return true;
        }

        return A == B
            || A.StartsWith(B + TEXT("."))
            || B.StartsWith(A + TEXT("."));
    }

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

    static bool TryGetConvertedBindingValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, const EDreamFlowValueType TargetType, FDreamFlowValue& OutValue)
    {
        if (Executor == nullptr)
        {
            return false;
        }

        FDreamFlowValue ResolvedValue;
        if (!Executor->ResolveBindingValue(Binding, ResolvedValue))
        {
            return false;
        }

        return DreamFlowVariable::TryConvertValue(ResolvedValue, TargetType, OutValue);
    }

    static bool TryGetConvertedNodeStateValue(const UDreamFlowExecutor* Executor, const FGuid NodeGuid, const FName StateKey, const EDreamFlowValueType TargetType, FDreamFlowValue& OutValue)
    {
        if (Executor == nullptr)
        {
            return false;
        }

        FDreamFlowValue StoredValue;
        if (!Executor->GetNodeStateValue(NodeGuid, StateKey, StoredValue))
        {
            return false;
        }

        return DreamFlowVariable::TryConvertValue(StoredValue, TargetType, OutValue);
    }

    static void AppendReplicatedValues(const TMap<FName, FDreamFlowValue>& SourceValues, TArray<FDreamFlowReplicatedVariableValue>& OutValues)
    {
        TArray<FName> VariableNames;
        SourceValues.GenerateKeyArray(VariableNames);
        VariableNames.Sort(FNameLexicalLess());

        for (const FName VariableName : VariableNames)
        {
            if (const FDreamFlowValue* Value = SourceValues.Find(VariableName))
            {
                FDreamFlowReplicatedVariableValue& ReplicatedValue = OutValues.AddDefaulted_GetRef();
                ReplicatedValue.Name = VariableName;
                ReplicatedValue.Value = *Value;
            }
        }
    }

    static const FDreamFlowNodeRuntimeState* FindNodeRuntimeState(const TArray<FDreamFlowNodeRuntimeState>& SourceStates, const FGuid NodeGuid)
    {
        return SourceStates.FindByPredicate([NodeGuid](const FDreamFlowNodeRuntimeState& State)
        {
            return State.NodeGuid == NodeGuid;
        });
    }

    static FDreamFlowNodeRuntimeState* FindOrAddNodeRuntimeState(TArray<FDreamFlowNodeRuntimeState>& SourceStates, const FGuid NodeGuid)
    {
        if (FDreamFlowNodeRuntimeState* ExistingState = SourceStates.FindByPredicate([NodeGuid](const FDreamFlowNodeRuntimeState& State)
        {
            return State.NodeGuid == NodeGuid;
        }))
        {
            return ExistingState;
        }

        FDreamFlowNodeRuntimeState& NewState = SourceStates.AddDefaulted_GetRef();
        NewState.NodeGuid = NodeGuid;
        return &NewState;
    }

    static const FDreamFlowReplicatedVariableValue* FindReplicatedValue(const TArray<FDreamFlowReplicatedVariableValue>& SourceValues, const FName ValueName)
    {
        return SourceValues.FindByPredicate([ValueName](const FDreamFlowReplicatedVariableValue& Value)
        {
            return Value.Name == ValueName;
        });
    }

    static FDreamFlowReplicatedVariableValue* FindOrAddReplicatedValue(TArray<FDreamFlowReplicatedVariableValue>& SourceValues, const FName ValueName)
    {
        if (FDreamFlowReplicatedVariableValue* ExistingValue = SourceValues.FindByPredicate([ValueName](const FDreamFlowReplicatedVariableValue& Value)
        {
            return Value.Name == ValueName;
        }))
        {
            return ExistingValue;
        }

        FDreamFlowReplicatedVariableValue& NewValue = SourceValues.AddDefaulted_GetRef();
        NewValue.Name = ValueName;
        return &NewValue;
    }

    static void SortNodeRuntimeStates(TArray<FDreamFlowNodeRuntimeState>& InOutStates)
    {
        InOutStates.Sort([](const FDreamFlowNodeRuntimeState& A, const FDreamFlowNodeRuntimeState& B)
        {
            return A.NodeGuid.ToString(EGuidFormats::DigitsWithHyphens) < B.NodeGuid.ToString(EGuidFormats::DigitsWithHyphens);
        });

        for (FDreamFlowNodeRuntimeState& State : InOutStates)
        {
            State.Values.Sort([](const FDreamFlowReplicatedVariableValue& A, const FDreamFlowReplicatedVariableValue& B)
            {
                return FNameLexicalLess()(A.Name, B.Name);
            });
        }
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

bool UDreamFlowExecutor::HasNodeStateValue(FGuid NodeGuid, FName StateKey) const
{
    const FDreamFlowNodeRuntimeState* NodeState = DreamFlowExecutorPrivate::FindNodeRuntimeState(NodeRuntimeStates, NodeGuid);
    return NodeState != nullptr && !StateKey.IsNone() && DreamFlowExecutorPrivate::FindReplicatedValue(NodeState->Values, StateKey) != nullptr;
}

bool UDreamFlowExecutor::GetNodeStateValue(FGuid NodeGuid, FName StateKey, FDreamFlowValue& OutValue) const
{
    const FDreamFlowNodeRuntimeState* NodeState = DreamFlowExecutorPrivate::FindNodeRuntimeState(NodeRuntimeStates, NodeGuid);
    if (NodeState == nullptr)
    {
        return false;
    }

    if (const FDreamFlowReplicatedVariableValue* StoredValue = DreamFlowExecutorPrivate::FindReplicatedValue(NodeState->Values, StateKey))
    {
        OutValue = StoredValue->Value;
        return true;
    }

    return false;
}

bool UDreamFlowExecutor::GetNodeStateBoolValue(FGuid NodeGuid, FName StateKey, bool& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedNodeStateValue(this, NodeGuid, StateKey, EDreamFlowValueType::Bool, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.BoolValue;
    return true;
}

bool UDreamFlowExecutor::GetNodeStateIntValue(FGuid NodeGuid, FName StateKey, int32& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedNodeStateValue(this, NodeGuid, StateKey, EDreamFlowValueType::Int, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.IntValue;
    return true;
}

bool UDreamFlowExecutor::GetNodeStateFloatValue(FGuid NodeGuid, FName StateKey, float& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedNodeStateValue(this, NodeGuid, StateKey, EDreamFlowValueType::Float, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.FloatValue;
    return true;
}

bool UDreamFlowExecutor::GetNodeStateNameValue(FGuid NodeGuid, FName StateKey, FName& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedNodeStateValue(this, NodeGuid, StateKey, EDreamFlowValueType::Name, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.NameValue;
    return true;
}

bool UDreamFlowExecutor::GetNodeStateStringValue(FGuid NodeGuid, FName StateKey, FString& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedNodeStateValue(this, NodeGuid, StateKey, EDreamFlowValueType::String, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.StringValue;
    return true;
}

bool UDreamFlowExecutor::GetNodeStateTextValue(FGuid NodeGuid, FName StateKey, FText& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedNodeStateValue(this, NodeGuid, StateKey, EDreamFlowValueType::Text, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.TextValue;
    return true;
}

bool UDreamFlowExecutor::GetNodeStateGameplayTagValue(FGuid NodeGuid, FName StateKey, FGameplayTag& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedNodeStateValue(this, NodeGuid, StateKey, EDreamFlowValueType::GameplayTag, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.GameplayTagValue;
    return true;
}

bool UDreamFlowExecutor::GetNodeStateObjectValue(FGuid NodeGuid, FName StateKey, UObject*& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedNodeStateValue(this, NodeGuid, StateKey, EDreamFlowValueType::Object, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.ObjectValue.Get();
    return true;
}

bool UDreamFlowExecutor::SetNodeStateValue(FGuid NodeGuid, FName StateKey, const FDreamFlowValue& InValue)
{
    if (!NodeGuid.IsValid() || StateKey.IsNone())
    {
        return false;
    }

    FDreamFlowNodeRuntimeState* NodeState = DreamFlowExecutorPrivate::FindOrAddNodeRuntimeState(NodeRuntimeStates, NodeGuid);
    FDreamFlowReplicatedVariableValue* StateValue = DreamFlowExecutorPrivate::FindOrAddReplicatedValue(NodeState->Values, StateKey);
    StateValue->Value = InValue;
    DreamFlowExecutorPrivate::SortNodeRuntimeStates(NodeRuntimeStates);
    CachedSubFlowStack.Reset();
    NotifyDebuggerStateChanged();
    return true;
}

bool UDreamFlowExecutor::SetNodeStateBoolValue(FGuid NodeGuid, FName StateKey, bool InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Bool;
    Value.BoolValue = InValue;
    return SetNodeStateValue(NodeGuid, StateKey, Value);
}

bool UDreamFlowExecutor::SetNodeStateIntValue(FGuid NodeGuid, FName StateKey, int32 InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Int;
    Value.IntValue = InValue;
    return SetNodeStateValue(NodeGuid, StateKey, Value);
}

bool UDreamFlowExecutor::SetNodeStateFloatValue(FGuid NodeGuid, FName StateKey, float InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Float;
    Value.FloatValue = InValue;
    return SetNodeStateValue(NodeGuid, StateKey, Value);
}

bool UDreamFlowExecutor::SetNodeStateNameValue(FGuid NodeGuid, FName StateKey, FName InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Name;
    Value.NameValue = InValue;
    return SetNodeStateValue(NodeGuid, StateKey, Value);
}

bool UDreamFlowExecutor::SetNodeStateStringValue(FGuid NodeGuid, FName StateKey, const FString& InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::String;
    Value.StringValue = InValue;
    return SetNodeStateValue(NodeGuid, StateKey, Value);
}

bool UDreamFlowExecutor::SetNodeStateTextValue(FGuid NodeGuid, FName StateKey, const FText& InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Text;
    Value.TextValue = InValue;
    return SetNodeStateValue(NodeGuid, StateKey, Value);
}

bool UDreamFlowExecutor::SetNodeStateGameplayTagValue(FGuid NodeGuid, FName StateKey, FGameplayTag InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::GameplayTag;
    Value.GameplayTagValue = InValue;
    return SetNodeStateValue(NodeGuid, StateKey, Value);
}

bool UDreamFlowExecutor::SetNodeStateObjectValue(FGuid NodeGuid, FName StateKey, UObject* InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Object;
    Value.ObjectValue = InValue;
    return SetNodeStateValue(NodeGuid, StateKey, Value);
}

void UDreamFlowExecutor::ResetNodeState(FGuid NodeGuid)
{
    const int32 RemovedCount = NodeRuntimeStates.RemoveAll([NodeGuid](const FDreamFlowNodeRuntimeState& State)
    {
        return State.NodeGuid == NodeGuid;
    });

    if (RemovedCount > 0)
    {
        NotifyDebuggerStateChanged();
    }
}

void UDreamFlowExecutor::ResetAllNodeStates()
{
    if (NodeRuntimeStates.Num() > 0)
    {
        NodeRuntimeStates.Reset();
        NotifyDebuggerStateChanged();
    }
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
    TArray<TObjectPtr<UDreamFlowExecutor>> ActiveChildren;
    ActiveChildExecutorsByNodeGuid.GenerateValueArray(ActiveChildren);
    for (UDreamFlowExecutor* ChildExecutor : ActiveChildren)
    {
        if (ChildExecutor != nullptr && ChildExecutor->IsRunning())
        {
            ChildExecutor->FinishFlow();
        }
    }

    ActiveChildExecutorsByNodeGuid.Reset();
    CachedSubFlowStack.Reset();
    PersistentRuntimeObjectsByNodeGuid.Reset();
    DetachFromParentExecutor();

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
    if (ExecutionContext == InExecutionContext)
    {
        return;
    }

    ExecutionContext = InExecutionContext;
    NotifyExecutionContextChanged();
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

UDreamFlowExecutor* UDreamFlowExecutor::GetParentExecutor() const
{
    return ParentExecutor;
}

UDreamFlowExecutor* UDreamFlowExecutor::GetChildExecutorForNode(const UDreamFlowNode* Node) const
{
    if (Node == nullptr || !Node->NodeGuid.IsValid())
    {
        return nullptr;
    }

    const TObjectPtr<UDreamFlowExecutor>* ChildExecutor = ActiveChildExecutorsByNodeGuid.Find(Node->NodeGuid);
    return ChildExecutor != nullptr ? ChildExecutor->Get() : nullptr;
}

UDreamFlowExecutor* UDreamFlowExecutor::GetCurrentChildExecutor() const
{
    if (CurrentNode != nullptr)
    {
        if (UDreamFlowExecutor* CurrentChild = GetChildExecutorForNode(CurrentNode))
        {
            return CurrentChild;
        }
    }

    for (const TPair<FGuid, TObjectPtr<UDreamFlowExecutor>>& Pair : ActiveChildExecutorsByNodeGuid)
    {
        if (Pair.Value != nullptr)
        {
            return Pair.Value.Get();
        }
    }

    return nullptr;
}

TArray<UDreamFlowExecutor*> UDreamFlowExecutor::GetActiveChildExecutors() const
{
    TArray<UDreamFlowExecutor*> Result;
    Result.Reserve(ActiveChildExecutorsByNodeGuid.Num());

    for (const TPair<FGuid, TObjectPtr<UDreamFlowExecutor>>& Pair : ActiveChildExecutorsByNodeGuid)
    {
        if (Pair.Value != nullptr)
        {
            Result.Add(Pair.Value.Get());
        }
    }

    return Result;
}

TArray<FDreamFlowSubFlowStackEntry> UDreamFlowExecutor::GetActiveSubFlowStack() const
{
    if (ActiveChildExecutorsByNodeGuid.Num() == 0)
    {
        return CachedSubFlowStack;
    }

    TArray<FDreamFlowSubFlowStackEntry> Result;
    TArray<FGuid> OwnerNodeGuids;
    ActiveChildExecutorsByNodeGuid.GenerateKeyArray(OwnerNodeGuids);
    OwnerNodeGuids.Sort([](const FGuid& A, const FGuid& B)
    {
        return A.ToString(EGuidFormats::DigitsWithHyphens) < B.ToString(EGuidFormats::DigitsWithHyphens);
    });

    for (const FGuid& OwnerNodeGuid : OwnerNodeGuids)
    {
        const TObjectPtr<UDreamFlowExecutor>* ChildExecutorPtr = ActiveChildExecutorsByNodeGuid.Find(OwnerNodeGuid);
        if (ChildExecutorPtr == nullptr || *ChildExecutorPtr == nullptr)
        {
            continue;
        }

        UDreamFlowExecutor* ChildExecutor = ChildExecutorPtr->Get();
        if (ChildExecutor == nullptr || ChildExecutor->GetFlowAsset() == FlowAsset)
        {
            continue;
        }

        FDreamFlowSubFlowStackEntry& Entry = Result.AddDefaulted_GetRef();
        Entry.OwnerNodeGuid = OwnerNodeGuid;
        Entry.FlowAsset = ChildExecutor->GetFlowAsset();
        Entry.CurrentNodeGuid = ChildExecutor->GetCurrentNode() != nullptr
            ? ChildExecutor->GetCurrentNode()->NodeGuid
            : FGuid();

        Result.Append(ChildExecutor->GetActiveSubFlowStack());
    }

    return Result;
}

bool UDreamFlowExecutor::GetExecutionContextPropertyValue(const FString& PropertyPath, FDreamFlowValue& OutValue) const
{
    DreamFlowExecutorPrivate::FResolvedPropertyPath ResolvedPath;
    return DreamFlowExecutorPrivate::TryResolvePropertyPath(ExecutionContext, PropertyPath, ResolvedPath)
        && DreamFlowExecutorPrivate::TryReadPropertyValue(ResolvedPath, OutValue);
}

bool UDreamFlowExecutor::SetExecutionContextPropertyValue(const FString& PropertyPath, const FDreamFlowValue& InValue)
{
    DreamFlowExecutorPrivate::FResolvedPropertyPath ResolvedPath;
    FDreamFlowValue PreviousValue;
    const bool bHadPreviousValue =
        DreamFlowExecutorPrivate::TryResolvePropertyPath(ExecutionContext, PropertyPath, ResolvedPath)
        && DreamFlowExecutorPrivate::TryReadPropertyValue(ResolvedPath, PreviousValue);

    if (!bHadPreviousValue || !DreamFlowExecutorPrivate::TryWritePropertyValue(ResolvedPath, InValue))
    {
        return false;
    }

    FDreamFlowValue CurrentValue;
    if (!DreamFlowExecutorPrivate::TryReadPropertyValue(ResolvedPath, CurrentValue))
    {
        CurrentValue = PreviousValue;
    }

    if (!DreamFlowExecutorPrivate::AreValuesEquivalent(PreviousValue, CurrentValue))
    {
        ExecutionContextPropertyChangedNative.Broadcast(this, PropertyPath, CurrentValue);
    }

    NotifyDebuggerStateChanged();
    return true;
}

void UDreamFlowExecutor::NotifyExecutionContextChanged()
{
    NotifyDebuggerStateChanged();
}

bool UDreamFlowExecutor::NotifyExecutionContextPropertyChanged(const FString& PropertyPath)
{
    if (PropertyPath.IsEmpty())
    {
        NotifyExecutionContextChanged();
        return true;
    }

    FDreamFlowValue CurrentValue;
    if (!GetExecutionContextPropertyValue(PropertyPath, CurrentValue))
    {
        return false;
    }

    ExecutionContextPropertyChangedNative.Broadcast(this, PropertyPath, CurrentValue);
    NotifyDebuggerStateChanged();
    return true;
}

UDreamFlowNode* UDreamFlowExecutor::GetEntryNode() const
{
    return FlowAsset != nullptr ? FlowAsset->GetEntryNode() : nullptr;
}

TArray<UDreamFlowNode*> UDreamFlowExecutor::GetNodes() const
{
    return FlowAsset != nullptr ? FlowAsset->GetNodesCopy() : TArray<UDreamFlowNode*>();
}

UDreamFlowNode* UDreamFlowExecutor::FindNodeByGuid(FGuid NodeGuid) const
{
    return FlowAsset != nullptr ? FlowAsset->FindNodeByGuid(NodeGuid) : nullptr;
}

bool UDreamFlowExecutor::OwnsNode(const UDreamFlowNode* Node) const
{
    return FlowAsset != nullptr && FlowAsset->OwnsNode(Node);
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

bool UDreamFlowExecutor::IsWaitingForAdvance() const
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

FDreamFlowExecutionSnapshot UDreamFlowExecutor::BuildExecutionSnapshot() const
{
    FDreamFlowExecutionSnapshot Snapshot;
    Snapshot.DebugState = GetDebugState();
    Snapshot.CurrentNodeGuid = CurrentNode != nullptr ? CurrentNode->NodeGuid : FGuid();
    Snapshot.bPauseOnBreakpoints = bPauseOnBreakpoints;

    for (UDreamFlowNode* VisitedNode : VisitedNodes)
    {
        if (VisitedNode != nullptr && VisitedNode->NodeGuid.IsValid())
        {
            Snapshot.VisitedNodeGuids.Add(VisitedNode->NodeGuid);
        }
    }

    DreamFlowExecutorPrivate::AppendReplicatedValues(RuntimeVariables, Snapshot.Variables);
    Snapshot.NodeStates = NodeRuntimeStates;
    DreamFlowExecutorPrivate::SortNodeRuntimeStates(Snapshot.NodeStates);
    Snapshot.SubFlowStack = GetActiveSubFlowStack();
    return Snapshot;
}

void UDreamFlowExecutor::ApplyExecutionSnapshot(const FDreamFlowExecutionSnapshot& InSnapshot)
{
    FDreamFlowReplicatedExecutionState ReplicatedState;
    ReplicatedState.DebugState = InSnapshot.DebugState;
    ReplicatedState.CurrentNodeGuid = InSnapshot.CurrentNodeGuid;
    ReplicatedState.VisitedNodeGuids = InSnapshot.VisitedNodeGuids;
    ReplicatedState.Variables = InSnapshot.Variables;
    ReplicatedState.NodeStates = InSnapshot.NodeStates;
    ReplicatedState.SubFlowStack = InSnapshot.SubFlowStack;
    ReplicatedState.bPauseOnBreakpoints = InSnapshot.bPauseOnBreakpoints;
    ApplyReplicatedState(ReplicatedState);
}

void UDreamFlowExecutor::RegisterChildExecutor(UDreamFlowNode* OwnerNode, UDreamFlowExecutor* ChildExecutor)
{
    if (OwnerNode == nullptr || ChildExecutor == nullptr || !OwnerNode->NodeGuid.IsValid())
    {
        return;
    }

    ActiveChildExecutorsByNodeGuid.FindOrAdd(OwnerNode->NodeGuid) = ChildExecutor;
    ChildExecutor->DetachFromParentExecutor();
    ChildExecutor->ParentExecutor = this;
    ChildExecutor->ParentLinkNodeGuid = OwnerNode->NodeGuid;
    CachedSubFlowStack.Reset();
    NotifyDebuggerStateChanged();
}

void UDreamFlowExecutor::UnregisterChildExecutor(UDreamFlowNode* OwnerNode, UDreamFlowExecutor* ChildExecutor)
{
    if (OwnerNode == nullptr || !OwnerNode->NodeGuid.IsValid())
    {
        return;
    }

    const TObjectPtr<UDreamFlowExecutor>* RegisteredExecutor = ActiveChildExecutorsByNodeGuid.Find(OwnerNode->NodeGuid);
    if (RegisteredExecutor != nullptr && (ChildExecutor == nullptr || RegisteredExecutor->Get() == ChildExecutor))
    {
        if (RegisteredExecutor->Get() != nullptr && RegisteredExecutor->Get()->ParentExecutor == this)
        {
            RegisteredExecutor->Get()->ParentExecutor = nullptr;
            RegisteredExecutor->Get()->ParentLinkNodeGuid.Invalidate();
            RegisteredExecutor->Get()->bSharesRuntimeVariablesWithParent = false;
        }

        ActiveChildExecutorsByNodeGuid.Remove(OwnerNode->NodeGuid);
        CachedSubFlowStack.Reset();
        NotifyDebuggerStateChanged();
    }
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

    DreamFlowExecutorPrivate::AppendReplicatedValues(RuntimeVariables, OutState.Variables);
    OutState.NodeStates = NodeRuntimeStates;
    DreamFlowExecutorPrivate::SortNodeRuntimeStates(OutState.NodeStates);
    OutState.SubFlowStack = GetActiveSubFlowStack();

    DREAMFLOW_LOG(
        Replication,
        Verbose,
        "Built replicated execution state for flow '%s' with %d visited nodes, %d variables, %d node states and %d sub flow entries.",
        *GetNameSafe(FlowAsset),
        OutState.VisitedNodeGuids.Num(),
        OutState.Variables.Num(),
        OutState.NodeStates.Num(),
        OutState.SubFlowStack.Num());
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

    NodeRuntimeStates = InState.NodeStates;
    DreamFlowExecutorPrivate::SortNodeRuntimeStates(NodeRuntimeStates);
    ActiveChildExecutorsByNodeGuid.Reset();
    CachedSubFlowStack = InState.SubFlowStack;

    DREAMFLOW_LOG(
        Replication,
        Verbose,
        "Applied replicated execution state for flow '%s'. Current node='%s', variables=%d, node states=%d, cached sub flow entries=%d.",
        *GetNameSafe(FlowAsset),
        CurrentNode != nullptr ? *CurrentNode->GetNodeDisplayName().ToString() : TEXT("<none>"),
        RuntimeVariables.Num(),
        NodeRuntimeStates.Num(),
        CachedSubFlowStack.Num());
}

FDreamFlowExecutorRuntimeStateChangedNativeSignature& UDreamFlowExecutor::OnRuntimeStateChangedNative()
{
    return RuntimeStateChangedNative;
}

FDreamFlowExecutorVariableChangedNativeSignature& UDreamFlowExecutor::OnVariableChangedNative()
{
    return VariableChangedNative;
}

FDreamFlowExecutorExecutionContextPropertyChangedNativeSignature& UDreamFlowExecutor::OnExecutionContextPropertyChangedNative()
{
    return ExecutionContextPropertyChangedNative;
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

    const FDreamFlowValue* PreviousValue = RuntimeVariables.Find(VariableName);
    if (PreviousValue != nullptr && DreamFlowExecutorPrivate::AreValuesEquivalent(*PreviousValue, ConvertedValue))
    {
        return true;
    }

    const FString PreviousValueText = PreviousValue != nullptr ? PreviousValue->Describe() : TEXT("<unset>");
    RuntimeVariables.FindOrAdd(VariableName) = ConvertedValue;
    DREAMFLOW_LOG(Variables, Log, "Variable '%s' changed from '%s' to '%s' on flow '%s'.", *VariableName.ToString(), *PreviousValueText, *ConvertedValue.Describe(), *GetNameSafe(FlowAsset));
    VariableChangedNative.Broadcast(this, VariableName, ConvertedValue);

    if (ParentExecutor != nullptr && bSharesRuntimeVariablesWithParent && ParentExecutor->GetFlowAsset() == FlowAsset)
    {
        ParentExecutor->SetVariableValue(VariableName, ConvertedValue);
    }

    for (const TPair<FGuid, TObjectPtr<UDreamFlowExecutor>>& Pair : ActiveChildExecutorsByNodeGuid)
    {
        UDreamFlowExecutor* ChildExecutor = Pair.Value.Get();
        if (ChildExecutor != nullptr
            && ChildExecutor->bSharesRuntimeVariablesWithParent
            && ChildExecutor->GetFlowAsset() == FlowAsset)
        {
            ChildExecutor->SetVariableValue(VariableName, ConvertedValue);
        }
    }

    NotifyDebuggerStateChanged();
    return true;
}

bool UDreamFlowExecutor::ResolveBindingValue(const FDreamFlowValueBinding& Binding, FDreamFlowValue& OutValue) const
{
    if (Binding.SourceType == EDreamFlowValueSourceType::FlowVariable)
    {
        return GetVariableValue(Binding.VariableName, OutValue);
    }

    if (Binding.SourceType == EDreamFlowValueSourceType::ExecutionContextProperty)
    {
        return GetExecutionContextPropertyValue(Binding.PropertyPath, OutValue);
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

bool UDreamFlowExecutor::GetBindingVariableName(const FDreamFlowValueBinding& Binding, FName& OutVariableName) const
{
    OutVariableName = NAME_None;

    if (Binding.SourceType != EDreamFlowValueSourceType::FlowVariable || Binding.VariableName.IsNone())
    {
        return false;
    }

    OutVariableName = Binding.VariableName;
    return true;
}

bool UDreamFlowExecutor::CanWriteBindingValue(const FDreamFlowValueBinding& Binding) const
{
    FName VariableName = NAME_None;
    if (GetBindingVariableName(Binding, VariableName))
    {
        return FlowAsset != nullptr
            && FlowAsset->HasVariableDefinition(VariableName);
    }

    if (Binding.SourceType == EDreamFlowValueSourceType::ExecutionContextProperty)
    {
        DreamFlowExecutorPrivate::FResolvedPropertyPath ResolvedPath;
        return DreamFlowExecutorPrivate::TryResolvePropertyPath(ExecutionContext, Binding.PropertyPath, ResolvedPath);
    }

    return false;
}

bool UDreamFlowExecutor::GetBindingBoolValue(const FDreamFlowValueBinding& Binding, bool& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedBindingValue(this, Binding, EDreamFlowValueType::Bool, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.BoolValue;
    return true;
}

bool UDreamFlowExecutor::GetBindingIntValue(const FDreamFlowValueBinding& Binding, int32& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedBindingValue(this, Binding, EDreamFlowValueType::Int, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.IntValue;
    return true;
}

bool UDreamFlowExecutor::GetBindingFloatValue(const FDreamFlowValueBinding& Binding, float& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedBindingValue(this, Binding, EDreamFlowValueType::Float, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.FloatValue;
    return true;
}

bool UDreamFlowExecutor::GetBindingNameValue(const FDreamFlowValueBinding& Binding, FName& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedBindingValue(this, Binding, EDreamFlowValueType::Name, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.NameValue;
    return true;
}

bool UDreamFlowExecutor::GetBindingStringValue(const FDreamFlowValueBinding& Binding, FString& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedBindingValue(this, Binding, EDreamFlowValueType::String, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.StringValue;
    return true;
}

bool UDreamFlowExecutor::GetBindingTextValue(const FDreamFlowValueBinding& Binding, FText& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedBindingValue(this, Binding, EDreamFlowValueType::Text, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.TextValue;
    return true;
}

bool UDreamFlowExecutor::GetBindingGameplayTagValue(const FDreamFlowValueBinding& Binding, FGameplayTag& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedBindingValue(this, Binding, EDreamFlowValueType::GameplayTag, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.GameplayTagValue;
    return true;
}

bool UDreamFlowExecutor::GetBindingObjectValue(const FDreamFlowValueBinding& Binding, UObject*& OutValue) const
{
    FDreamFlowValue ConvertedValue;
    if (!DreamFlowExecutorPrivate::TryGetConvertedBindingValue(this, Binding, EDreamFlowValueType::Object, ConvertedValue))
    {
        return false;
    }

    OutValue = ConvertedValue.ObjectValue.Get();
    return true;
}

bool UDreamFlowExecutor::SetBindingBoolValue(const FDreamFlowValueBinding& Binding, bool InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Bool;
    Value.BoolValue = InValue;
    return SetBindingValue(Binding, Value);
}

bool UDreamFlowExecutor::SetBindingIntValue(const FDreamFlowValueBinding& Binding, int32 InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Int;
    Value.IntValue = InValue;
    return SetBindingValue(Binding, Value);
}

bool UDreamFlowExecutor::SetBindingFloatValue(const FDreamFlowValueBinding& Binding, float InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Float;
    Value.FloatValue = InValue;
    return SetBindingValue(Binding, Value);
}

bool UDreamFlowExecutor::SetBindingNameValue(const FDreamFlowValueBinding& Binding, FName InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Name;
    Value.NameValue = InValue;
    return SetBindingValue(Binding, Value);
}

bool UDreamFlowExecutor::SetBindingStringValue(const FDreamFlowValueBinding& Binding, const FString& InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::String;
    Value.StringValue = InValue;
    return SetBindingValue(Binding, Value);
}

bool UDreamFlowExecutor::SetBindingTextValue(const FDreamFlowValueBinding& Binding, const FText& InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Text;
    Value.TextValue = InValue;
    return SetBindingValue(Binding, Value);
}

bool UDreamFlowExecutor::SetBindingGameplayTagValue(const FDreamFlowValueBinding& Binding, FGameplayTag InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::GameplayTag;
    Value.GameplayTagValue = InValue;
    return SetBindingValue(Binding, Value);
}

bool UDreamFlowExecutor::SetBindingObjectValue(const FDreamFlowValueBinding& Binding, UObject* InValue)
{
    FDreamFlowValue Value;
    Value.Type = EDreamFlowValueType::Object;
    Value.ObjectValue = InValue;
    return SetBindingValue(Binding, Value);
}

bool UDreamFlowExecutor::SetBindingValue(const FDreamFlowValueBinding& Binding, const FDreamFlowValue& InValue)
{
    FName VariableName = NAME_None;
    if (GetBindingVariableName(Binding, VariableName))
    {
        return SetVariableValue(VariableName, InValue);
    }

    if (Binding.SourceType == EDreamFlowValueSourceType::ExecutionContextProperty)
    {
        return SetExecutionContextPropertyValue(Binding.PropertyPath, InValue);
    }

    DREAMFLOW_LOG(
        Variables,
        Warning,
        "Failed to write to binding on flow '%s' because only flow-variable and execution-context-property bindings are writable.",
        *GetNameSafe(FlowAsset));
    return false;
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
    if (ResolvedOutputPinName.IsNone() && !bCanFinishWithoutTransition)
    {
        return false;
    }

    if (!ResolvedOutputPinName.IsNone())
    {
        if (AsyncNode->GetFirstChildForOutputPin(ResolvedOutputPinName) != nullptr)
        {
            ClearAsyncExecutionState();
            DREAMFLOW_LOG(Execution, Log, "Async node '%s' completed through output '%s'.", *AsyncNode->GetNodeDisplayName().ToString(), *ResolvedOutputPinName.ToString());
            return EnterNode(AsyncNode->GetFirstChildForOutputPin(ResolvedOutputPinName));
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

UDreamFlowExecutor* UDreamFlowExecutor::StartParallelBranchAtNode(UDreamFlowNode* StartNode)
{
    return StartParallelBranchAtNodeWithOwnerKey(StartNode, StartNode != nullptr ? StartNode->NodeGuid : FGuid());
}

UDreamFlowExecutor* UDreamFlowExecutor::StartParallelBranchAtNodeWithOwnerKey(UDreamFlowNode* StartNode, FGuid OwnerKey)
{
    if (FlowAsset == nullptr || StartNode == nullptr || !FlowAsset->OwnsNode(StartNode))
    {
        return nullptr;
    }

    if (!OwnerKey.IsValid())
    {
        OwnerKey = StartNode->NodeGuid;
    }

    UClass* EffectiveClass = GetClass();
    UDreamFlowExecutor* BranchExecutor = NewObject<UDreamFlowExecutor>(this, EffectiveClass, NAME_None, RF_Transient);
    if (BranchExecutor == nullptr)
    {
        return nullptr;
    }

    BranchExecutor->Initialize(FlowAsset, ExecutionContext);
    BranchExecutor->RuntimeVariables = RuntimeVariables;
    BranchExecutor->NodeRuntimeStates = NodeRuntimeStates;
    BranchExecutor->bPauseOnBreakpoints = bPauseOnBreakpoints;

    if (OwnerKey == StartNode->NodeGuid)
    {
        RegisterChildExecutor(StartNode, BranchExecutor);
    }
    else
    {
        ActiveChildExecutorsByNodeGuid.FindOrAdd(OwnerKey) = BranchExecutor;
        BranchExecutor->DetachFromParentExecutor();
        BranchExecutor->ParentExecutor = this;
        BranchExecutor->ParentLinkNodeGuid = OwnerKey;
        CachedSubFlowStack.Reset();
        NotifyDebuggerStateChanged();
    }
    BranchExecutor->bSharesRuntimeVariablesWithParent = true;

    BranchExecutor->bIsRunning = true;
    BranchExecutor->bIsPaused = false;
    BranchExecutor->bBreakOnNextNode = false;
    BranchExecutor->bHasFinished = false;
    BranchExecutor->RegisterWithDebugger();
    BranchExecutor->OnFlowStarted.Broadcast();
    BranchExecutor->BroadcastDebugStateChanged();

    if (!BranchExecutor->ActivateNode(StartNode, true))
    {
        BranchExecutor->FinishFlow();
        return nullptr;
    }

    return BranchExecutor;
}

UObject* UDreamFlowExecutor::GetPersistentRuntimeObject(FGuid OwnerNodeGuid) const
{
    if (!OwnerNodeGuid.IsValid())
    {
        return nullptr;
    }

    const TObjectPtr<UObject>* RuntimeObject = PersistentRuntimeObjectsByNodeGuid.Find(OwnerNodeGuid);
    return RuntimeObject != nullptr ? RuntimeObject->Get() : nullptr;
}

void UDreamFlowExecutor::SetPersistentRuntimeObject(FGuid OwnerNodeGuid, UObject* RuntimeObject)
{
    if (!OwnerNodeGuid.IsValid())
    {
        return;
    }

    if (RuntimeObject == nullptr)
    {
        PersistentRuntimeObjectsByNodeGuid.Remove(OwnerNodeGuid);
        return;
    }

    PersistentRuntimeObjectsByNodeGuid.FindOrAdd(OwnerNodeGuid) = RuntimeObject;
}

void UDreamFlowExecutor::ClearPersistentRuntimeObject(FGuid OwnerNodeGuid)
{
    if (!OwnerNodeGuid.IsValid())
    {
        return;
    }

    PersistentRuntimeObjectsByNodeGuid.Remove(OwnerNodeGuid);
}

void UDreamFlowExecutor::DetachFromParentExecutor()
{
    if (ParentExecutor != nullptr && ParentLinkNodeGuid.IsValid())
    {
        const TObjectPtr<UDreamFlowExecutor>* RegisteredExecutor = ParentExecutor->ActiveChildExecutorsByNodeGuid.Find(ParentLinkNodeGuid);
        if (RegisteredExecutor != nullptr && RegisteredExecutor->Get() == this)
        {
            ParentExecutor->ActiveChildExecutorsByNodeGuid.Remove(ParentLinkNodeGuid);
            ParentExecutor->CachedSubFlowStack.Reset();
            ParentExecutor->NotifyDebuggerStateChanged();
        }
    }

    ParentExecutor = nullptr;
    ParentLinkNodeGuid.Invalidate();
    bSharesRuntimeVariablesWithParent = false;
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
    NodeRuntimeStates.Reset();
    ClearAsyncExecutionState();
    bIsRunning = false;
    bIsPaused = false;
    bPauseOnBreakpoints = true;
    bBreakOnNextNode = false;
    bHasFinished = false;
    ParentExecutor = nullptr;
    ActiveChildExecutorsByNodeGuid.Reset();
    ParentLinkNodeGuid.Invalidate();
    bSharesRuntimeVariablesWithParent = false;
    CachedSubFlowStack.Reset();
    PersistentRuntimeObjectsByNodeGuid.Reset();

    ResetVariablesToDefaults();

    if (bNotifyDebugger)
    {
        BroadcastDebugStateChanged();
    }
}
