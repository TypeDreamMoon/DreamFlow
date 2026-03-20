#include "DreamFlowEntryNode.h"

UDreamFlowEntryNode::UDreamFlowEntryNode()
{
    Title = FText::FromString(TEXT("Entry"));
    Description = FText::FromString(TEXT("Graph entry point."));

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.82f, 0.39f, 0.08f, 1.0f);
#endif
}

FText UDreamFlowEntryNode::GetNodeDisplayName_Implementation() const
{
    return FText::FromString(TEXT("Entry"));
}

bool UDreamFlowEntryNode::SupportsMultipleParents_Implementation() const
{
    return false;
}

bool UDreamFlowEntryNode::IsEntryNode_Implementation() const
{
    return true;
}

bool UDreamFlowEntryNode::IsUserCreatable_Implementation() const
{
    return false;
}

bool UDreamFlowEntryNode::CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const
{
    return OtherNode != nullptr && OtherNode != this && !OtherNode->IsEntryNode();
}
