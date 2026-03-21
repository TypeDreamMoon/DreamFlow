#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UDreamFlowEdGraphNode;

class DREAMFLOWEDITOR_API SGraphNode_DreamFlow : public SGraphNode
{
public:
    SLATE_BEGIN_ARGS(SGraphNode_DreamFlow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UDreamFlowEdGraphNode* InNode);

    virtual void UpdateGraphNode() override;
    virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
    virtual void MoveTo(const FVector2f& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void GetOverlayBrushes(bool bSelected, const FVector2f& WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;

private:
    TSharedRef<SWidget> BuildPreviewArea();
    TSharedRef<SWidget> BuildDisplayItemWidget(const struct FDreamFlowNodeDisplayItem& Item);
    const FSlateBrush* GetOuterNodeBrush() const;
    const FSlateBrush* GetInnerNodeBrush() const;
    const FSlateBrush* GetMarkerBrush() const;
    const FSlateBrush* GetBadgeBrush() const;
    FSlateColor GetNodeOutlineColor() const;
    FSlateColor GetNodeSurfaceColor() const;
    FSlateColor GetMarkerColor() const;
    FSlateColor GetTitleTextColor() const;
    FSlateColor GetClassTextColor() const;
    FSlateColor GetBadgeTextColor() const;
    FSlateColor GetMutedTextColor() const;
    FText GetNodeClassLabel() const;
    FText GetNodeCategoryLabel() const;
    FText GetNodeAccentLabel() const;
    FText GetNodeConnectionLabel() const;
    FText GetNodeDescription() const;
    EVisibility GetPreviewVisibility() const;
    EVisibility GetBreakpointVisibility() const;
    EVisibility GetAccentVisibility() const;
    EVisibility GetDescriptionVisibility() const;

    UDreamFlowEdGraphNode* GetFlowNode() const;

private:
    TArray<TSharedPtr<FSlateBrush>> PreviewImageBrushes;
};
