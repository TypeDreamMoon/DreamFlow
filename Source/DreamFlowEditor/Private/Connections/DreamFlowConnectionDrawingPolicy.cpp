#include "Connections/DreamFlowConnectionDrawingPolicy.h"

#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEdGraphRerouteNode.h"
#include "DreamFlowEdGraphSchema.h"
#include "Containers/Set.h"
#include "EdGraph/EdGraphPin.h"
#include "SGraphPin.h"
#include "Styling/StyleColors.h"

namespace DreamFlowConnectionDrawing
{
    static bool TryResolveFlowNodeTint(const UEdGraphPin* StartPin, FLinearColor& OutNodeTint)
    {
        if (StartPin == nullptr)
        {
            return false;
        }

        TArray<const UEdGraphPin*> PinsToVisit;
        TSet<const UEdGraphPin*> VisitedPins;
        PinsToVisit.Add(StartPin);

        while (PinsToVisit.Num() > 0)
        {
            const UEdGraphPin* CurrentPin = PinsToVisit.Pop(EAllowShrinking::No);
            if (CurrentPin == nullptr || VisitedPins.Contains(CurrentPin))
            {
                continue;
            }

            VisitedPins.Add(CurrentPin);

            if (const UDreamFlowEdGraphNode* FlowNode = Cast<UDreamFlowEdGraphNode>(CurrentPin->GetOwningNode()))
            {
                OutNodeTint = FlowNode->GetNodeTitleColor();
                return true;
            }

            const UDreamFlowEdGraphRerouteNode* RerouteNode = Cast<UDreamFlowEdGraphRerouteNode>(CurrentPin->GetOwningNode());
            if (RerouteNode == nullptr)
            {
                continue;
            }

            for (const UEdGraphPin* LinkedPin : CurrentPin->LinkedTo)
            {
                PinsToVisit.Add(LinkedPin);
            }

            if (const UEdGraphPin* PassThroughPin = RerouteNode->GetPassThroughPin(CurrentPin))
            {
                PinsToVisit.Add(PassThroughPin);

                for (const UEdGraphPin* LinkedPin : PassThroughPin->LinkedTo)
                {
                    PinsToVisit.Add(LinkedPin);
                }
            }
        }

        return false;
    }

    static FLinearColor GetNodeWireColor(const UEdGraphPin* PreferredPin, const UEdGraphPin* FallbackPin)
    {
        FLinearColor NodeTint = FStyleColors::Foreground.GetSpecifiedColor();
        if (TryResolveFlowNodeTint(PreferredPin, NodeTint) || TryResolveFlowNodeTint(FallbackPin, NodeTint))
        {
            return NodeTint.CopyWithNewOpacity(0.94f);
        }

        return NodeTint.CopyWithNewOpacity(0.86f);
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

    ArrowImage = nullptr;
    ArrowRadius = FVector2f::ZeroVector;
}

void FDreamFlowConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
    FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

    if (UDreamFlowEdGraphRerouteNode* OutputRerouteNode = OutputPin != nullptr ? Cast<UDreamFlowEdGraphRerouteNode>(OutputPin->GetOwningNode()) : nullptr)
    {
        if (ShouldChangeTangentForReroute(OutputRerouteNode))
        {
            Params.StartDirection = EGPD_Input;
        }
    }

    if (UDreamFlowEdGraphRerouteNode* InputRerouteNode = InputPin != nullptr ? Cast<UDreamFlowEdGraphRerouteNode>(InputPin->GetOwningNode()) : nullptr)
    {
        if (ShouldChangeTangentForReroute(InputRerouteNode))
        {
            Params.EndDirection = EGPD_Output;
        }
    }

    Params.WireThickness = FMath::Max(Params.WireThickness, 4.5f);
    Params.WireColor = DreamFlowConnectionDrawing::GetNodeWireColor(OutputPin, InputPin);
    Params.bDrawBubbles = false;

    if ((OutputPin != nullptr && OutputPin->bOrphanedPin) || (InputPin != nullptr && InputPin->bOrphanedPin))
    {
        Params.WireColor = FLinearColor(0.85f, 0.22f, 0.22f, 0.95f);
        Params.WireThickness = 3.0f;
    }
}

bool FDreamFlowConnectionDrawingPolicy::ShouldChangeTangentForReroute(UDreamFlowEdGraphRerouteNode* RerouteNode)
{
    if (bool* ExistingResult = RerouteToReversedDirectionMap.Find(RerouteNode))
    {
        return *ExistingResult;
    }

    bool bPinReversed = false;

    FVector2f AverageLeftPin = FVector2f::ZeroVector;
    FVector2f AverageRightPin = FVector2f::ZeroVector;
    FVector2f CenterPin = FVector2f::ZeroVector;
    const bool bCenterValid = FindPinCenter(RerouteNode != nullptr ? RerouteNode->GetOutputPin() : nullptr, CenterPin);
    const bool bLeftValid = GetAverageConnectedPosition(RerouteNode, EGPD_Input, AverageLeftPin);
    const bool bRightValid = GetAverageConnectedPosition(RerouteNode, EGPD_Output, AverageRightPin);

    if (bLeftValid && bRightValid)
    {
        bPinReversed = AverageRightPin.X < AverageLeftPin.X;
    }
    else if (bCenterValid)
    {
        if (bLeftValid)
        {
            bPinReversed = CenterPin.X < AverageLeftPin.X;
        }
        else if (bRightValid)
        {
            bPinReversed = AverageRightPin.X < CenterPin.X;
        }
    }

    RerouteToReversedDirectionMap.Add(RerouteNode, bPinReversed);
    return bPinReversed;
}

bool FDreamFlowConnectionDrawingPolicy::GetAverageConnectedPosition(UDreamFlowEdGraphRerouteNode* RerouteNode, EEdGraphPinDirection Direction, FVector2f& OutPos) const
{
    if (RerouteNode == nullptr)
    {
        return false;
    }

    const UEdGraphPin* Pin = Direction == EGPD_Input ? RerouteNode->GetInputPin() : RerouteNode->GetOutputPin();
    if (Pin == nullptr)
    {
        return false;
    }

    FVector2f AccumulatedPosition = FVector2f::ZeroVector;
    int32 PositionCount = 0;

    for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
    {
        FVector2f CenterPoint = FVector2f::ZeroVector;
        if (FindPinCenter(LinkedPin, CenterPoint))
        {
            AccumulatedPosition += CenterPoint;
            ++PositionCount;
        }
    }

    if (PositionCount == 0)
    {
        return false;
    }

    OutPos = AccumulatedPosition * (1.0f / static_cast<float>(PositionCount));
    return true;
}

bool FDreamFlowConnectionDrawingPolicy::FindPinCenter(UEdGraphPin* Pin, FVector2f& OutCenter) const
{
    if (Pin == nullptr || PinGeometries == nullptr)
    {
        return false;
    }

    if (const TSharedPtr<SGraphPin>* PinWidget = PinToPinWidgetMap.Find(Pin))
    {
        if (FArrangedWidget* PinEntry = PinGeometries->Find((*PinWidget).ToSharedRef()))
        {
            OutCenter = FGeometryHelper::CenterOf(PinEntry->Geometry);
            return true;
        }
    }

    return false;
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
