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
