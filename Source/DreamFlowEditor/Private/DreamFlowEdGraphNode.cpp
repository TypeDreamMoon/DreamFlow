#include "DreamFlowEdGraphNode.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEdGraph.h"
#include "DreamFlowEdGraphSchema.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowNode.h"
#include "SGraphNode_DreamFlow.h"

const FName UDreamFlowEdGraphNode::InputPinName(TEXT("In"));
const FName UDreamFlowEdGraphNode::OutputPinName(TEXT("Out"));
const FName UDreamFlowEdGraphNode::PinCategory(TEXT("DreamFlow"));

UDreamFlowEdGraphNode::UDreamFlowEdGraphNode()
{
}

void UDreamFlowEdGraphNode::AllocateDefaultPins()
{
    if (!IsEntryNode())
    {
        CreatePin(EGPD_Input, PinCategory, InputPinName);
    }

    CreatePin(EGPD_Output, PinCategory, OutputPinName);
}

void UDreamFlowEdGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
    Super::AutowireNewNode(FromPin);

    if (FromPin == nullptr || GetSchema() == nullptr)
    {
        return;
    }

    UEdGraphPin* TargetPin = nullptr;

    if (FromPin->Direction == EGPD_Output)
    {
        TargetPin = FindPin(InputPinName, EGPD_Input);
    }
    else if (FromPin->Direction == EGPD_Input)
    {
        TargetPin = FindPin(OutputPinName, EGPD_Output);
    }

    if (TargetPin != nullptr)
    {
        GetSchema()->TryCreateConnection(FromPin, TargetPin);
    }
}

FText UDreamFlowEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    (void)TitleType;
    return RuntimeNode ? RuntimeNode->GetNodeDisplayName() : FText::FromString(TEXT("Flow Node"));
}

FLinearColor UDreamFlowEdGraphNode::GetNodeTitleColor() const
{
    return RuntimeNode ? RuntimeNode->GetNodeTint() : FLinearColor(0.16f, 0.45f, 0.57f, 1.0f);
}

FText UDreamFlowEdGraphNode::GetTooltipText() const
{
    return RuntimeNode ? RuntimeNode->Description : FText::GetEmpty();
}

bool UDreamFlowEdGraphNode::CanDuplicateNode() const
{
    return false;
}

bool UDreamFlowEdGraphNode::CanUserDeleteNode() const
{
    return !IsEntryNode();
}

bool UDreamFlowEdGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
    return Schema != nullptr && Schema->IsA<UDreamFlowEdGraphSchema>();
}

bool UDreamFlowEdGraphNode::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
    return Graph != nullptr && Graph->IsA<UDreamFlowEdGraph>();
}

TSharedPtr<SGraphNode> UDreamFlowEdGraphNode::CreateVisualWidget()
{
    return SNew(SGraphNode_DreamFlow, this);
}

void UDreamFlowEdGraphNode::NodeConnectionListChanged()
{
    Super::NodeConnectionListChanged();
    SyncOwningAsset();
}

void UDreamFlowEdGraphNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
    Super::PinConnectionListChanged(Pin);
    SyncOwningAsset();
}

#if WITH_EDITOR
void UDreamFlowEdGraphNode::PostEditUndo()
{
    Super::PostEditUndo();
    SyncOwningAsset();
}
#endif

void UDreamFlowEdGraphNode::DestroyNode()
{
    Super::DestroyNode();
    SyncOwningAsset();
}

void UDreamFlowEdGraphNode::SetRuntimeNode(UDreamFlowNode* InRuntimeNode)
{
    RuntimeNode = InRuntimeNode;
}

UDreamFlowNode* UDreamFlowEdGraphNode::GetRuntimeNode() const
{
    return RuntimeNode;
}

bool UDreamFlowEdGraphNode::IsEntryNode() const
{
    return RuntimeNode != nullptr && RuntimeNode->IsEntryNode();
}

bool UDreamFlowEdGraphNode::HasBreakpoint() const
{
    const UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(GetGraph());
    const UDreamFlowAsset* FlowAsset = FlowGraph ? FlowGraph->GetFlowAsset() : nullptr;
    return FlowAsset != nullptr && RuntimeNode != nullptr && FlowAsset->HasBreakpointOnNode(RuntimeNode->NodeGuid);
}

void UDreamFlowEdGraphNode::SetBreakpointEnabled(bool bEnabled)
{
    UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(GetGraph());
    UDreamFlowAsset* FlowAsset = FlowGraph ? FlowGraph->GetFlowAsset() : nullptr;
    if (FlowAsset == nullptr || RuntimeNode == nullptr)
    {
        return;
    }

    FlowAsset->Modify();
    FlowAsset->SetBreakpointOnNode(RuntimeNode->NodeGuid, bEnabled);

    if (FlowGraph != nullptr)
    {
        FlowGraph->NotifyGraphChanged();
    }
}

void UDreamFlowEdGraphNode::ToggleBreakpoint()
{
    SetBreakpointEnabled(!HasBreakpoint());
}

void UDreamFlowEdGraphNode::SyncOwningAsset() const
{
    if (const UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(GetGraph()))
    {
        FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowGraph->GetFlowAsset());
    }
}
