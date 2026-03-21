#include "DreamFlowEdGraphNode.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEdGraph.h"
#include "DreamFlowEdGraphSchema.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowNode.h"
#include "SGraphNode_DreamFlow.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "SourceCodeNavigation.h"
#include "Subsystems/AssetEditorSubsystem.h"

const FName UDreamFlowEdGraphNode::InputPinName(TEXT("In"));
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

    const TArray<FDreamFlowNodeOutputPin> OutputPins = RuntimeNode != nullptr
        ? RuntimeNode->GetOutputPins()
        : TArray<FDreamFlowNodeOutputPin>();

    if (OutputPins.Num() == 0)
    {
        UEdGraphPin* OutputPin = CreatePin(EGPD_Output, PinCategory, TEXT("Out"));
        OutputPin->PinFriendlyName = FText::FromString(TEXT("Next"));
        return;
    }

    for (const FDreamFlowNodeOutputPin& OutputPinDesc : OutputPins)
    {
        const FName PinName = OutputPinDesc.PinName.IsNone() ? FName(TEXT("Out")) : OutputPinDesc.PinName;
        UEdGraphPin* OutputPin = CreatePin(EGPD_Output, PinCategory, PinName);
        OutputPin->PinFriendlyName = OutputPinDesc.DisplayName.IsEmpty()
            ? FText::FromName(PinName)
            : OutputPinDesc.DisplayName;
    }
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
        TargetPin = GetPrimaryOutputPin();
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
    return !IsEntryNode();
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

void UDreamFlowEdGraphNode::PrepareForCopying()
{
    Super::PrepareForCopying();

    if (RuntimeNode != nullptr)
    {
        // Temporarily move the runtime node under the graph node so text export serializes it as part of the copy payload.
        RuntimeNode->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
    }
}

void UDreamFlowEdGraphNode::PostPasteNode()
{
    Super::PostPasteNode();

    RestoreRuntimeNodeOwner();
    CreateNewGuid();

    if (RuntimeNode != nullptr)
    {
        RuntimeNode->SetFlags(RF_Transactional);
        RuntimeNode->NodeGuid = FGuid::NewGuid();
#if WITH_EDITOR
        RuntimeNode->SetEditorPosition(FVector2D(NodePosX, NodePosY));
#endif
    }
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

TArray<UEdGraphPin*> UDreamFlowEdGraphNode::GetOutputPins() const
{
    TArray<UEdGraphPin*> Result;

    for (UEdGraphPin* Pin : Pins)
    {
        if (Pin != nullptr && Pin->Direction == EGPD_Output)
        {
            Result.Add(Pin);
        }
    }

    return Result;
}

UEdGraphPin* UDreamFlowEdGraphNode::GetPrimaryOutputPin() const
{
    const TArray<UEdGraphPin*> OutputPins = GetOutputPins();
    return OutputPins.Num() > 0 ? OutputPins[0] : nullptr;
}

UEdGraphPin* UDreamFlowEdGraphNode::FindOutputPinByName(FName PinName) const
{
    for (UEdGraphPin* Pin : GetOutputPins())
    {
        if (Pin != nullptr && Pin->PinName == PinName)
        {
            return Pin;
        }
    }

    return nullptr;
}

bool UDreamFlowEdGraphNode::DoesOutputPinAllowMultipleConnections(const UEdGraphPin* OutputPin) const
{
    if (OutputPin == nullptr || RuntimeNode == nullptr || OutputPin->Direction != EGPD_Output)
    {
        return false;
    }

    for (const FDreamFlowNodeOutputPin& OutputPinDesc : RuntimeNode->GetOutputPins())
    {
        if (OutputPinDesc.PinName == OutputPin->PinName)
        {
            return OutputPinDesc.bAllowMultipleConnections;
        }
    }

    return RuntimeNode->SupportsMultipleChildren();
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

bool UDreamFlowEdGraphNode::CanEditNodeSource() const
{
    const UClass* NodeClass = RuntimeNode != nullptr ? RuntimeNode->GetClass() : nullptr;
    if (NodeClass == nullptr)
    {
        return false;
    }

    if (Cast<UBlueprint>(NodeClass->ClassGeneratedBy) != nullptr)
    {
        return true;
    }

    return FSourceCodeNavigation::CanNavigateToClass(NodeClass);
}

bool UDreamFlowEdGraphNode::EditNodeSource() const
{
    const UClass* NodeClass = RuntimeNode != nullptr ? RuntimeNode->GetClass() : nullptr;
    if (NodeClass == nullptr)
    {
        return false;
    }

    if (UBlueprint* GeneratingBlueprint = Cast<UBlueprint>(NodeClass->ClassGeneratedBy))
    {
        if (GEditor == nullptr)
        {
            return false;
        }

        if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            AssetEditorSubsystem->OpenEditorForAsset(GeneratingBlueprint);
            return true;
        }

        return false;
    }

    return FSourceCodeNavigation::NavigateToClass(NodeClass);
}

void UDreamFlowEdGraphNode::RestoreRuntimeNodeOwner()
{
    if (RuntimeNode == nullptr)
    {
        return;
    }

    const UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(GetGraph());
    UDreamFlowAsset* FlowAsset = FlowGraph ? FlowGraph->GetFlowAsset() : nullptr;
    if (FlowAsset == nullptr)
    {
        return;
    }

    RuntimeNode->Rename(nullptr, FlowAsset, REN_DontCreateRedirectors | REN_DoNotDirty);
    RuntimeNode->ClearFlags(RF_Transient);
}

void UDreamFlowEdGraphNode::SyncOwningAsset() const
{
    if (const UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(GetGraph()))
    {
        FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowGraph->GetFlowAsset());
    }
}
