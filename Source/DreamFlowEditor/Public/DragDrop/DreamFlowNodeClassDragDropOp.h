#pragma once

#include "CoreMinimal.h"
#include "DragAndDrop/GraphNodeDragDropOp.h"

class UDreamFlowNode;

class DREAMFLOWEDITOR_API FDreamFlowNodeClassDragDropOp : public FGraphNodeDragDropOp
{
public:
    DECLARE_DELEGATE_TwoParams(FOnNodeClassDropped, TSubclassOf<UDreamFlowNode>, const FVector2f&);

    DRAG_DROP_OPERATOR_TYPE(FDreamFlowNodeClassDragDropOp, FGraphNodeDragDropOp)

    static TSharedRef<FDreamFlowNodeClassDragDropOp> New(
        TSubclassOf<UDreamFlowNode> InNodeClass,
        const FText& InDisplayName,
        const FText& InCategory,
        const FLinearColor& InTint,
        FOnNodeClassDropped InOnNodeClassDropped = FOnNodeClassDropped());

    TSubclassOf<UDreamFlowNode> GetNodeClass() const
    {
        return NodeClass;
    }

private:
    TSubclassOf<UDreamFlowNode> NodeClass;
    FOnNodeClassDropped OnNodeClassDropped;
};
