#include "Connections/DreamFlowConnectionDrawingPolicy.h"

#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEdGraphSchema.h"

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

    Params.WireThickness = 3.0f;
    Params.WireColor = FLinearColor(0.21f, 0.76f, 0.83f, 0.9f);

    const UDreamFlowEdGraphNode* SourceNode = OutputPin != nullptr ? Cast<UDreamFlowEdGraphNode>(OutputPin->GetOwningNode()) : nullptr;
    if (SourceNode != nullptr)
    {
        const FLinearColor SourceColor = SourceNode->GetNodeTitleColor();
        Params.WireColor = FLinearColor(SourceColor.R, SourceColor.G, SourceColor.B, 0.88f);
    }

    if ((OutputPin != nullptr && OutputPin->bOrphanedPin) || (InputPin != nullptr && InputPin->bOrphanedPin))
    {
        Params.WireColor = FLinearColor(0.85f, 0.22f, 0.22f, 0.95f);
        Params.WireThickness = 2.0f;
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
