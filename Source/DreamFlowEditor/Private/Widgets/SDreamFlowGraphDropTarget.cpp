#include "Widgets/SDreamFlowGraphDropTarget.h"

#include "DragDrop/DreamFlowNodeClassDragDropOp.h"
#include "GraphEditor.h"
#include "SGraphPanel.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

void SDreamFlowGraphDropTarget::Construct(const FArguments& InArgs)
{
    GraphEditor = InArgs._GraphEditor;
    OnNodeClassDropped = InArgs._OnNodeClassDropped;

    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [
            InArgs._Content.IsValid() ? InArgs._Content.ToSharedRef() : SNullWidget::NullWidget
        ]
        + SOverlay::Slot()
        [
            SNew(SBorder)
            .Visibility(this, &SDreamFlowGraphDropTarget::GetDropOverlayVisibility)
            .BorderImage(FAppStyle::GetBrush("Graph.ConnectorFeedback.Border"))
            .BorderBackgroundColor(FSlateColor(EStyleColor::Primary))
            .Padding(FMargin(10.0f, 6.0f))
            [
                SNew(STextBlock)
                .Text(this, &SDreamFlowGraphDropTarget::GetDropOverlayText)
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::White))
            ]
        ]
    ];
}

void SDreamFlowGraphDropTarget::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
    SCompoundWidget::OnDragEnter(MyGeometry, DragDropEvent);
    ActiveDragDropOp = GetNodeClassDragDropOp(DragDropEvent);
}

void SDreamFlowGraphDropTarget::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
    SCompoundWidget::OnDragLeave(DragDropEvent);
    ActiveDragDropOp.Reset();
}

FReply SDreamFlowGraphDropTarget::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
    (void)MyGeometry;

    if (const TSharedPtr<FDreamFlowNodeClassDragDropOp> DragDropOp = GetNodeClassDragDropOp(DragDropEvent))
    {
        ActiveDragDropOp = DragDropOp;
        return FReply::Handled();
    }

    ActiveDragDropOp.Reset();
    return SCompoundWidget::OnDragOver(MyGeometry, DragDropEvent);
}

FReply SDreamFlowGraphDropTarget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
    (void)MyGeometry;

    const TSharedPtr<FDreamFlowNodeClassDragDropOp> DragDropOp = GetNodeClassDragDropOp(DragDropEvent);
    ActiveDragDropOp.Reset();

    if (!DragDropOp.IsValid() || !OnNodeClassDropped.IsBound())
    {
        return SCompoundWidget::OnDrop(MyGeometry, DragDropEvent);
    }

    OnNodeClassDropped.Execute(DragDropOp->GetNodeClass(), ResolveGraphDropLocation(DragDropEvent));
    return FReply::Handled();
}

TSharedPtr<FDreamFlowNodeClassDragDropOp> SDreamFlowGraphDropTarget::GetNodeClassDragDropOp(const FDragDropEvent& DragDropEvent) const
{
    return DragDropEvent.GetOperationAs<FDreamFlowNodeClassDragDropOp>();
}

FVector2f SDreamFlowGraphDropTarget::ResolveGraphDropLocation(const FDragDropEvent& DragDropEvent) const
{
    if (!GraphEditor.IsValid())
    {
        return FVector2f::ZeroVector;
    }

    if (SGraphPanel* GraphPanel = GraphEditor->GetGraphPanel())
    {
        const FGeometry GraphPanelGeometry = GraphPanel->GetTickSpaceGeometry();
        return GraphPanel->PanelCoordToGraphCoord(GraphPanelGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition()));
    }

    return GraphEditor->GetPasteLocation2f();
}

EVisibility SDreamFlowGraphDropTarget::GetDropOverlayVisibility() const
{
    return ActiveDragDropOp.IsValid() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

FText SDreamFlowGraphDropTarget::GetDropOverlayText() const
{
    if (const TSharedPtr<FDreamFlowNodeClassDragDropOp> DragDropOp = ActiveDragDropOp.Pin())
    {
        return FText::Format(
            FText::FromString(TEXT("Drop to create {0}")),
            DragDropOp->GetNodeClass() != nullptr
                ? DragDropOp->GetNodeClass()->GetDisplayNameText()
                : FText::FromString(TEXT("node")));
    }

    return FText::GetEmpty();
}
