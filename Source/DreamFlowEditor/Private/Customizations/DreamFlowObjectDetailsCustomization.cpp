#include "Customizations/DreamFlowObjectDetailsCustomization.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "PropertyHandle.h"

TSharedRef<IDetailCustomization> FDreamFlowAssetDetailsCustomization::MakeInstance()
{
    return MakeShared<FDreamFlowAssetDetailsCustomization>();
}

void FDreamFlowAssetDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    if (TSharedPtr<IPropertyHandle> NodesHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowAsset, Nodes)))
    {
        NodesHandle->MarkHiddenByCustomization();
    }
}

TSharedRef<IDetailCustomization> FDreamFlowNodeDetailsCustomization::MakeInstance()
{
    return MakeShared<FDreamFlowNodeDetailsCustomization>();
}

void FDreamFlowNodeDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    if (TSharedPtr<IPropertyHandle> ChildrenHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowNode, Children)))
    {
        ChildrenHandle->MarkHiddenByCustomization();
    }

    if (TSharedPtr<IPropertyHandle> OutputLinksHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowNode, OutputLinks)))
    {
        OutputLinksHandle->MarkHiddenByCustomization();
    }

    if (TSharedPtr<IPropertyHandle> PreviewItemsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowNode, PreviewItems)))
    {
        PreviewItemsHandle->MarkHiddenByCustomization();
    }

    if (TSharedPtr<IPropertyHandle> NodeGuidHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowNode, NodeGuid)))
    {
        NodeGuidHandle->MarkHiddenByCustomization();

        IDetailCategoryBuilder& FlowCategory = DetailBuilder.EditCategory(TEXT("Flow"));
        FlowCategory.AddProperty(NodeGuidHandle, EPropertyLocation::Advanced);
    }
}
