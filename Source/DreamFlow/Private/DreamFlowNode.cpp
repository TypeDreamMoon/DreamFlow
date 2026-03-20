#include "DreamFlowNode.h"

#include "DreamFlowAsset.h"
#include "Execution/DreamFlowExecutor.h"
#include "UObject/Class.h"

UDreamFlowNode::UDreamFlowNode()
{
    NodeGuid = FGuid::NewGuid();
    Title = FText::FromString(TEXT("Flow Node"));
    Description = FText::GetEmpty();
    SupportedFlowAssetType = UDreamFlowAsset::StaticClass();

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.13f, 0.42f, 0.55f, 1.0f);
    EditorPosition = FVector2D::ZeroVector;
#endif
}

FText UDreamFlowNode::GetNodeDisplayName_Implementation() const
{
    if (!Title.IsEmpty())
    {
        return Title;
    }

    return GetClass()->GetDisplayNameText();
}

FLinearColor UDreamFlowNode::GetNodeTint_Implementation() const
{
#if WITH_EDITORONLY_DATA
    return NodeTint;
#else
    return FLinearColor(0.13f, 0.42f, 0.55f, 1.0f);
#endif
}

FText UDreamFlowNode::GetNodeCategory_Implementation() const
{
    return FText::FromString(TEXT("Core"));
}

FText UDreamFlowNode::GetNodeAccentLabel_Implementation() const
{
    return FText::GetEmpty();
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowNode::GetNodeDisplayItems_Implementation() const
{
    return PreviewItems;
}

bool UDreamFlowNode::SupportsMultipleChildren_Implementation() const
{
    return true;
}

bool UDreamFlowNode::SupportsMultipleParents_Implementation() const
{
    return true;
}

bool UDreamFlowNode::IsEntryNode_Implementation() const
{
    return false;
}

bool UDreamFlowNode::IsUserCreatable_Implementation() const
{
    return true;
}

bool UDreamFlowNode::IsTerminalNode_Implementation() const
{
    return false;
}

bool UDreamFlowNode::CanEnterNode_Implementation(UObject* Context) const
{
    (void)Context;
    return true;
}

bool UDreamFlowNode::CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const
{
    return OtherNode != nullptr && OtherNode != this;
}

void UDreamFlowNode::ExecuteNode_Implementation(UObject* Context)
{
    (void)Context;
}

void UDreamFlowNode::ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor)
{
    (void)Executor;
    ExecuteNode(Context);
}

bool UDreamFlowNode::SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;
    (void)Executor;
    return false;
}

int32 UDreamFlowNode::ResolveAutomaticTransitionChildIndex_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;
    (void)Executor;
    return INDEX_NONE;
}

void UDreamFlowNode::SetChildren(const TArray<UDreamFlowNode*>& InChildren)
{
    Children.Reset();

    for (UDreamFlowNode* Child : InChildren)
    {
        if (Child != nullptr)
        {
            Children.Add(Child);
        }
    }
}

TArray<UDreamFlowNode*> UDreamFlowNode::GetChildrenCopy() const
{
    TArray<UDreamFlowNode*> Result;
    Result.Reserve(Children.Num());

    for (UDreamFlowNode* Child : Children)
    {
        Result.Add(Child);
    }

    return Result;
}

TSubclassOf<UDreamFlowAsset> UDreamFlowNode::GetSupportedFlowAssetType() const
{
    if (SupportedFlowAssetType != nullptr)
    {
        return SupportedFlowAssetType;
    }

    return UDreamFlowAsset::StaticClass();
}

bool UDreamFlowNode::SupportsFlowAsset(const UDreamFlowAsset* FlowAsset) const
{
    return SupportsFlowAssetClass(FlowAsset != nullptr ? FlowAsset->GetClass() : UDreamFlowAsset::StaticClass());
}

bool UDreamFlowNode::SupportsFlowAssetClass(const UClass* FlowAssetClass) const
{
    const UClass* RequiredAssetClass = GetSupportedFlowAssetType().Get();
    const UClass* ActualAssetClass = FlowAssetClass != nullptr ? FlowAssetClass : UDreamFlowAsset::StaticClass();
    return RequiredAssetClass != nullptr && ActualAssetClass != nullptr && ActualAssetClass->IsChildOf(RequiredAssetClass);
}

bool UDreamFlowNode::CanAcceptChild(const UDreamFlowNode* OtherNode) const
{
    if (!CanConnectTo(OtherNode))
    {
        return false;
    }

    if (!SupportsMultipleChildren() && Children.Num() > 0 && !Children.Contains(OtherNode))
    {
        return false;
    }

    return true;
}

void UDreamFlowNode::ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    (void)OwningAsset;
    (void)OutMessages;
}

#if WITH_EDITOR
void UDreamFlowNode::SetEditorPosition(const FVector2D& InPosition)
{
#if WITH_EDITORONLY_DATA
    EditorPosition = InPosition;
#else
    (void)InPosition;
#endif
}
#endif
