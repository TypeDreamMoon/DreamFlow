#pragma once

#include "ConnectionDrawingPolicy.h"
#include "EdGraphUtilities.h"

class DREAMFLOWEDITOR_API FDreamFlowConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
    FDreamFlowConnectionDrawingPolicy(
        int32 InBackLayerID,
        int32 InFrontLayerID,
        float InZoomFactor,
        const FSlateRect& InClippingRect,
        FSlateWindowElementList& InDrawElements,
        UEdGraph* InGraphObj);

    virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override;
};

class DREAMFLOWEDITOR_API FDreamFlowConnectionFactory : public FGraphPanelPinConnectionFactory
{
public:
    virtual FConnectionDrawingPolicy* CreateConnectionPolicy(
        const UEdGraphSchema* Schema,
        int32 InBackLayerID,
        int32 InFrontLayerID,
        float InZoomFactor,
        const FSlateRect& InClippingRect,
        FSlateWindowElementList& InDrawElements,
        UEdGraph* InGraphObj) const override;
};
