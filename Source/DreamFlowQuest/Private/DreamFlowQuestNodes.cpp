#include "DreamFlowQuestNodes.h"

#include "DreamQuestFlowAsset.h"

UDreamFlowQuestNode::UDreamFlowQuestNode()
{
    SupportedFlowAssetType = UDreamQuestFlowAsset::StaticClass();
}

FText UDreamFlowQuestNode::GetNodeCategory_Implementation() const
{
    return FText::FromString(TEXT("Quest"));
}

FText UDreamFlowQuestNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("Quest"));
}

UDreamFlowQuestTaskNode::UDreamFlowQuestTaskNode()
{
    Title = FText::FromString(TEXT("Quest Task"));
    Description = FText::FromString(TEXT("A concrete quest step or objective."));
}

FText UDreamFlowQuestTaskNode::GetNodeDisplayName_Implementation() const
{
    if (!TaskLabel.IsEmpty())
    {
        return TaskLabel;
    }

    if (!QuestId.IsNone())
    {
        return FText::FromName(QuestId);
    }

    return Title;
}

FLinearColor UDreamFlowQuestTaskNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.13f, 0.58f, 0.42f, 1.0f);
}

FText UDreamFlowQuestTaskNode::GetNodeAccentLabel_Implementation() const
{
    return bOptional ? FText::FromString(TEXT("Optional")) : FText::FromString(TEXT("Required"));
}

UDreamFlowQuestConditionNode::UDreamFlowQuestConditionNode()
{
    Title = FText::FromString(TEXT("Quest Condition"));
    Description = FText::FromString(TEXT("A branch point driven by quest state or gameplay checks."));
}

FText UDreamFlowQuestConditionNode::GetNodeDisplayName_Implementation() const
{
    return !ConditionLabel.IsEmpty() ? ConditionLabel : Title;
}

FLinearColor UDreamFlowQuestConditionNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.84f, 0.53f, 0.16f, 1.0f);
}

FText UDreamFlowQuestConditionNode::GetNodeAccentLabel_Implementation() const
{
    return bExpectedResult ? FText::FromString(TEXT("True")) : FText::FromString(TEXT("False"));
}

UDreamFlowQuestCompleteNode::UDreamFlowQuestCompleteNode()
{
    Title = FText::FromString(TEXT("Quest Complete"));
    Description = FText::FromString(TEXT("A terminal node representing quest completion or reward payout."));
}

FText UDreamFlowQuestCompleteNode::GetNodeDisplayName_Implementation() const
{
    return !CompletionLabel.IsEmpty() ? CompletionLabel : Title;
}

FLinearColor UDreamFlowQuestCompleteNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.08f, 0.69f, 0.68f, 1.0f);
}

FText UDreamFlowQuestCompleteNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("Complete"));
}

bool UDreamFlowQuestCompleteNode::SupportsMultipleChildren_Implementation() const
{
    return false;
}

bool UDreamFlowQuestCompleteNode::IsTerminalNode_Implementation() const
{
    return true;
}

bool UDreamFlowQuestCompleteNode::CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const
{
    (void)OtherNode;
    return false;
}
