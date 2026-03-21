#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNode.h"
#include "Widgets/SCompoundWidget.h"

class FDreamFlowNodeClassDragDropOp;
class SGraphEditor;

class DREAMFLOWEDITOR_API SDreamFlowGraphDropTarget : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_TwoParams(FOnNodeClassDropped, TSubclassOf<UDreamFlowNode>, const FVector2f&);

    SLATE_BEGIN_ARGS(SDreamFlowGraphDropTarget) {}
        SLATE_ARGUMENT(TSharedPtr<SGraphEditor>, GraphEditor)
        SLATE_ARGUMENT(TSharedPtr<SWidget>, Content)
        SLATE_EVENT(FOnNodeClassDropped, OnNodeClassDropped)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
    virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
    virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
    virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
    TSharedPtr<FDreamFlowNodeClassDragDropOp> GetNodeClassDragDropOp(const FDragDropEvent& DragDropEvent) const;
    FVector2f ResolveGraphDropLocation(const FDragDropEvent& DragDropEvent) const;
    EVisibility GetDropOverlayVisibility() const;
    FText GetDropOverlayText() const;

private:
    TSharedPtr<SGraphEditor> GraphEditor;
    FOnNodeClassDropped OnNodeClassDropped;
    TWeakPtr<FDreamFlowNodeClassDragDropOp> ActiveDragDropOp;
};
