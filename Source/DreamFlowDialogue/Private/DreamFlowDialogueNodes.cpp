#include "DreamFlowDialogueNodes.h"

#include "DreamDialogueFlowAsset.h"

UDreamFlowDialogueNode::UDreamFlowDialogueNode()
{
    SupportedFlowAssetType = UDreamDialogueFlowAsset::StaticClass();
}

FText UDreamFlowDialogueNode::GetNodeCategory_Implementation() const
{
    return FText::FromString(TEXT("Dialogue"));
}

FText UDreamFlowDialogueNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("Dialogue"));
}

UDreamFlowDialogueLineNode::UDreamFlowDialogueLineNode()
{
    Title = FText::FromString(TEXT("Dialogue Line"));
    Description = FText::FromString(TEXT("A single spoken or narrated line."));
}

FText UDreamFlowDialogueLineNode::GetNodeDisplayName_Implementation() const
{
    return !SpeakerName.IsEmpty()
        ? FText::Format(FText::FromString(TEXT("{0} Line")), SpeakerName)
        : Title;
}

FLinearColor UDreamFlowDialogueLineNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.26f, 0.44f, 0.85f, 1.0f);
}

bool UDreamFlowDialogueLineNode::SupportsMultipleChildren_Implementation() const
{
    return false;
}

UDreamFlowDialogueChoiceNode::UDreamFlowDialogueChoiceNode()
{
    Title = FText::FromString(TEXT("Dialogue Choice"));
    Description = FText::FromString(TEXT("A branching point that offers the player multiple dialogue responses."));
}

FText UDreamFlowDialogueChoiceNode::GetNodeDisplayName_Implementation() const
{
    return !Prompt.IsEmpty() ? Prompt : Title;
}

FLinearColor UDreamFlowDialogueChoiceNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.58f, 0.30f, 0.82f, 1.0f);
}

FText UDreamFlowDialogueChoiceNode::GetNodeAccentLabel_Implementation() const
{
    return ChoiceLabels.Num() > 0
        ? FText::Format(FText::FromString(TEXT("{0} Choices")), ChoiceLabels.Num())
        : FText::FromString(TEXT("Branch"));
}

UDreamFlowDialogueEndNode::UDreamFlowDialogueEndNode()
{
    Title = FText::FromString(TEXT("Dialogue End"));
    Description = FText::FromString(TEXT("A terminal node that closes the current conversation."));
}

FText UDreamFlowDialogueEndNode::GetNodeDisplayName_Implementation() const
{
    return !EndLabel.IsEmpty() ? EndLabel : Title;
}

FLinearColor UDreamFlowDialogueEndNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.91f, 0.24f, 0.33f, 1.0f);
}

FText UDreamFlowDialogueEndNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("End"));
}

bool UDreamFlowDialogueEndNode::SupportsMultipleChildren_Implementation() const
{
    return false;
}

bool UDreamFlowDialogueEndNode::IsTerminalNode_Implementation() const
{
    return true;
}

bool UDreamFlowDialogueEndNode::CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const
{
    (void)OtherNode;
    return false;
}
