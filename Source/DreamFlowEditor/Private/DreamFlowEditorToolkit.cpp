#include "DreamFlowEditorToolkit.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEdGraph.h"
#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowNode.h"
#include "Widgets/SDreamFlowNodePalette.h"
#include "Widgets/SDreamFlowValidationView.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "DreamFlowEditorToolkit"

TMap<UDreamFlowAsset*, FDreamFlowEditorToolkit*> FDreamFlowEditorToolkit::ActiveEditors;
const FName FDreamFlowEditorToolkit::PaletteTabId(TEXT("DreamFlowEditor_Palette"));
const FName FDreamFlowEditorToolkit::GraphTabId(TEXT("DreamFlowEditor_Graph"));
const FName FDreamFlowEditorToolkit::DetailsTabId(TEXT("DreamFlowEditor_Details"));
const FName FDreamFlowEditorToolkit::ValidationTabId(TEXT("DreamFlowEditor_Validation"));

FDreamFlowEditorToolkit::~FDreamFlowEditorToolkit()
{
    if (FlowAsset != nullptr)
    {
        ActiveEditors.Remove(FlowAsset);
    }

    if (EditingGraph != nullptr && GraphChangedHandle.IsValid())
    {
        EditingGraph->RemoveOnGraphChangedHandler(GraphChangedHandle);
        GraphChangedHandle.Reset();
    }
}

void FDreamFlowEditorToolkit::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDreamFlowAsset* InFlowAsset)
{
    FlowAsset = InFlowAsset;
    EditingGraph = FDreamFlowEditorUtils::GetOrCreateGraph(FlowAsset);
    ActiveEditors.Add(FlowAsset, this);

    BindCommands();
    CreateWidgets();

    if (EditingGraph != nullptr)
    {
        GraphChangedHandle = EditingGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FDreamFlowEditorToolkit::HandleGraphChanged));
    }

    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("DreamFlowEditorLayout_v1")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.22f)
                ->AddTab(PaletteTabId, ETabState::OpenedTab)
                ->SetHideTabWell(true)
            )
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.53f)
                ->AddTab(GraphTabId, ETabState::OpenedTab)
                ->SetHideTabWell(true)
            )
            ->Split
            (
                FTabManager::NewSplitter()
                ->SetOrientation(Orient_Vertical)
                ->SetSizeCoefficient(0.25f)
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.58f)
                    ->AddTab(DetailsTabId, ETabState::OpenedTab)
                    ->SetHideTabWell(true)
                )
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.42f)
                    ->AddTab(ValidationTabId, ETabState::OpenedTab)
                    ->SetHideTabWell(true)
                )
            )
        );

    const bool bCreateDefaultStandaloneMenu = true;
    const bool bCreateDefaultToolbar = true;
    TArray<UObject*> ObjectsToEdit;
    ObjectsToEdit.Add(FlowAsset);

    InitAssetEditor(
        Mode,
        InitToolkitHost,
        GetToolkitFName(),
        Layout,
        bCreateDefaultStandaloneMenu,
        bCreateDefaultToolbar,
        ObjectsToEdit);

    RegenerateMenusAndToolbars();

    if (DetailsView.IsValid())
    {
        DetailsView->SetObject(FlowAsset);
    }

    RefreshValidation();
}

void FDreamFlowEditorToolkit::OpenNodeEditorForGraph(UEdGraph* Graph, UObject* ObjectToEdit)
{
    UDreamFlowAsset* Asset = FDreamFlowEditorUtils::GetFlowAssetFromGraph(Graph);
    FDreamFlowEditorToolkit* Toolkit = Asset != nullptr ? ActiveEditors.FindRef(Asset) : nullptr;
    if (Toolkit != nullptr)
    {
        Toolkit->OpenNodeEditor(ObjectToEdit);
    }
}

FName FDreamFlowEditorToolkit::GetToolkitFName() const
{
    return FName(TEXT("DreamFlowEditor"));
}

FText FDreamFlowEditorToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("DreamFlowEditorAppName", "Dream Flow");
}

FText FDreamFlowEditorToolkit::GetToolkitName() const
{
    return FlowAsset != nullptr
        ? FText::Format(LOCTEXT("DreamFlowEditorToolkitName", "Dream Flow - {0}"), FText::FromString(FlowAsset->GetName()))
        : LOCTEXT("DreamFlowEditorToolkitNameFallback", "Dream Flow");
}

FString FDreamFlowEditorToolkit::GetWorldCentricTabPrefix() const
{
    return TEXT("DreamFlow");
}

FLinearColor FDreamFlowEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.75f, 0.38f, 0.11f, 1.0f);
}

void FDreamFlowEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner(PaletteTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnPaletteTab))
        .SetDisplayName(LOCTEXT("PaletteTabLabel", "Palette"));

    InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnGraphTab))
        .SetDisplayName(LOCTEXT("GraphTabLabel", "Graph"));

    InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnDetailsTab))
        .SetDisplayName(LOCTEXT("DetailsTabLabel", "Node Editor"));

    InTabManager->RegisterTabSpawner(ValidationTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnValidationTab))
        .SetDisplayName(LOCTEXT("ValidationTabLabel", "Validation"));
}

void FDreamFlowEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
    InTabManager->UnregisterTabSpawner(PaletteTabId);
    InTabManager->UnregisterTabSpawner(GraphTabId);
    InTabManager->UnregisterTabSpawner(DetailsTabId);
    InTabManager->UnregisterTabSpawner(ValidationTabId);
}

void FDreamFlowEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(FlowAsset);
    Collector.AddReferencedObject(EditingGraph);
}

FString FDreamFlowEditorToolkit::GetReferencerName() const
{
    return TEXT("FDreamFlowEditorToolkit");
}

TSharedRef<SDockTab> FDreamFlowEditorToolkit::SpawnGraphTab(const FSpawnTabArgs& Args)
{
    (void)Args;
    return SNew(SDockTab)
        [
            GraphEditorWidget.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FDreamFlowEditorToolkit::SpawnPaletteTab(const FSpawnTabArgs& Args)
{
    (void)Args;
    return SNew(SDockTab)
        [
            PaletteWidget.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FDreamFlowEditorToolkit::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
    (void)Args;
    return SNew(SDockTab)
        [
            DetailsView.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FDreamFlowEditorToolkit::SpawnValidationTab(const FSpawnTabArgs& Args)
{
    (void)Args;
    return SNew(SDockTab)
        [
            ValidationWidget.ToSharedRef()
        ];
}

void FDreamFlowEditorToolkit::CreateWidgets()
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;
    DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);

    PaletteWidget = SNew(SDreamFlowNodePalette)
        .OnNodeClassPicked(SDreamFlowNodePalette::FOnNodeClassPicked::CreateSP(this, &FDreamFlowEditorToolkit::CreateNodeFromPalette));

    ValidationWidget = SNew(SDreamFlowValidationView)
        .OnMessageActivated(SDreamFlowValidationView::FOnMessageActivated::CreateSP(this, &FDreamFlowEditorToolkit::JumpToNodeGuid));

    FGraphAppearanceInfo AppearanceInfo;
    AppearanceInfo.CornerText = LOCTEXT("GraphCornerText", "Dream Flow Studio");

    SGraphEditor::FGraphEditorEvents GraphEditorEvents;
    GraphEditorEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FDreamFlowEditorToolkit::HandleSelectedNodesChanged);

    GraphEditorWidget = SNew(SGraphEditor)
        .IsEditable(true)
        .AdditionalCommands(GraphEditorCommands)
        .Appearance(AppearanceInfo)
        .GraphToEdit(EditingGraph)
        .GraphEvents(GraphEditorEvents);
}

void FDreamFlowEditorToolkit::BindCommands()
{
    GraphEditorCommands = MakeShared<FUICommandList>();

    GraphEditorCommands->MapAction(
        FGenericCommands::Get().Delete,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::DeleteSelectedNodes),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanDeleteSelectedNodes));

    GraphEditorCommands->MapAction(
        FGenericCommands::Get().SelectAll,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::SelectAllNodes));
}

void FDreamFlowEditorToolkit::HandleSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
    if (!DetailsView.IsValid())
    {
        return;
    }

    if (NewSelection.Num() == 1)
    {
        UObject* SelectedObject = *NewSelection.CreateConstIterator();
        if (const UDreamFlowEdGraphNode* SelectedNode = Cast<UDreamFlowEdGraphNode>(SelectedObject))
        {
            OpenNodeEditor(SelectedNode->GetRuntimeNode());
            return;
        }
    }

    DetailsView->SetObject(FlowAsset);
}

void FDreamFlowEditorToolkit::HandleGraphChanged(const FEdGraphEditAction& Action)
{
    (void)Action;
    FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowAsset);
    RefreshValidation();
}

void FDreamFlowEditorToolkit::DeleteSelectedNodes()
{
    if (!GraphEditorWidget.IsValid())
    {
        return;
    }

    const FScopedTransaction Transaction(LOCTEXT("DeleteFlowNodes", "Delete Dream Flow Node"));
    const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    GraphEditorWidget->ClearSelectionSet();

    for (UObject* SelectedObject : SelectedNodes)
    {
        UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject);
        if (Node == nullptr || !Node->CanUserDeleteNode())
        {
            continue;
        }

        Node->Modify();
        if (Node->GetSchema() != nullptr)
        {
            Node->GetSchema()->BreakNodeLinks(*Node);
        }
        Node->DestroyNode();
    }

    FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowAsset);
}

bool FDreamFlowEditorToolkit::CanDeleteSelectedNodes() const
{
    if (!GraphEditorWidget.IsValid())
    {
        return false;
    }

    const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    for (UObject* SelectedObject : SelectedNodes)
    {
        if (const UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject))
        {
            if (Node->CanUserDeleteNode())
            {
                return true;
            }
        }
    }

    return false;
}

void FDreamFlowEditorToolkit::SelectAllNodes()
{
    if (GraphEditorWidget.IsValid())
    {
        GraphEditorWidget->SelectAllNodes();
    }
}

void FDreamFlowEditorToolkit::CreateNodeFromPalette(TSubclassOf<UDreamFlowNode> NodeClass)
{
    if (EditingGraph == nullptr || !GraphEditorWidget.IsValid() || NodeClass == nullptr)
    {
        return;
    }

PRAGMA_DISABLE_DEPRECATION_WARNINGS
    const FVector2D PasteLocation = GraphEditorWidget->GetPasteLocation();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
    UDreamFlowEdGraphNode* NewNode = FDreamFlowEditorUtils::CreateNodeInGraph(
        EditingGraph,
        NodeClass,
        FVector2f(PasteLocation.X, PasteLocation.Y),
        nullptr,
        true);

    if (NewNode != nullptr)
    {
        GraphEditorWidget->ClearSelectionSet();
        GraphEditorWidget->SetNodeSelection(NewNode, true);
        GraphEditorWidget->JumpToNode(NewNode, false);
        RefreshValidation();
    }
}

void FDreamFlowEditorToolkit::JumpToNodeGuid(const FGuid& NodeGuid)
{
    UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(EditingGraph);
    if (FlowGraph == nullptr || !GraphEditorWidget.IsValid() || !NodeGuid.IsValid())
    {
        return;
    }

    TArray<UDreamFlowEdGraphNode*> GraphNodes;
    FlowGraph->GetNodesOfClass(GraphNodes);

    for (UDreamFlowEdGraphNode* GraphNode : GraphNodes)
    {
        if (GraphNode == nullptr || GraphNode->GetRuntimeNode() == nullptr)
        {
            continue;
        }

        if (GraphNode->GetRuntimeNode()->NodeGuid == NodeGuid)
        {
            GraphEditorWidget->ClearSelectionSet();
            GraphEditorWidget->SetNodeSelection(GraphNode, true);
            GraphEditorWidget->JumpToNode(GraphNode, false);
            OpenNodeEditor(GraphNode->GetRuntimeNode());
            break;
        }
    }
}

void FDreamFlowEditorToolkit::OpenNodeEditor(UObject* ObjectToEdit)
{
    if (DetailsView.IsValid() && ObjectToEdit != nullptr)
    {
        DetailsView->SetObject(ObjectToEdit);
    }

    if (TSharedPtr<FTabManager> ToolkitTabManager = GetTabManager())
    {
        ToolkitTabManager->TryInvokeTab(DetailsTabId);
    }
}

void FDreamFlowEditorToolkit::RefreshValidation()
{
    if (FlowAsset == nullptr)
    {
        ValidationMessages.Reset();
    }
    else
    {
        FlowAsset->ValidateFlow(ValidationMessages);
    }

    if (ValidationWidget.IsValid())
    {
        ValidationWidget->SetMessages(ValidationMessages);
    }
}

#undef LOCTEXT_NAMESPACE
