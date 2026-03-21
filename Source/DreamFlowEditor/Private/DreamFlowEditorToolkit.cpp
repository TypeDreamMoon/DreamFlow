#include "DreamFlowEditorToolkit.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEdGraph.h"
#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEditorCommands.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowVariablesEditorData.h"
#include "Widgets/SDreamFlowDebuggerView.h"
#include "DreamFlowNode.h"
#include "Widgets/SDreamFlowGraphDropTarget.h"
#include "Widgets/SDreamFlowNodePalette.h"
#include "Widgets/SDreamFlowValidationView.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditor.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DreamFlowEditorToolkit"

namespace DreamFlowEditorToolkit
{
    static TSharedRef<SWidget> BuildTabShell(const FText& Title, const FText& Subtitle, const TSharedRef<SWidget>& Content)
    {
        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(0.0f)
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
                    .Padding(FMargin(14.0f, 12.0f))
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(Title)
                            .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
                            .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(Subtitle)
                            .TextStyle(FAppStyle::Get(), "SmallText")
                            .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                            .WrapTextAt(420.0f)
                        ]
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SSeparator)
                    .Thickness(1.0f)
                ]

                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    Content
                ]
            ];
    }
}

TMap<UDreamFlowAsset*, FDreamFlowEditorToolkit*> FDreamFlowEditorToolkit::ActiveEditors;
const FName FDreamFlowEditorToolkit::PaletteTabId(TEXT("DreamFlowEditor_Palette"));
const FName FDreamFlowEditorToolkit::GraphTabId(TEXT("DreamFlowEditor_Graph"));
const FName FDreamFlowEditorToolkit::DetailsTabId(TEXT("DreamFlowEditor_Details"));
const FName FDreamFlowEditorToolkit::VariablesTabId(TEXT("DreamFlowEditor_Variables"));
const FName FDreamFlowEditorToolkit::DebuggerTabId(TEXT("DreamFlowEditor_Debugger"));
const FName FDreamFlowEditorToolkit::ValidationTabId(TEXT("DreamFlowEditor_Validation"));

FDreamFlowEditorToolkit::~FDreamFlowEditorToolkit()
{
    if (GEditor != nullptr)
    {
        GEditor->UnregisterForUndo(this);
    }

    if (FlowAsset != nullptr)
    {
        ActiveEditors.Remove(FlowAsset);
    }

    if (EditingGraph != nullptr && GraphChangedHandle.IsValid())
    {
        EditingGraph->RemoveOnGraphChangedHandler(GraphChangedHandle);
        GraphChangedHandle.Reset();
    }

    if (ObjectPropertyChangedHandle.IsValid())
    {
        FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ObjectPropertyChangedHandle);
        ObjectPropertyChangedHandle.Reset();
    }
}

void FDreamFlowEditorToolkit::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDreamFlowAsset* InFlowAsset)
{
    FlowAsset = InFlowAsset;
    EditingGraph = FDreamFlowEditorUtils::GetOrCreateGraph(FlowAsset);
    ActiveEditors.Add(FlowAsset, this);

    BindCommands();
    CreateWidgets();

    if (GEditor != nullptr)
    {
        GEditor->RegisterForUndo(this);
    }

    if (EditingGraph != nullptr)
    {
        GraphChangedHandle = EditingGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FDreamFlowEditorToolkit::HandleGraphChanged));
    }

    if (!ObjectPropertyChangedHandle.IsValid())
    {
        ObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FDreamFlowEditorToolkit::HandleObjectPropertyChanged);
    }

    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("DreamFlowEditorLayout_v2")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewSplitter()
                ->SetOrientation(Orient_Vertical)
                ->SetSizeCoefficient(0.22f)
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.72f)
                    ->AddTab(PaletteTabId, ETabState::OpenedTab)
                    ->SetHideTabWell(true)
                )
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.28f)
                    ->AddTab(ValidationTabId, ETabState::OpenedTab)
                    ->SetHideTabWell(true)
                )
            )
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.54f)
                ->AddTab(GraphTabId, ETabState::OpenedTab)
                ->SetHideTabWell(true)
            )
            ->Split
            (
                FTabManager::NewSplitter()
                ->SetOrientation(Orient_Vertical)
                ->SetSizeCoefficient(0.24f)
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.46f)
                    ->AddTab(DetailsTabId, ETabState::OpenedTab)
                    ->SetHideTabWell(true)
                )
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.22f)
                    ->AddTab(VariablesTabId, ETabState::OpenedTab)
                    ->SetHideTabWell(true)
                )
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.32f)
                    ->AddTab(DebuggerTabId, ETabState::OpenedTab)
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

    SyncVariableEditorDataFromAsset();
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

bool FDreamFlowEditorToolkit::OpenAssetAndFocusNode(UDreamFlowAsset* InFlowAsset, const FGuid& NodeGuid)
{
    if (InFlowAsset == nullptr || !NodeGuid.IsValid() || GEditor == nullptr)
    {
        return false;
    }

    if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
    {
        AssetEditorSubsystem->OpenEditorForAsset(InFlowAsset);

        if (IAssetEditorInstance* AssetEditorInstance = AssetEditorSubsystem->FindEditorForAsset(InFlowAsset, true))
        {
            AssetEditorInstance->InvokeTab(FTabId(GraphTabId));
            AssetEditorInstance->FocusWindow(InFlowAsset);
        }
    }

    if (FDreamFlowEditorToolkit* Toolkit = ActiveEditors.FindRef(InFlowAsset))
    {
        return Toolkit->JumpToNodeGuid(NodeGuid);
    }

    return false;
}

bool FDreamFlowEditorToolkit::GetValidationMessagesForGraphNode(UEdGraph* Graph, const FGuid& NodeGuid, TArray<FDreamFlowValidationMessage>& OutMessages)
{
    OutMessages.Reset();

    if (Graph == nullptr || !NodeGuid.IsValid())
    {
        return false;
    }

    UDreamFlowAsset* Asset = FDreamFlowEditorUtils::GetFlowAssetFromGraph(Graph);
    FDreamFlowEditorToolkit* Toolkit = Asset != nullptr ? ActiveEditors.FindRef(Asset) : nullptr;
    if (Toolkit == nullptr)
    {
        return false;
    }

    for (const FDreamFlowValidationMessage& Message : Toolkit->ValidationMessages)
    {
        if (Message.NodeGuid == NodeGuid)
        {
            OutMessages.Add(Message);
        }
    }

    return OutMessages.Num() > 0;
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
        .SetDisplayName(LOCTEXT("PaletteTabLabel", "Palette"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.PlusCircle"));

    InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnGraphTab))
        .SetDisplayName(LOCTEXT("GraphTabLabel", "Graph"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

    InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnDetailsTab))
        .SetDisplayName(LOCTEXT("DetailsTabLabel", "Node Editor"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

    InTabManager->RegisterTabSpawner(VariablesTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnVariablesTab))
        .SetDisplayName(LOCTEXT("VariablesTabLabel", "Variables"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Variables"));

    InTabManager->RegisterTabSpawner(DebuggerTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnDebuggerTab))
        .SetDisplayName(LOCTEXT("DebuggerTabLabel", "Debugger"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Debugger"));

    InTabManager->RegisterTabSpawner(ValidationTabId, FOnSpawnTab::CreateSP(this, &FDreamFlowEditorToolkit::SpawnValidationTab))
        .SetDisplayName(LOCTEXT("ValidationTabLabel", "Validation"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.CompilerResults"));
}

void FDreamFlowEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
    InTabManager->UnregisterTabSpawner(PaletteTabId);
    InTabManager->UnregisterTabSpawner(GraphTabId);
    InTabManager->UnregisterTabSpawner(DetailsTabId);
    InTabManager->UnregisterTabSpawner(VariablesTabId);
    InTabManager->UnregisterTabSpawner(DebuggerTabId);
    InTabManager->UnregisterTabSpawner(ValidationTabId);
}

void FDreamFlowEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(FlowAsset);
    Collector.AddReferencedObject(VariablesEditorData);
    Collector.AddReferencedObject(EditingGraph);
}

FString FDreamFlowEditorToolkit::GetReferencerName() const
{
    return TEXT("FDreamFlowEditorToolkit");
}

void FDreamFlowEditorToolkit::PostUndo(bool bSuccess)
{
    if (!bSuccess || !GraphEditorWidget.IsValid())
    {
        return;
    }

    GraphEditorWidget->ClearSelectionSet();
    GraphEditorWidget->NotifyGraphChanged();
    SyncVariableEditorDataFromAsset();
    RefreshValidation();
    FSlateApplication::Get().DismissAllMenus();
}

void FDreamFlowEditorToolkit::PostRedo(bool bSuccess)
{
    PostUndo(bSuccess);
}

TSharedRef<SDockTab> FDreamFlowEditorToolkit::SpawnGraphTab(const FSpawnTabArgs& Args)
{
    (void)Args;
    return SNew(SDockTab)
        [
            DreamFlowEditorToolkit::BuildTabShell(
                LOCTEXT("GraphShellTitle", "Flow Graph"),
                FlowAsset != nullptr
                    ? FText::Format(
                        LOCTEXT("GraphShellSubtitle", "Author reusable logic inside {0}. Click or drag nodes in from the palette, edit key properties directly on the graph, and use the side panels for deeper inspection."),
                        FText::FromString(FlowAsset->GetName()))
                    : LOCTEXT("GraphShellSubtitleFallback", "Author reusable logic, inspect node data, and debug execution from the surrounding panels."),
                GraphDropTargetWidget.ToSharedRef())
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
            DreamFlowEditorToolkit::BuildTabShell(
                LOCTEXT("DetailsShellTitle", "Node Details"),
                LOCTEXT("DetailsShellSubtitle", "Select a graph node to edit its runtime properties, preview data, bindings, and behavior."),
                DetailsView.ToSharedRef())
        ];
}

TSharedRef<SDockTab> FDreamFlowEditorToolkit::SpawnVariablesTab(const FSpawnTabArgs& Args)
{
    (void)Args;
    return SNew(SDockTab)
        [
            DreamFlowEditorToolkit::BuildTabShell(
                LOCTEXT("VariablesShellTitle", "Flow Variables"),
                LOCTEXT("VariablesShellSubtitle", "Manage flow-scoped environment variables and defaults that bindings can read at runtime."),
                VariablesDetailsView.ToSharedRef())
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

TSharedRef<SDockTab> FDreamFlowEditorToolkit::SpawnDebuggerTab(const FSpawnTabArgs& Args)
{
    (void)Args;
    return SNew(SDockTab)
        [
            DebuggerWidget.ToSharedRef()
        ];
}

void FDreamFlowEditorToolkit::CreateWidgets()
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bUpdatesFromSelection = false;
    DetailsArgs.bLockable = false;
    DetailsArgs.bAllowSearch = true;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsArgs.bSearchInitialKeyFocus = false;
    DetailsArgs.bShowOptions = false;
    DetailsArgs.bShowPropertyMatrixButton = false;
    DetailsArgs.ColumnWidth = 0.36f;
    DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);

    FDetailsViewArgs VariablesDetailsArgs = DetailsArgs;
    VariablesDetailsArgs.ColumnWidth = 0.34f;
    VariablesDetailsArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
    VariablesDetailsView = PropertyEditorModule.CreateDetailView(VariablesDetailsArgs);

    VariablesEditorData = NewObject<UDreamFlowVariablesEditorData>(GetTransientPackage(), NAME_None, RF_Transactional);
    VariablesDetailsView->SetObject(VariablesEditorData);

    PaletteWidget = SNew(SDreamFlowNodePalette)
        .FlowAsset(FlowAsset)
        .OnNodeClassPicked(SDreamFlowNodePalette::FOnNodeClassPicked::CreateSP(this, &FDreamFlowEditorToolkit::CreateNodeFromPalette))
        .OnNodeClassDropped(SDreamFlowNodePalette::FOnNodeClassDropped::CreateSP(this, &FDreamFlowEditorToolkit::CreateNodeFromPaletteAtPosition));

    DebuggerWidget = SNew(SDreamFlowDebuggerView)
        .FlowAsset(FlowAsset)
        .OnNodeGuidActivated(SDreamFlowDebuggerView::FOnNodeGuidActivated::CreateSP(this, &FDreamFlowEditorToolkit::HandleNodeGuidActivated));

    ValidationWidget = SNew(SDreamFlowValidationView)
        .OnMessageActivated(SDreamFlowValidationView::FOnMessageActivated::CreateSP(this, &FDreamFlowEditorToolkit::HandleNodeGuidActivated));

    FGraphAppearanceInfo AppearanceInfo;
    AppearanceInfo.CornerText = LOCTEXT("GraphCornerText", "DREAM FLOW");

    SGraphEditor::FGraphEditorEvents GraphEditorEvents;
    GraphEditorEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FDreamFlowEditorToolkit::HandleSelectedNodesChanged);

    GraphEditorWidget = SNew(SGraphEditor)
        .IsEditable(true)
        .AdditionalCommands(GraphEditorCommands)
        .Appearance(AppearanceInfo)
        .GraphToEdit(EditingGraph)
        .GraphEvents(GraphEditorEvents);

    GraphDropTargetWidget = SNew(SDreamFlowGraphDropTarget)
        .GraphEditor(GraphEditorWidget)
        .Content(GraphEditorWidget)
        .OnNodeClassDropped(SDreamFlowGraphDropTarget::FOnNodeClassDropped::CreateSP(this, &FDreamFlowEditorToolkit::CreateNodeFromPaletteAtPosition));
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
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::SelectAllNodes),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanSelectAllNodes));

    GraphEditorCommands->MapAction(
        FGenericCommands::Get().Copy,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CopySelectedNodes),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanCopySelectedNodes));

    GraphEditorCommands->MapAction(
        FGenericCommands::Get().Cut,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CutSelectedNodes),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanCutSelectedNodes));

    GraphEditorCommands->MapAction(
        FGenericCommands::Get().Paste,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::PasteNodes),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanPasteNodes));

    GraphEditorCommands->MapAction(
        FGenericCommands::Get().Duplicate,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::DuplicateNodes),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanDuplicateNodes));

    GraphEditorCommands->MapAction(
        FGenericCommands::Get().Undo,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::UndoGraphAction));

    GraphEditorCommands->MapAction(
        FGenericCommands::Get().Redo,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::RedoGraphAction));

    GraphEditorCommands->MapAction(
        FDreamFlowEditorCommands::Get().ToggleBreakpoint,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::ToggleBreakpointsOnSelectedNodes),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanToggleBreakpointsOnSelectedNodes));
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

void FDreamFlowEditorToolkit::HandleObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
    (void)PropertyChangedEvent;

    if (bIsSynchronizingVariableEditorData || FlowAsset == nullptr || EditingGraph == nullptr || ObjectBeingModified == nullptr)
    {
        return;
    }

    if (VariablesEditorData != nullptr && (ObjectBeingModified == VariablesEditorData || ObjectBeingModified->IsIn(VariablesEditorData)))
    {
        SyncVariablesFromEditorData();
        return;
    }

    // Runtime nodes are instanced under the flow asset, so this catches both asset and node edits.
    if (ObjectBeingModified != FlowAsset && !ObjectBeingModified->IsIn(FlowAsset))
    {
        return;
    }

    if (ObjectBeingModified == FlowAsset)
    {
        SyncVariableEditorDataFromAsset();
    }

    EditingGraph->NotifyGraphChanged();
    RefreshValidation();
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

void FDreamFlowEditorToolkit::DeleteSelectedDuplicatableNodes()
{
    if (!GraphEditorWidget.IsValid())
    {
        return;
    }

    const FScopedTransaction Transaction(LOCTEXT("CutFlowNodes", "Cut Dream Flow Node"));
    const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    GraphEditorWidget->ClearSelectionSet();

    for (UObject* SelectedObject : SelectedNodes)
    {
        UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject);
        if (Node == nullptr || !Node->CanDuplicateNode() || !Node->CanUserDeleteNode())
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

    if (FlowAsset != nullptr)
    {
        FlowAsset->Modify();
    }

    FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowAsset);
    RefreshValidation();
}

bool FDreamFlowEditorToolkit::CanSelectAllNodes() const
{
    return GraphEditorWidget.IsValid();
}

void FDreamFlowEditorToolkit::CopySelectedNodes()
{
    if (!GraphEditorWidget.IsValid())
    {
        return;
    }

    FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    FString ExportedText;

    for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
    {
        UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
        if (Node == nullptr || !Node->CanDuplicateNode())
        {
            SelectedIter.RemoveCurrent();
            continue;
        }

        Node->PrepareForCopying();
    }

    if (SelectedNodes.Num() == 0)
    {
        return;
    }

    FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
    FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

    for (UObject* SelectedObject : SelectedNodes)
    {
        if (UDreamFlowEdGraphNode* FlowNode = Cast<UDreamFlowEdGraphNode>(SelectedObject))
        {
            FlowNode->RestoreRuntimeNodeOwner();
        }
    }
}

bool FDreamFlowEditorToolkit::CanCopySelectedNodes() const
{
    if (!GraphEditorWidget.IsValid())
    {
        return false;
    }

    const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    for (UObject* SelectedObject : SelectedNodes)
    {
        const UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject);
        if (Node != nullptr && Node->CanDuplicateNode())
        {
            return true;
        }
    }

    return false;
}

void FDreamFlowEditorToolkit::CutSelectedNodes()
{
    CopySelectedNodes();
    DeleteSelectedDuplicatableNodes();
}

bool FDreamFlowEditorToolkit::CanCutSelectedNodes() const
{
    return CanCopySelectedNodes() && CanDeleteSelectedNodes();
}

void FDreamFlowEditorToolkit::PasteNodes()
{
    if (!GraphEditorWidget.IsValid())
    {
        return;
    }

PRAGMA_DISABLE_DEPRECATION_WARNINGS
    const FVector2D PasteLocation = GraphEditorWidget->GetPasteLocation();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
    PasteNodesHere(PasteLocation);
}

void FDreamFlowEditorToolkit::PasteNodesHere(const FVector2D& Location)
{
    if (!GraphEditorWidget.IsValid() || EditingGraph == nullptr || FlowAsset == nullptr)
    {
        return;
    }

    FString TextToImport;
    FPlatformApplicationMisc::ClipboardPaste(TextToImport);
    if (TextToImport.IsEmpty())
    {
        return;
    }

    const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
    EditingGraph->Modify();
    FlowAsset->Modify();

    GraphEditorWidget->ClearSelectionSet();

    TSet<UEdGraphNode*> PastedNodes;
    FEdGraphUtilities::ImportNodesFromText(EditingGraph, TextToImport, PastedNodes);

    FVector2D AverageNodePosition(0.0f, 0.0f);
    int32 PositionCount = 0;
    for (UEdGraphNode* PastedNode : PastedNodes)
    {
        if (PastedNode != nullptr)
        {
            AverageNodePosition.X += PastedNode->NodePosX;
            AverageNodePosition.Y += PastedNode->NodePosY;
            ++PositionCount;
        }
    }

    if (PositionCount > 0)
    {
        AverageNodePosition /= static_cast<float>(PositionCount);
    }

    for (UEdGraphNode* PastedNode : PastedNodes)
    {
        if (PastedNode == nullptr)
        {
            continue;
        }

        PastedNode->NodePosX = FMath::RoundToInt((PastedNode->NodePosX - AverageNodePosition.X) + Location.X);
        PastedNode->NodePosY = FMath::RoundToInt((PastedNode->NodePosY - AverageNodePosition.Y) + Location.Y);
        PastedNode->SnapToGrid(16);
        GraphEditorWidget->SetNodeSelection(PastedNode, true);

        if (UDreamFlowEdGraphNode* FlowNode = Cast<UDreamFlowEdGraphNode>(PastedNode))
        {
            if (UDreamFlowNode* RuntimeNode = FlowNode->GetRuntimeNode())
            {
#if WITH_EDITOR
                RuntimeNode->SetEditorPosition(FVector2D(FlowNode->NodePosX, FlowNode->NodePosY));
#endif
            }
        }
    }

    FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowAsset);
    GraphEditorWidget->NotifyGraphChanged();
    RefreshValidation();
}

bool FDreamFlowEditorToolkit::CanPasteNodes() const
{
    if (!GraphEditorWidget.IsValid() || EditingGraph == nullptr)
    {
        return false;
    }

    FString ClipboardContent;
    FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
    return FEdGraphUtilities::CanImportNodesFromText(EditingGraph, ClipboardContent);
}

void FDreamFlowEditorToolkit::DuplicateNodes()
{
    CopySelectedNodes();
    PasteNodes();
}

bool FDreamFlowEditorToolkit::CanDuplicateNodes() const
{
    return CanCopySelectedNodes();
}

void FDreamFlowEditorToolkit::UndoGraphAction()
{
    if (GEditor != nullptr)
    {
        GEditor->UndoTransaction();
    }
}

void FDreamFlowEditorToolkit::RedoGraphAction()
{
    if (GEditor != nullptr)
    {
        GEditor->RedoTransaction();
    }
}

void FDreamFlowEditorToolkit::ToggleBreakpointsOnSelectedNodes()
{
    if (!GraphEditorWidget.IsValid())
    {
        return;
    }

    const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    bool bToggledAnyBreakpoint = false;

    const FScopedTransaction Transaction(LOCTEXT("ToggleDreamFlowBreakpoint", "Toggle Dream Flow Breakpoint"));

    for (UObject* SelectedObject : SelectedNodes)
    {
        if (UDreamFlowEdGraphNode* FlowGraphNode = Cast<UDreamFlowEdGraphNode>(SelectedObject))
        {
            FlowGraphNode->Modify();
            FlowGraphNode->ToggleBreakpoint();
            bToggledAnyBreakpoint = true;
        }
    }

    if (bToggledAnyBreakpoint)
    {
        RefreshValidation();
    }
}

bool FDreamFlowEditorToolkit::CanToggleBreakpointsOnSelectedNodes() const
{
    if (!GraphEditorWidget.IsValid())
    {
        return false;
    }

    const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    for (UObject* SelectedObject : SelectedNodes)
    {
        if (Cast<UDreamFlowEdGraphNode>(SelectedObject) != nullptr)
        {
            return true;
        }
    }

    return false;
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
    CreateNodeFromPaletteAtPosition(NodeClass, FVector2f(PasteLocation.X, PasteLocation.Y));
}

void FDreamFlowEditorToolkit::CreateNodeFromPaletteAtPosition(TSubclassOf<UDreamFlowNode> NodeClass, const FVector2f& GraphPosition)
{
    if (EditingGraph == nullptr || !GraphEditorWidget.IsValid() || NodeClass == nullptr)
    {
        return;
    }

    UDreamFlowEdGraphNode* NewNode = FDreamFlowEditorUtils::CreateNodeInGraph(
        EditingGraph,
        NodeClass,
        GraphPosition,
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

void FDreamFlowEditorToolkit::HandleNodeGuidActivated(const FGuid& NodeGuid)
{
    JumpToNodeGuid(NodeGuid);
}

bool FDreamFlowEditorToolkit::JumpToNodeGuid(const FGuid& NodeGuid)
{
    UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(EditingGraph);
    if (FlowGraph == nullptr || !GraphEditorWidget.IsValid() || !NodeGuid.IsValid())
    {
        return false;
    }

    FocusWindow(FlowAsset);
    if (TSharedPtr<FTabManager> ToolkitTabManager = GetTabManager())
    {
        ToolkitTabManager->TryInvokeTab(GraphTabId);
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
            return true;
        }
    }

    return false;
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

void FDreamFlowEditorToolkit::SyncVariableEditorDataFromAsset()
{
    if (FlowAsset == nullptr || VariablesEditorData == nullptr)
    {
        return;
    }

    TGuardValue<bool> SyncGuard(bIsSynchronizingVariableEditorData, true);
    VariablesEditorData->Variables = FlowAsset->Variables;

    if (VariablesDetailsView.IsValid())
    {
        VariablesDetailsView->ForceRefresh();
    }
}

void FDreamFlowEditorToolkit::SyncVariablesFromEditorData()
{
    if (FlowAsset == nullptr || VariablesEditorData == nullptr)
    {
        return;
    }

    TGuardValue<bool> SyncGuard(bIsSynchronizingVariableEditorData, true);
    FlowAsset->Modify();
    FlowAsset->Variables = VariablesEditorData->Variables;

    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }

    EditingGraph->NotifyGraphChanged();
    RefreshValidation();
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
