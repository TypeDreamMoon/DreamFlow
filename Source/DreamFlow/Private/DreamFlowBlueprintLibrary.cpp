#include "DreamFlowBlueprintLibrary.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "Execution/DreamFlowExecutor.h"
#include "UObject/UObjectGlobals.h"

UDreamFlowExecutor* UDreamFlowBlueprintLibrary::CreateFlowExecutor(
    UObject* WorldContextObject,
    UDreamFlowAsset* FlowAsset,
    UObject* ExecutionContext,
    TSubclassOf<UDreamFlowExecutor> ExecutorClass)
{
    UObject* Outer = WorldContextObject != nullptr ? WorldContextObject : GetTransientPackage();
    UClass* EffectiveExecutorClass = ExecutorClass != nullptr ? ExecutorClass.Get() : UDreamFlowExecutor::StaticClass();
    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(Outer, EffectiveExecutorClass);
    if (Executor != nullptr)
    {
        Executor->Initialize(FlowAsset, ExecutionContext != nullptr ? ExecutionContext : WorldContextObject);
    }

    return Executor;
}

UDreamFlowNode* UDreamFlowBlueprintLibrary::GetEntryNode(const UDreamFlowAsset* FlowAsset)
{
    return FlowAsset ? FlowAsset->GetEntryNode() : nullptr;
}

TArray<UDreamFlowNode*> UDreamFlowBlueprintLibrary::GetNodes(const UDreamFlowAsset* FlowAsset)
{
    return FlowAsset ? FlowAsset->GetNodesCopy() : TArray<UDreamFlowNode*>();
}

TArray<UDreamFlowNode*> UDreamFlowBlueprintLibrary::GetChildren(const UDreamFlowNode* FlowNode)
{
    return FlowNode ? FlowNode->GetChildrenCopy() : TArray<UDreamFlowNode*>();
}

TArray<FDreamFlowNodeOutputPin> UDreamFlowBlueprintLibrary::GetNodeOutputPins(const UDreamFlowNode* FlowNode)
{
    return FlowNode ? FlowNode->GetOutputPins() : TArray<FDreamFlowNodeOutputPin>();
}

TArray<FDreamFlowNodeOutputLink> UDreamFlowBlueprintLibrary::GetNodeOutputLinks(const UDreamFlowNode* FlowNode)
{
    return FlowNode ? FlowNode->GetOutputLinksCopy() : TArray<FDreamFlowNodeOutputLink>();
}

UDreamFlowNode* UDreamFlowBlueprintLibrary::GetFirstChildForOutputPin(const UDreamFlowNode* FlowNode, FName OutputPinName)
{
    return FlowNode ? FlowNode->GetFirstChildForOutputPin(OutputPinName) : nullptr;
}

FText UDreamFlowBlueprintLibrary::GetNodeTitle(const UDreamFlowNode* FlowNode)
{
    return FlowNode ? FlowNode->GetNodeDisplayName() : FText::GetEmpty();
}

FText UDreamFlowBlueprintLibrary::GetNodeCategory(const UDreamFlowNode* FlowNode)
{
    return FlowNode ? FlowNode->GetNodeCategory() : FText::GetEmpty();
}

UDreamFlowNode* UDreamFlowBlueprintLibrary::FindNodeByGuid(const UDreamFlowAsset* FlowAsset, FGuid NodeGuid)
{
    return FlowAsset ? FlowAsset->FindNodeByGuid(NodeGuid) : nullptr;
}

TArray<FDreamFlowVariableDefinition> UDreamFlowBlueprintLibrary::GetFlowVariables(const UDreamFlowAsset* FlowAsset)
{
    return FlowAsset ? FlowAsset->GetVariablesCopy() : TArray<FDreamFlowVariableDefinition>();
}

bool UDreamFlowBlueprintLibrary::HasFlowVariableDefinition(const UDreamFlowAsset* FlowAsset, FName VariableName)
{
    return FlowAsset != nullptr && FlowAsset->HasVariableDefinition(VariableName);
}

bool UDreamFlowBlueprintLibrary::FindFlowVariableDefinition(const UDreamFlowAsset* FlowAsset, FName VariableName, FDreamFlowVariableDefinition& OutDefinition)
{
    if (FlowAsset == nullptr)
    {
        return false;
    }

    if (const FDreamFlowVariableDefinition* VariableDefinition = FlowAsset->FindVariableDefinition(VariableName))
    {
        OutDefinition = *VariableDefinition;
        return true;
    }

    return false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBoolVariable(const UDreamFlowExecutor* Executor, FName VariableName, bool& OutValue)
{
    return Executor ? Executor->GetVariableBoolValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorIntVariable(const UDreamFlowExecutor* Executor, FName VariableName, int32& OutValue)
{
    return Executor ? Executor->GetVariableIntValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorFloatVariable(const UDreamFlowExecutor* Executor, FName VariableName, float& OutValue)
{
    return Executor ? Executor->GetVariableFloatValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorNameVariable(const UDreamFlowExecutor* Executor, FName VariableName, FName& OutValue)
{
    return Executor ? Executor->GetVariableNameValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorStringVariable(const UDreamFlowExecutor* Executor, FName VariableName, FString& OutValue)
{
    return Executor ? Executor->GetVariableStringValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorTextVariable(const UDreamFlowExecutor* Executor, FName VariableName, FText& OutValue)
{
    return Executor ? Executor->GetVariableTextValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorGameplayTagVariable(const UDreamFlowExecutor* Executor, FName VariableName, FGameplayTag& OutValue)
{
    return Executor ? Executor->GetVariableGameplayTagValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorObjectVariable(const UDreamFlowExecutor* Executor, FName VariableName, UObject*& OutValue)
{
    return Executor ? Executor->GetVariableObjectValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorVariable(const UDreamFlowExecutor* Executor, FName VariableName, FDreamFlowValue& OutValue)
{
    return Executor ? Executor->GetVariableValue(VariableName, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBoolVariable(UDreamFlowExecutor* Executor, FName VariableName, bool InValue)
{
    return Executor ? Executor->SetVariableBoolValue(VariableName, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorIntVariable(UDreamFlowExecutor* Executor, FName VariableName, int32 InValue)
{
    return Executor ? Executor->SetVariableIntValue(VariableName, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorFloatVariable(UDreamFlowExecutor* Executor, FName VariableName, float InValue)
{
    return Executor ? Executor->SetVariableFloatValue(VariableName, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorNameVariable(UDreamFlowExecutor* Executor, FName VariableName, FName InValue)
{
    return Executor ? Executor->SetVariableNameValue(VariableName, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorStringVariable(UDreamFlowExecutor* Executor, FName VariableName, const FString& InValue)
{
    return Executor ? Executor->SetVariableStringValue(VariableName, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorTextVariable(UDreamFlowExecutor* Executor, FName VariableName, const FText& InValue)
{
    return Executor ? Executor->SetVariableTextValue(VariableName, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorGameplayTagVariable(UDreamFlowExecutor* Executor, FName VariableName, FGameplayTag InValue)
{
    return Executor ? Executor->SetVariableGameplayTagValue(VariableName, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorObjectVariable(UDreamFlowExecutor* Executor, FName VariableName, UObject* InValue)
{
    return Executor ? Executor->SetVariableObjectValue(VariableName, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorVariable(UDreamFlowExecutor* Executor, FName VariableName, const FDreamFlowValue& InValue)
{
    return Executor ? Executor->SetVariableValue(VariableName, InValue) : false;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::MakeBoolFlowValue(bool Value)
{
    FDreamFlowValue FlowValue;
    FlowValue.Type = EDreamFlowValueType::Bool;
    FlowValue.BoolValue = Value;
    return FlowValue;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::MakeIntFlowValue(int32 Value)
{
    FDreamFlowValue FlowValue;
    FlowValue.Type = EDreamFlowValueType::Int;
    FlowValue.IntValue = Value;
    return FlowValue;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::MakeFloatFlowValue(float Value)
{
    FDreamFlowValue FlowValue;
    FlowValue.Type = EDreamFlowValueType::Float;
    FlowValue.FloatValue = Value;
    return FlowValue;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::MakeNameFlowValue(FName Value)
{
    FDreamFlowValue FlowValue;
    FlowValue.Type = EDreamFlowValueType::Name;
    FlowValue.NameValue = Value;
    return FlowValue;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::MakeStringFlowValue(const FString& Value)
{
    FDreamFlowValue FlowValue;
    FlowValue.Type = EDreamFlowValueType::String;
    FlowValue.StringValue = Value;
    return FlowValue;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::MakeTextFlowValue(const FText& Value)
{
    FDreamFlowValue FlowValue;
    FlowValue.Type = EDreamFlowValueType::Text;
    FlowValue.TextValue = Value;
    return FlowValue;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::MakeGameplayTagFlowValue(FGameplayTag Value)
{
    FDreamFlowValue FlowValue;
    FlowValue.Type = EDreamFlowValueType::GameplayTag;
    FlowValue.GameplayTagValue = Value;
    return FlowValue;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::MakeObjectFlowValue(UObject* Value)
{
    FDreamFlowValue FlowValue;
    FlowValue.Type = EDreamFlowValueType::Object;
    FlowValue.ObjectValue = Value;
    return FlowValue;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::MakeLiteralFlowBinding(const FDreamFlowValue& LiteralValue)
{
    FDreamFlowValueBinding Binding;
    Binding.SourceType = EDreamFlowValueSourceType::Literal;
    Binding.LiteralValue = LiteralValue;
    Binding.VariableName = NAME_None;
    return Binding;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::MakeVariableFlowBinding(FName VariableName)
{
    FDreamFlowValueBinding Binding;
    Binding.SourceType = EDreamFlowValueSourceType::FlowVariable;
    Binding.VariableName = VariableName;
    return Binding;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::MakeExecutionContextPropertyFlowBinding(const FString& PropertyPath)
{
    FDreamFlowValueBinding Binding;
    Binding.SourceType = EDreamFlowValueSourceType::ExecutionContextProperty;
    Binding.PropertyPath = PropertyPath;
    return Binding;
}

EDreamFlowValueSourceType UDreamFlowBlueprintLibrary::GetFlowBindingSourceType(const FDreamFlowValueBinding& Binding)
{
    return Binding.SourceType;
}

bool UDreamFlowBlueprintLibrary::IsLiteralFlowBinding(const FDreamFlowValueBinding& Binding)
{
    return Binding.SourceType == EDreamFlowValueSourceType::Literal;
}

bool UDreamFlowBlueprintLibrary::IsVariableFlowBinding(const FDreamFlowValueBinding& Binding)
{
    return Binding.SourceType == EDreamFlowValueSourceType::FlowVariable;
}

bool UDreamFlowBlueprintLibrary::IsExecutionContextPropertyFlowBinding(const FDreamFlowValueBinding& Binding)
{
    return Binding.SourceType == EDreamFlowValueSourceType::ExecutionContextProperty;
}

FName UDreamFlowBlueprintLibrary::GetFlowBindingVariableName(const FDreamFlowValueBinding& Binding)
{
    return Binding.VariableName;
}

FString UDreamFlowBlueprintLibrary::GetFlowBindingPropertyPath(const FDreamFlowValueBinding& Binding)
{
    return Binding.PropertyPath;
}

FDreamFlowValue UDreamFlowBlueprintLibrary::GetFlowBindingLiteralValue(const FDreamFlowValueBinding& Binding)
{
    return Binding.LiteralValue;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingVariableName(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FName& OutVariableName)
{
    return Executor ? Executor->GetBindingVariableName(Binding, OutVariableName) : false;
}

bool UDreamFlowBlueprintLibrary::CanExecutorWriteBinding(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding)
{
    return Executor ? Executor->CanWriteBindingValue(Binding) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingBoolValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, bool& OutValue)
{
    return Executor ? Executor->GetBindingBoolValue(Binding, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingIntValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, int32& OutValue)
{
    return Executor ? Executor->GetBindingIntValue(Binding, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingFloatValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, float& OutValue)
{
    return Executor ? Executor->GetBindingFloatValue(Binding, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingNameValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FName& OutValue)
{
    return Executor ? Executor->GetBindingNameValue(Binding, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingStringValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FString& OutValue)
{
    return Executor ? Executor->GetBindingStringValue(Binding, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingTextValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FText& OutValue)
{
    return Executor ? Executor->GetBindingTextValue(Binding, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingGameplayTagValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FGameplayTag& OutValue)
{
    return Executor ? Executor->GetBindingGameplayTagValue(Binding, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingObjectValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, UObject*& OutValue)
{
    return Executor ? Executor->GetBindingObjectValue(Binding, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorBindingValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FDreamFlowValue& OutValue)
{
    return Executor ? Executor->ResolveBindingValue(Binding, OutValue) : false;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::SetFlowBindingSourceType(const FDreamFlowValueBinding& Binding, EDreamFlowValueSourceType SourceType)
{
    FDreamFlowValueBinding Result = Binding;
    Result.SourceType = SourceType;
    return Result;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::SetFlowBindingVariableName(const FDreamFlowValueBinding& Binding, FName VariableName)
{
    FDreamFlowValueBinding Result = Binding;
    Result.VariableName = VariableName;
    return Result;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::SetFlowBindingPropertyPath(const FDreamFlowValueBinding& Binding, const FString& PropertyPath)
{
    FDreamFlowValueBinding Result = Binding;
    Result.PropertyPath = PropertyPath;
    return Result;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::SetFlowBindingLiteralValue(const FDreamFlowValueBinding& Binding, const FDreamFlowValue& LiteralValue)
{
    FDreamFlowValueBinding Result = Binding;
    Result.LiteralValue = LiteralValue;
    return Result;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::SetFlowBindingAsLiteral(const FDreamFlowValueBinding& Binding, const FDreamFlowValue& LiteralValue)
{
    FDreamFlowValueBinding Result = Binding;
    Result.SourceType = EDreamFlowValueSourceType::Literal;
    Result.VariableName = NAME_None;
    Result.LiteralValue = LiteralValue;
    return Result;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::SetFlowBindingAsVariable(const FDreamFlowValueBinding& Binding, FName VariableName)
{
    FDreamFlowValueBinding Result = Binding;
    Result.SourceType = EDreamFlowValueSourceType::FlowVariable;
    Result.VariableName = VariableName;
    return Result;
}

FDreamFlowValueBinding UDreamFlowBlueprintLibrary::SetFlowBindingAsExecutionContextProperty(const FDreamFlowValueBinding& Binding, const FString& PropertyPath)
{
    FDreamFlowValueBinding Result = Binding;
    Result.SourceType = EDreamFlowValueSourceType::ExecutionContextProperty;
    Result.VariableName = NAME_None;
    Result.PropertyPath = PropertyPath;
    return Result;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingBoolValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, bool InValue)
{
    return Executor ? Executor->SetBindingBoolValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingIntValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, int32 InValue)
{
    return Executor ? Executor->SetBindingIntValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingFloatValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, float InValue)
{
    return Executor ? Executor->SetBindingFloatValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingNameValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FName InValue)
{
    return Executor ? Executor->SetBindingNameValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingStringValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, const FString& InValue)
{
    return Executor ? Executor->SetBindingStringValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingTextValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, const FText& InValue)
{
    return Executor ? Executor->SetBindingTextValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingGameplayTagValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FGameplayTag InValue)
{
    return Executor ? Executor->SetBindingGameplayTagValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingObjectValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, UObject* InValue)
{
    return Executor ? Executor->SetBindingObjectValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorBindingValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, const FDreamFlowValue& InValue)
{
    return Executor ? Executor->SetBindingValue(Binding, InValue) : false;
}

bool UDreamFlowBlueprintLibrary::GetExecutorExecutionContextProperty(const UDreamFlowExecutor* Executor, const FString& PropertyPath, FDreamFlowValue& OutValue)
{
    return Executor ? Executor->GetExecutionContextPropertyValue(PropertyPath, OutValue) : false;
}

bool UDreamFlowBlueprintLibrary::SetExecutorExecutionContextProperty(UDreamFlowExecutor* Executor, const FString& PropertyPath, const FDreamFlowValue& InValue)
{
    return Executor ? Executor->SetExecutionContextPropertyValue(PropertyPath, InValue) : false;
}

FDreamFlowExecutionSnapshot UDreamFlowBlueprintLibrary::CaptureExecutorSnapshot(const UDreamFlowExecutor* Executor)
{
    return Executor != nullptr ? Executor->BuildExecutionSnapshot() : FDreamFlowExecutionSnapshot();
}

void UDreamFlowBlueprintLibrary::ApplyExecutorSnapshot(UDreamFlowExecutor* Executor, const FDreamFlowExecutionSnapshot& Snapshot)
{
    if (Executor != nullptr)
    {
        Executor->ApplyExecutionSnapshot(Snapshot);
    }
}

bool UDreamFlowBlueprintLibrary::GetExecutorNodeState(const UDreamFlowExecutor* Executor, FGuid NodeGuid, FName StateKey, FDreamFlowValue& OutValue)
{
    return Executor != nullptr && Executor->GetNodeStateValue(NodeGuid, StateKey, OutValue);
}

bool UDreamFlowBlueprintLibrary::SetExecutorNodeState(UDreamFlowExecutor* Executor, FGuid NodeGuid, FName StateKey, const FDreamFlowValue& InValue)
{
    return Executor != nullptr && Executor->SetNodeStateValue(NodeGuid, StateKey, InValue);
}

void UDreamFlowBlueprintLibrary::ResetExecutorNodeState(UDreamFlowExecutor* Executor, FGuid NodeGuid)
{
    if (Executor != nullptr)
    {
        Executor->ResetNodeState(NodeGuid);
    }
}

void UDreamFlowBlueprintLibrary::ResetAllExecutorNodeStates(UDreamFlowExecutor* Executor)
{
    if (Executor != nullptr)
    {
        Executor->ResetAllNodeStates();
    }
}

UDreamFlowExecutor* UDreamFlowBlueprintLibrary::GetExecutorParent(const UDreamFlowExecutor* Executor)
{
    return Executor != nullptr ? Executor->GetParentExecutor() : nullptr;
}

UDreamFlowExecutor* UDreamFlowBlueprintLibrary::GetExecutorCurrentChild(const UDreamFlowExecutor* Executor)
{
    return Executor != nullptr ? Executor->GetCurrentChildExecutor() : nullptr;
}

TArray<UDreamFlowExecutor*> UDreamFlowBlueprintLibrary::GetExecutorActiveChildren(const UDreamFlowExecutor* Executor)
{
    return Executor != nullptr ? Executor->GetActiveChildExecutors() : TArray<UDreamFlowExecutor*>();
}

TArray<FDreamFlowSubFlowStackEntry> UDreamFlowBlueprintLibrary::GetExecutorSubFlowStack(const UDreamFlowExecutor* Executor)
{
    return Executor != nullptr ? Executor->GetActiveSubFlowStack() : TArray<FDreamFlowSubFlowStackEntry>();
}

FString UDreamFlowBlueprintLibrary::DescribeFlowValue(const FDreamFlowValue& Value)
{
    return Value.Describe();
}

FString UDreamFlowBlueprintLibrary::DescribeCompactFlowValue(const FDreamFlowValue& Value)
{
    return Value.DescribeCompact();
}

FString UDreamFlowBlueprintLibrary::DescribeFlowBinding(const FDreamFlowValueBinding& Binding)
{
    return Binding.Describe();
}

FString UDreamFlowBlueprintLibrary::DescribeCompactFlowBinding(const FDreamFlowValueBinding& Binding)
{
    return Binding.DescribeCompact();
}

bool UDreamFlowBlueprintLibrary::ConvertFlowValue(const FDreamFlowValue& InValue, EDreamFlowValueType TargetType, FDreamFlowValue& OutValue)
{
    return DreamFlowVariable::TryConvertValue(InValue, TargetType, OutValue);
}

bool UDreamFlowBlueprintLibrary::CompareFlowValues(const FDreamFlowValue& LeftValue, const FDreamFlowValue& RightValue, EDreamFlowComparisonOperation Operation, bool& OutResult)
{
    return DreamFlowVariable::TryCompareValues(LeftValue, RightValue, Operation, OutResult);
}

bool UDreamFlowBlueprintLibrary::NodeSupportsFlowAsset(const UDreamFlowNode* FlowNode, const UDreamFlowAsset* FlowAsset)
{
    return FlowNode != nullptr && FlowNode->SupportsFlowAsset(FlowAsset);
}

void UDreamFlowBlueprintLibrary::ValidateFlow(const UDreamFlowAsset* FlowAsset, TArray<FDreamFlowValidationMessage>& OutMessages)
{
    if (FlowAsset == nullptr)
    {
        OutMessages.Reset();

        FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
        Message.Severity = EDreamFlowValidationSeverity::Error;
        Message.Message = FText::FromString(TEXT("Flow asset is null."));
        return;
    }

    FlowAsset->ValidateFlow(OutMessages);
}

bool UDreamFlowBlueprintLibrary::HasValidationErrors(const TArray<FDreamFlowValidationMessage>& ValidationMessages)
{
    for (const FDreamFlowValidationMessage& Message : ValidationMessages)
    {
        if (Message.Severity == EDreamFlowValidationSeverity::Error)
        {
            return true;
        }
    }

    return false;
}
