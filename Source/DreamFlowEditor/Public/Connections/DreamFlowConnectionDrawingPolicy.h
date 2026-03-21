#pragma once

#include "ConnectionDrawingPolicy.h"
#include "Containers/Map.h"
#include "EdGraphUtilities.h"

class UDreamFlowEdGraphRerouteNode;

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

private:
    bool ShouldChangeTangentForReroute(UDreamFlowEdGraphRerouteNode* RerouteNode);
    bool GetAverageConnectedPosition(UDreamFlowEdGraphRerouteNode* RerouteNode, EEdGraphPinDirection Direction, FVector2f& OutPos) const;
    bool FindPinCenter(UEdGraphPin* Pin, FVector2f& OutCenter) const;

private:
    TMap<UDreamFlowEdGraphRerouteNode*, bool> RerouteToReversedDirectionMap;
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
