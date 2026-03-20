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

bool UDreamFlowBlueprintLibrary::GetExecutorVariable(const UDreamFlowExecutor* Executor, FName VariableName, FDreamFlowValue& OutValue)
{
    return Executor ? Executor->GetVariableValue(VariableName, OutValue) : false;
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
