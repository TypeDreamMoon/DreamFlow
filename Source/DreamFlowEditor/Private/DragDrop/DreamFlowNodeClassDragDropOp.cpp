#include "DragDrop/DreamFlowNodeClassDragDropOp.h"

#include "DreamFlowNode.h"
#include "Styling/SlateIconFinder.h"

TSharedRef<FDreamFlowNodeClassDragDropOp> FDreamFlowNodeClassDragDropOp::New(
    TSubclassOf<UDreamFlowNode> InNodeClass,
    const FText& InDisplayName,
    const FText& InCategory,
    const FLinearColor& InTint,
    FOnNodeClassDropped InOnNodeClassDropped)
{
    TSharedRef<FDreamFlowNodeClassDragDropOp> DragDropOp = MakeShared<FDreamFlowNodeClassDragDropOp>();
    DragDropOp->NodeClass = InNodeClass;
    DragDropOp->OnNodeClassDropped = InOnNodeClassDropped;
    DragDropOp->CurrentIconBrush = FSlateIconFinder::FindIconBrushForClass(*InNodeClass);
    DragDropOp->CurrentIconColorAndOpacity = InTint.CopyWithNewOpacity(1.0f);
    DragDropOp->MouseCursor = EMouseCursor::GrabHandClosed;
    DragDropOp->SetToolTip(FText::Format(
        FText::FromString(TEXT("{0}  •  {1}")),
        InDisplayName,
        InCategory), DragDropOp->CurrentIconBrush);
    DragDropOp->CurrentHoverText = FText::Format(
        FText::FromString(TEXT("Drop {0} into the graph")),
        InDisplayName);
    DragDropOp->OnPerformDropToGraphAtLocation = FOnPerformDropToGraphAtLocation::CreateLambda(
        [DroppedNodeClass = InNodeClass, OnNodeClassDropped = InOnNodeClassDropped](TSharedPtr<FDragDropOperation> InOperation, UEdGraph* InGraph, const FVector2f& InNodePosition, const FVector2f& InScreenPosition)
        {
            (void)InOperation;
            (void)InGraph;
            (void)InScreenPosition;

            if (OnNodeClassDropped.IsBound())
            {
                OnNodeClassDropped.Execute(DroppedNodeClass, InNodePosition);
            }
        });
    DragDropOp->SetupDefaults();
    DragDropOp->Construct();
    return DragDropOp;
}
