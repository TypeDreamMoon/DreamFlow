#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "SGraphNode.h"

class IDetailTreeNode;
class IPropertyRowGenerator;
class UDreamFlowEdGraphNode;
class UTexture2D;

class DREAMFLOWEDITOR_API SGraphNode_DreamFlow : public SGraphNode
{
public:
    SLATE_BEGIN_ARGS(SGraphNode_DreamFlow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UDreamFlowEdGraphNode* InNode);

    virtual void UpdateGraphNode() override;
    virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
    virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
    virtual void MoveTo(const FVector2f& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void GetOverlayBrushes(bool bSelected, const FVector2f& WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;

private:
    void RefreshInlinePropertyRows();
    void CollectDetailNodesRecursive(const TArray<TSharedRef<IDetailTreeNode>>& Nodes, TMap<FName, TSharedRef<IDetailTreeNode>>& OutNodes) const;
    TArray<TSharedRef<IDetailTreeNode>> GetInlinePropertyNodes() const;
    TSharedRef<SWidget> BuildInlineEditorArea();
    TSharedRef<SWidget> BuildInlinePropertyWidget(const TSharedRef<IDetailTreeNode>& DetailNode) const;
    TSharedRef<SWidget> BuildPreviewArea();
    TSharedRef<SWidget> BuildDisplayItemWidget(const struct FDreamFlowNodeDisplayItem& Item);
    TArray<FDreamFlowValidationMessage> GetValidationMessages() const;
    FText GetValidationBadgeText() const;
    FText GetValidationToolTipText() const;
    const FSlateBrush* GetOuterNodeBrush() const;
    const FSlateBrush* GetInnerNodeBrush() const;
    const FSlateBrush* GetMarkerBrush() const;
    const FSlateBrush* GetBadgeBrush() const;
    const FSlateBrush* GetNodeIconBrush() const;
    FSlateColor GetNodeOutlineColor() const;
    FSlateColor GetNodeSurfaceColor() const;
    FSlateColor GetMarkerColor() const;
    FSlateColor GetTitleTextColor() const;
    FSlateColor GetClassTextColor() const;
    FSlateColor GetBadgeTextColor() const;
    FSlateColor GetMutedTextColor() const;
    FSlateColor GetValidationBadgeColor() const;
    FText GetNodeClassLabel() const;
    FText GetNodeCategoryLabel() const;
    FText GetNodeAccentLabel() const;
    FText GetNodeConnectionLabel() const;
    FText GetNodeDescription() const;
    EVisibility GetIconVisibility() const;
    EVisibility GetInlineEditorVisibility() const;
    EVisibility GetPreviewVisibility() const;
    EVisibility GetBreakpointVisibility() const;
    EVisibility GetAccentVisibility() const;
    EVisibility GetDescriptionVisibility() const;
    EVisibility GetValidationBadgeVisibility() const;

    UDreamFlowEdGraphNode* GetFlowNode() const;

private:
    TSharedPtr<IPropertyRowGenerator> InlinePropertyRowGenerator;
    TArray<TSharedPtr<FSlateBrush>> PreviewImageBrushes;
    mutable TSharedPtr<FSlateBrush> NodeIconBrush;
};
