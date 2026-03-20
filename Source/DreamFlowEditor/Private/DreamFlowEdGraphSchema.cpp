#include "DreamFlowEdGraphSchema.h"

#include "DreamFlowEdGraph.h"
#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowEntryNode.h"
#include "DreamFlowNode.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UIAction.h"
#include "GraphEditorActions.h"
#include "Kismet2/SClassPickerDialog.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "DreamFlowEdGraphSchema"

namespace DreamFlowSchema
{
    static void SynchronizeGraphAsset(const UEdGraph* Graph)
    {
        if (UDreamFlowAsset* FlowAsset = FDreamFlowEditorUtils::GetFlowAssetFromGraph(Graph))
        {
            FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowAsset);
        }
    }

    class FDreamFlowClassFilter : public IClassViewerFilter
    {
    public:
        virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
        {
            (void)InInitOptions;
            (void)InFilterFuncs;
            return FDreamFlowEditorUtils::IsNodeClassCreatable(InClass);
        }

        virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
        {
            (void)InInitOptions;
            (void)InFilterFuncs;

            if (InUnloadedClassData->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
            {
                return false;
            }

            return InUnloadedClassData->IsChildOf(UDreamFlowNode::StaticClass()) && !InUnloadedClassData->IsChildOf(UDreamFlowEntryNode::StaticClass());
        }
    };

    class FDreamFlowSchemaAction_NewNode final : public FEdGraphSchemaAction
    {
    public:
        TSubclassOf<UDreamFlowNode> NodeClass;

        FDreamFlowSchemaAction_NewNode(TSubclassOf<UDreamFlowNode> InNodeClass, const FText& InDescription, const FText& InToolTip)
            : FEdGraphSchemaAction(
                InNodeClass != nullptr ? CastChecked<UDreamFlowNode>(InNodeClass->GetDefaultObject())->GetNodeCategory() : FText::GetEmpty(),
                InDescription,
                InToolTip,
                0)
            , NodeClass(InNodeClass)
        {
        }

        virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode) override
        {
            return FDreamFlowEditorUtils::CreateNodeInGraph(ParentGraph, NodeClass, Location, FromPin, bSelectNewNode);
        }
    };

    class FDreamFlowSchemaAction_PickNodeClass final : public FEdGraphSchemaAction
    {
    public:
        FDreamFlowSchemaAction_PickNodeClass()
            : FEdGraphSchemaAction(
                FText::GetEmpty(),
                LOCTEXT("PickNodeClassAction", "Choose Node Class..."),
                LOCTEXT("PickNodeClassActionTooltip", "Pick any DreamFlow node class, including Blueprint subclasses."),
                1)
        {
        }

        virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode) override
        {
            FClassViewerInitializationOptions PickerOptions;
            PickerOptions.Mode = EClassViewerMode::ClassPicker;
            PickerOptions.DisplayMode = EClassViewerDisplayMode::TreeView;
            PickerOptions.bShowUnloadedBlueprints = true;
            PickerOptions.bShowObjectRootClass = false;
            PickerOptions.NameTypeToDisplay = EClassViewerNameTypeToDisplay::Dynamic;
            PickerOptions.ClassFilters.Add(MakeShared<FDreamFlowClassFilter>());

            UClass* ChosenClass = nullptr;
            const bool bPickedClass = SClassPickerDialog::PickClass(
                LOCTEXT("PickNodeClassDialog", "Select DreamFlow Node Class"),
                PickerOptions,
                ChosenClass,
                UDreamFlowNode::StaticClass());

            return bPickedClass
                ? FDreamFlowEditorUtils::CreateNodeInGraph(ParentGraph, ChosenClass, Location, FromPin, bSelectNewNode)
                : nullptr;
        }
    };
}

void UDreamFlowEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    for (const TSubclassOf<UDreamFlowNode>& NodeClass : FDreamFlowEditorUtils::GetLoadedCreatableNodeClasses())
    {
        const UClass* LoadedClass = *NodeClass;
        const UDreamFlowNode* DefaultNode = LoadedClass ? Cast<UDreamFlowNode>(LoadedClass->GetDefaultObject()) : nullptr;
        if (DefaultNode == nullptr)
        {
            continue;
        }

        const FText Description = DefaultNode->GetNodeDisplayName();
        const FText ToolTip = FText::Format(
            LOCTEXT("CreateNodeTooltip", "Create a {0} node."),
            LoadedClass->GetDisplayNameText());

        ContextMenuBuilder.AddAction(MakeShared<DreamFlowSchema::FDreamFlowSchemaAction_NewNode>(NodeClass, Description, ToolTip));
    }

    ContextMenuBuilder.AddAction(MakeShared<DreamFlowSchema::FDreamFlowSchemaAction_PickNodeClass>());
}

void UDreamFlowEdGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
    if (Context != nullptr && Context->Node != nullptr)
    {
        FToolMenuSection& Section = Menu->AddSection("DreamFlowNodeActions", LOCTEXT("DreamFlowNodeActions", "DreamFlow"));
        if (UDreamFlowEdGraphNode* FlowGraphNode = Cast<UDreamFlowEdGraphNode>(Context->Node))
        {
            Section.AddMenuEntry(
                "DreamFlowToggleBreakpoint",
                FlowGraphNode->HasBreakpoint()
                    ? LOCTEXT("RemoveBreakpointLabel", "Remove Breakpoint")
                    : LOCTEXT("AddBreakpointLabel", "Add Breakpoint"),
                FlowGraphNode->HasBreakpoint()
                    ? LOCTEXT("RemoveBreakpointTooltip", "Remove the breakpoint from this node.")
                    : LOCTEXT("AddBreakpointTooltip", "Pause the DreamFlow executor before this node executes."),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([FlowGraphNode]()
                {
                    if (FlowGraphNode != nullptr)
                    {
                        FlowGraphNode->ToggleBreakpoint();
                    }
                })));
        }

        Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
        Section.AddMenuEntry(FGenericCommands::Get().Delete);
    }
}

const FPinConnectionResponse UDreamFlowEdGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
    if (PinA == nullptr || PinB == nullptr)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("NullPinConnection", "Invalid pin."));
    }

    if (PinA->GetOwningNode() == PinB->GetOwningNode())
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameNodeConnection", "Cannot connect a node to itself."));
    }

    const UEdGraphPin* InputPin = nullptr;
    const UEdGraphPin* OutputPin = nullptr;
    if (!CategorizePinsByDirection(PinA, PinB, InputPin, OutputPin))
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("DirectionMismatch", "Pins must connect from output to input."));
    }

    const UDreamFlowEdGraphNode* SourceGraphNode = Cast<UDreamFlowEdGraphNode>(OutputPin->GetOwningNode());
    const UDreamFlowEdGraphNode* TargetGraphNode = Cast<UDreamFlowEdGraphNode>(InputPin->GetOwningNode());
    const UDreamFlowNode* SourceNode = SourceGraphNode ? SourceGraphNode->GetRuntimeNode() : nullptr;
    const UDreamFlowNode* TargetNode = TargetGraphNode ? TargetGraphNode->GetRuntimeNode() : nullptr;

    if (SourceNode == nullptr || TargetNode == nullptr)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("MissingRuntimeNode", "Node data is missing."));
    }

    if (!SourceNode->CanAcceptChild(TargetNode))
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionRejected", "The source node rejected this connection."));
    }

    const bool bBreakSourceLinks = !SourceNode->SupportsMultipleChildren() && OutputPin->LinkedTo.Num() > 0;
    const bool bBreakTargetLinks = !TargetNode->SupportsMultipleParents() && InputPin->LinkedTo.Num() > 0;

    if (bBreakSourceLinks && bBreakTargetLinks)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_AB, LOCTEXT("ReplaceBothConnections", "Replace existing source and target links."));
    }

    if (bBreakSourceLinks)
    {
        return FPinConnectionResponse(OutputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("ReplaceSourceConnections", "Replace existing outgoing links."));
    }

    if (bBreakTargetLinks)
    {
        return FPinConnectionResponse(InputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("ReplaceTargetConnections", "Replace existing incoming links."));
    }

    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
}

bool UDreamFlowEdGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
    const bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);
    if (bModified && PinA != nullptr)
    {
        DreamFlowSchema::SynchronizeGraphAsset(PinA->GetOwningNode()->GetGraph());
    }

    return bModified;
}

void UDreamFlowEdGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
    Super::BreakNodeLinks(TargetNode);
    DreamFlowSchema::SynchronizeGraphAsset(TargetNode.GetGraph());
}

void UDreamFlowEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
    Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
    DreamFlowSchema::SynchronizeGraphAsset(TargetPin.GetOwningNode()->GetGraph());
}

void UDreamFlowEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
    Super::BreakSinglePinLink(SourcePin, TargetPin);

    if (SourcePin != nullptr)
    {
        DreamFlowSchema::SynchronizeGraphAsset(SourcePin->GetOwningNode()->GetGraph());
    }
}

void UDreamFlowEdGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
    if (UDreamFlowAsset* FlowAsset = FDreamFlowEditorUtils::GetFlowAssetFromGraph(&Graph))
    {
        FDreamFlowEditorUtils::GetOrCreateGraph(FlowAsset);
    }
}

bool UDreamFlowEdGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
    (void)Pin;
    return true;
}

FLinearColor UDreamFlowEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
    (void)PinType;
    return FLinearColor(0.07f, 0.72f, 0.78f, 1.0f);
}

#undef LOCTEXT_NAMESPACE
