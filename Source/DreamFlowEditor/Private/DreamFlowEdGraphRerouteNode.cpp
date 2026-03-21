#include "DreamFlowEdGraphRerouteNode.h"

#include "DreamFlowEdGraph.h"
#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEdGraphSchema.h"
#include "DreamFlowEditorUtils.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "SGraphNodeKnot.h"

#define LOCTEXT_NAMESPACE "DreamFlowEdGraphRerouteNode"

UDreamFlowEdGraphRerouteNode::UDreamFlowEdGraphRerouteNode()
{
    bCanRenameNode = true;
}

void UDreamFlowEdGraphRerouteNode::AllocateDefaultPins()
{
    UEdGraphPin* InputPin = CreatePin(EGPD_Input, UDreamFlowEdGraphNode::PinCategory, TEXT("InputPin"));
    InputPin->bDefaultValueIsIgnored = true;

    CreatePin(EGPD_Output, UDreamFlowEdGraphNode::PinCategory, TEXT("OutputPin"));
}

void UDreamFlowEdGraphRerouteNode::AutowireNewNode(UEdGraphPin* FromPin)
{
    Super::AutowireNewNode(FromPin);

    if (FromPin == nullptr || GetSchema() == nullptr)
    {
        return;
    }

    UEdGraphPin* TargetPin = nullptr;

    if (FromPin->Direction == EGPD_Output)
    {
        TargetPin = GetInputPin();
    }
    else if (FromPin->Direction == EGPD_Input)
    {
        TargetPin = GetOutputPin();
    }

    if (TargetPin != nullptr)
    {
        GetSchema()->TryCreateConnection(FromPin, TargetPin);
    }
}

FText UDreamFlowEdGraphRerouteNode::GetTooltipText() const
{
    return LOCTEXT("RerouteTooltip", "Reroute Node (reroutes DreamFlow wires).");
}

FText UDreamFlowEdGraphRerouteNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (TitleType == ENodeTitleType::EditableTitle)
    {
        return FText::FromString(NodeComment);
    }

    if (TitleType == ENodeTitleType::MenuTitle)
    {
        return LOCTEXT("RerouteMenuTitle", "Add Reroute Node...");
    }

    return LOCTEXT("RerouteTitle", "Reroute Node");
}

bool UDreamFlowEdGraphRerouteNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
    return Schema != nullptr && Schema->IsA<UDreamFlowEdGraphSchema>();
}

bool UDreamFlowEdGraphRerouteNode::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
    return Graph != nullptr && Graph->IsA<UDreamFlowEdGraph>();
}

FText UDreamFlowEdGraphRerouteNode::GetPinNameOverride(const UEdGraphPin& Pin) const
{
    (void)Pin;
    return FText::GetEmpty();
}

void UDreamFlowEdGraphRerouteNode::OnRenameNode(const FString& NewName)
{
    NodeComment = NewName;
}

TSharedPtr<INameValidatorInterface> UDreamFlowEdGraphRerouteNode::MakeNameValidator() const
{
    return MakeShareable(new FDummyNameValidator(EValidatorResult::Ok));
}

bool UDreamFlowEdGraphRerouteNode::CanSplitPin(const UEdGraphPin* Pin) const
{
    (void)Pin;
    return false;
}

UEdGraphPin* UDreamFlowEdGraphRerouteNode::GetPassThroughPin(const UEdGraphPin* FromPin) const
{
    if (FromPin == nullptr || !Pins.Contains(const_cast<UEdGraphPin*>(FromPin)))
    {
        return nullptr;
    }

    return FromPin == GetInputPin() ? GetOutputPin() : GetInputPin();
}

bool UDreamFlowEdGraphRerouteNode::ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const
{
    OutInputPinIndex = 0;
    OutOutputPinIndex = 1;
    return true;
}

TSharedPtr<SGraphNode> UDreamFlowEdGraphRerouteNode::CreateVisualWidget()
{
    return SNew(SGraphNodeKnot, this);
}

void UDreamFlowEdGraphRerouteNode::NodeConnectionListChanged()
{
    Super::NodeConnectionListChanged();
}

void UDreamFlowEdGraphRerouteNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
    Super::PinConnectionListChanged(Pin);
}

#if WITH_EDITOR
void UDreamFlowEdGraphRerouteNode::PostEditUndo()
{
    Super::PostEditUndo();
    SyncOwningAsset();
}
#endif

void UDreamFlowEdGraphRerouteNode::DestroyNode()
{
    Super::DestroyNode();
    SyncOwningAsset();
}

UEdGraphPin* UDreamFlowEdGraphRerouteNode::GetInputPin() const
{
    return Pins.Num() > 0 ? Pins[0] : nullptr;
}

UEdGraphPin* UDreamFlowEdGraphRerouteNode::GetOutputPin() const
{
    return Pins.Num() > 1 ? Pins[1] : nullptr;
}

void UDreamFlowEdGraphRerouteNode::SyncOwningAsset() const
{
    if (const UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(GetGraph()))
    {
        FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowGraph->GetFlowAsset());
    }
}

#undef LOCTEXT_NAMESPACE
