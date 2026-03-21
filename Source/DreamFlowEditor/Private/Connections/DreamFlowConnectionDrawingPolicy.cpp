#include "Connections/DreamFlowConnectionDrawingPolicy.h"

#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEdGraphSchema.h"
#include "Styling/StyleColors.h"

namespace DreamFlowConnectionDrawing
{
    static FLinearColor GetNodeWireColor(const UEdGraphPin* Pin)
    {
        if (const UDreamFlowEdGraphNode* FlowNode = Pin != nullptr ? Cast<UDreamFlowEdGraphNode>(Pin->GetOwningNode()) : nullptr)
        {
            const FLinearColor NodeTint = FlowNode->GetNodeTitleColor();
            return FLinearColor::LerpUsingHSV(FStyleColors::Foreground.GetSpecifiedColor(), NodeTint, 0.72f).CopyWithNewOpacity(0.94f);
        }

        return FStyleColors::Foreground.GetSpecifiedColor().CopyWithNewOpacity(0.86f);
    }
}

FDreamFlowConnectionDrawingPolicy::FDreamFlowConnectionDrawingPolicy(
    int32 InBackLayerID,
    int32 InFrontLayerID,
    float InZoomFactor,
    const FSlateRect& InClippingRect,
    FSlateWindowElementList& InDrawElements,
    UEdGraph* InGraphObj)
    : FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
{
    (void)InGraphObj;
}

void FDreamFlowConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
    FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

    const FLinearColor OutputColor = DreamFlowConnectionDrawing::GetNodeWireColor(OutputPin);
    const FLinearColor InputColor = DreamFlowConnectionDrawing::GetNodeWireColor(InputPin);
    const FLinearColor MixedColor = FLinearColor::LerpUsingHSV(OutputColor, InputColor, 0.35f);

    Params.WireThickness = FMath::Max(Params.WireThickness, 4.5f);
    Params.WireColor = MixedColor.CopyWithNewOpacity(0.94f);
    Params.bDrawBubbles = false;

    if ((OutputPin != nullptr && OutputPin->bOrphanedPin) || (InputPin != nullptr && InputPin->bOrphanedPin))
    {
        Params.WireColor = FLinearColor(0.85f, 0.22f, 0.22f, 0.95f);
        Params.WireThickness = 3.0f;
    }
}

FConnectionDrawingPolicy* FDreamFlowConnectionFactory::CreateConnectionPolicy(
    const UEdGraphSchema* Schema,
    int32 InBackLayerID,
    int32 InFrontLayerID,
    float InZoomFactor,
    const FSlateRect& InClippingRect,
    FSlateWindowElementList& InDrawElements,
    UEdGraph* InGraphObj) const
{
    return Schema != nullptr && Schema->IsA<UDreamFlowEdGraphSchema>()
        ? new FDreamFlowConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj)
        : nullptr;
}
