#include "DreamFlowBlueprintLibrary.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "Execution/DreamFlowExecutor.h"

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
