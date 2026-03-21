#include "DreamFlowEditorToolkit.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEdGraph.h"
#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEdGraphRerouteNode.h"
#include "DreamFlowEditorCommands.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowVariablesEditorData.h"
#include "Widgets/SDreamFlowDebuggerView.h"
#include "DreamFlowNode.h"
#include "Widgets/SDreamFlowNodePalette.h"
#include "Widgets/SDreamFlowValidationView.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GraphEditAction.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Kismet2/Kismet2NameValidators.h"
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
                            .TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
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

    if (DeferredGraphRefreshHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(DeferredGraphRefreshHandle);
        DeferredGraphRefreshHandle.Reset();
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

    ExtendToolbar();
    RegenerateMenusAndToolbars();

    if (DetailsView.IsValid())
    {
        SetDetailsObject(FlowAsset);
    }

    SyncVariableEditorDataFromAsset();
    MarkValidationDirty();
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
    if (Toolkit == nullptr || !Toolkit->bHasValidationRun || Toolkit->bValidationDirty)
    {
        return false;
    }

    if (const TArray<FDreamFlowValidationMessage>* CachedMessages = Toolkit->ValidationMessagesByNodeGuid.Find(NodeGuid))
    {
        OutMessages = *CachedMessages;
    }

    return OutMessages.Num() > 0;
}

bool FDreamFlowEditorToolkit::HasCurrentValidationResults(UEdGraph* Graph)
{
    if (Graph == nullptr)
    {
        return false;
    }

    UDreamFlowAsset* Asset = FDreamFlowEditorUtils::GetFlowAssetFromGraph(Graph);
    FDreamFlowEditorToolkit* Toolkit = Asset != nullptr ? ActiveEditors.FindRef(Asset) : nullptr;
    return Toolkit != nullptr && Toolkit->bHasValidationRun && !Toolkit->bValidationDirty;
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
    MarkValidationDirty();
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
                        LOCTEXT("GraphShellSubtitle", "Author reusable logic inside {0}. Click nodes in from the palette, edit key properties directly on the graph, and use the side panels for deeper inspection."),
                        FText::FromString(FlowAsset->GetName()))
                    : LOCTEXT("GraphShellSubtitleFallback", "Author reusable logic, inspect node data, and debug execution from the surrounding panels."),
                GraphEditorWidget.ToSharedRef())
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
    VariablesDetailsArgs.bAllowSearch = false;
    VariablesDetailsArgs.ColumnWidth = 0.34f;
    VariablesDetailsArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
    VariablesDetailsView = PropertyEditorModule.CreateDetailView(VariablesDetailsArgs);

    VariablesEditorData = NewObject<UDreamFlowVariablesEditorData>(GetTransientPackage(), NAME_None, RF_Transactional);
    VariablesDetailsView->SetObject(VariablesEditorData);

    PaletteWidget = SNew(SDreamFlowNodePalette)
        .FlowAsset(FlowAsset)
        .OnNodeClassPicked(SDreamFlowNodePalette::FOnNodeClassPicked::CreateSP(this, &FDreamFlowEditorToolkit::CreateNodeFromPalette));

    DebuggerWidget = SNew(SDreamFlowDebuggerView)
        .FlowAsset(FlowAsset)
        .OnNodeGuidActivated(SDreamFlowDebuggerView::FOnNodeGuidActivated::CreateSP(this, &FDreamFlowEditorToolkit::HandleNodeGuidActivated));

    ValidationWidget = SNew(SDreamFlowValidationView)
        .OnMessageActivated(SDreamFlowValidationView::FOnMessageActivated::CreateSP(this, &FDreamFlowEditorToolkit::HandleNodeGuidActivated))
        .OnValidateRequested(SDreamFlowValidationView::FOnValidateRequested::CreateSP(this, &FDreamFlowEditorToolkit::RunValidation));

    FGraphAppearanceInfo AppearanceInfo;
    AppearanceInfo.CornerText = LOCTEXT("GraphCornerText", "DREAM FLOW");

    SGraphEditor::FGraphEditorEvents GraphEditorEvents;
    GraphEditorEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FDreamFlowEditorToolkit::HandleSelectedNodesChanged);
    GraphEditorEvents.OnVerifyTextCommit = FOnNodeVerifyTextCommit::CreateSP(this, &FDreamFlowEditorToolkit::HandleNodeVerifyTitleCommit);
    GraphEditorEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FDreamFlowEditorToolkit::HandleNodeTitleCommitted);
    GraphEditorEvents.OnSpawnNodeByShortcutAtLocation = SGraphEditor::FOnSpawnNodeByShortcutAtLocation::CreateSP(this, &FDreamFlowEditorToolkit::HandleSpawnNodeByShortcutAtLocation);

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
        FDreamFlowEditorCommands::Get().QuickCreateNode,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::QuickCreateNode),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanQuickCreateNode));

    GraphEditorCommands->MapAction(
        FDreamFlowEditorCommands::Get().ValidateFlow,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::RunValidation),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanRunValidation));

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

    GraphEditorCommands->MapAction(
        FGraphEditorCommands::Get().CreateComment,
        FExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CreateCommentNode),
        FCanExecuteAction::CreateSP(this, &FDreamFlowEditorToolkit::CanCreateComment));
}

void FDreamFlowEditorToolkit::ExtendToolbar()
{
    ToolbarExtender = MakeShared<FExtender>();
    ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        GraphEditorCommands,
        FToolBarExtensionDelegate::CreateSP(this, &FDreamFlowEditorToolkit::FillToolbar));

    AddToolbarExtender(ToolbarExtender);
}

void FDreamFlowEditorToolkit::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
    ToolbarBuilder.BeginSection("DreamFlowValidation");
    {
        ToolbarBuilder.AddToolBarButton(
            FDreamFlowEditorCommands::Get().ValidateFlow,
            NAME_None,
            LOCTEXT("ValidateToolbarLabel", "Validate"),
            LOCTEXT("ValidateToolbarTooltip", "Run DreamFlow validation for the current asset."),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.CompilerResults"));

        ToolbarBuilder.AddToolBarButton(
            FDreamFlowEditorCommands::Get().QuickCreateNode,
            NAME_None,
            LOCTEXT("QuickCreateNodeToolbarLabel", "Create Node Class"),
            LOCTEXT("QuickCreateNodeToolbarTooltip", "Pick a DreamFlow node parent class and create a Blueprint implementation asset in the current content browser path."),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
    }
    ToolbarBuilder.EndSection();
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

        if (Cast<UEdGraphNode>(SelectedObject) != nullptr)
        {
            return;
        }
    }

    if (NewSelection.Num() > 1)
    {
        return;
    }

    if (NewSelection.Num() == 0
        && CurrentDetailsObject.IsValid()
        && CurrentDetailsObject.Get() != FlowAsset
        && CurrentDetailsObject->IsIn(FlowAsset))
    {
        return;
    }

    SetDetailsObject(FlowAsset);
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

    if (ObjectBeingModified->IsIn(EditingGraph))
    {
        const UEdGraphNode* GraphNode = Cast<UEdGraphNode>(ObjectBeingModified);
        if (GraphNode != nullptr
            && Cast<UDreamFlowEdGraphNode>(GraphNode) == nullptr)
        {
            return;
        }
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

    if (UDreamFlowNode* RuntimeNode = Cast<UDreamFlowNode>(ObjectBeingModified))
    {
        QueueDeferredNodeReconstruction(RuntimeNode);
    }
    else if (UDreamFlowNode* OwningRuntimeNode = ObjectBeingModified->GetTypedOuter<UDreamFlowNode>())
    {
        QueueDeferredNodeReconstruction(OwningRuntimeNode);
    }

    bValidationDirty = true;
    RefreshValidation();
    RequestDeferredGraphRefresh();
}

void FDreamFlowEditorToolkit::QueueDeferredNodeReconstruction(UDreamFlowNode* RuntimeNode)
{
    if (RuntimeNode == nullptr)
    {
        return;
    }

    PendingNodeReconstructions.RemoveAll([](const TWeakObjectPtr<UDreamFlowNode>& WeakNode)
    {
        return !WeakNode.IsValid();
    });

    PendingNodeReconstructions.AddUnique(RuntimeNode);
}

void FDreamFlowEditorToolkit::RequestDeferredGraphRefresh()
{
    if (DeferredGraphRefreshHandle.IsValid())
    {
        return;
    }

    // Rebuilding the graph immediately from a PropertyEditor callback can invalidate
    // inline property widgets while array mutations are still being processed.
    DeferredGraphRefreshHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateRaw(this, &FDreamFlowEditorToolkit::HandleDeferredGraphRefresh),
        0.0f);
}

bool FDreamFlowEditorToolkit::HandleDeferredGraphRefresh(float DeltaTime)
{
    (void)DeltaTime;

    DeferredGraphRefreshHandle.Reset();

    if (UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(EditingGraph))
    {
        TArray<UDreamFlowEdGraphNode*> GraphNodes;
        FlowGraph->GetNodesOfClass(GraphNodes);

        for (const TWeakObjectPtr<UDreamFlowNode>& WeakRuntimeNode : PendingNodeReconstructions)
        {
            UDreamFlowNode* RuntimeNode = WeakRuntimeNode.Get();
            if (RuntimeNode == nullptr)
            {
                continue;
            }

            for (UDreamFlowEdGraphNode* GraphNode : GraphNodes)
            {
                if (GraphNode != nullptr && GraphNode->GetRuntimeNode() == RuntimeNode)
                {
                    GraphNode->ReconstructNode();
                    break;
                }
            }
        }
    }

    PendingNodeReconstructions.Reset();

    if (GraphEditorWidget.IsValid())
    {
        TGuardValue<bool> RefreshGuard(bIsRefreshingValidationGraph, true);
        GraphEditorWidget->NotifyGraphChanged();
    }

    return false;
}

bool FDreamFlowEditorToolkit::HandleNodeVerifyTitleCommit(const FText& NewText, UEdGraphNode* NodeBeingChanged, FText& OutErrorMessage)
{
    if (NodeBeingChanged == nullptr || !NodeBeingChanged->GetCanRenameNode())
    {
        return false;
    }

    NodeBeingChanged->ErrorMsg.Empty();
    NodeBeingChanged->bHasCompilerMessage = false;

    const TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(NodeBeingChanged);
    if (!NameValidator.IsValid())
    {
        return true;
    }

    const FString NewName = NewText.ToString();
    const EValidatorResult OriginalValidationResult = NameValidator->IsValid(NewName, true);
    if (OriginalValidationResult == EValidatorResult::Ok)
    {
        return true;
    }

    const EValidatorResult ValidationResult = NameValidator->IsValid(NewName, false);
    const FText ErrorText = INameValidatorInterface::GetErrorText(NewName, ValidationResult);
    OutErrorMessage = ErrorText;
    NodeBeingChanged->bHasCompilerMessage = true;
    NodeBeingChanged->ErrorMsg = ErrorText.ToString();
    NodeBeingChanged->ErrorType = EMessageSeverity::Error;
    return false;
}

void FDreamFlowEditorToolkit::HandleNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
    (void)CommitInfo;

    if (NodeBeingChanged == nullptr)
    {
        return;
    }

    const FScopedTransaction Transaction(LOCTEXT("RenameDreamFlowGraphNode", "Rename Node"));
    NodeBeingChanged->Modify();
    NodeBeingChanged->OnRenameNode(NewText.ToString());
}

void FDreamFlowEditorToolkit::HandleGraphChanged(const FEdGraphEditAction& Action)
{
    if (bIsRefreshingValidationGraph)
    {
        return;
    }

    if (!ShouldSynchronizeGraphChange(Action))
    {
        return;
    }

    FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FlowAsset);
    MarkValidationDirty();
}

bool FDreamFlowEditorToolkit::ShouldSynchronizeGraphChange(const FEdGraphEditAction& Action) const
{
    const bool bSelectOnly = (Action.Action & GRAPHACTION_SelectNode) != 0
        && (Action.Action & (GRAPHACTION_AddNode | GRAPHACTION_RemoveNode | GRAPHACTION_EditNode)) == 0;
    if (bSelectOnly)
    {
        return false;
    }

    if (Action.Nodes.IsEmpty())
    {
        return true;
    }

    for (const UEdGraphNode* Node : Action.Nodes)
    {
        if (Cast<UDreamFlowEdGraphNode>(Node) != nullptr || Cast<UDreamFlowEdGraphRerouteNode>(Node) != nullptr)
        {
            return true;
        }
    }

    return false;
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
    MarkValidationDirty();
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
    MarkValidationDirty();
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
        GraphEditorWidget->NotifyGraphChanged();
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

void FDreamFlowEditorToolkit::CreateCommentNode()
{
    if (!GraphEditorWidget.IsValid())
    {
        return;
    }

    if (UEdGraph* Graph = GraphEditorWidget->GetCurrentGraph())
    {
        if (const UEdGraphSchema* Schema = Graph->GetSchema())
        {
            if (TSharedPtr<FEdGraphSchemaAction> Action = Schema->GetCreateCommentAction())
            {
                Action->PerformAction(Graph, nullptr, FVector2f::ZeroVector);
            }
        }
    }
}

bool FDreamFlowEditorToolkit::CanCreateComment() const
{
    return GraphEditorWidget.IsValid() && GraphEditorWidget->GetCurrentGraph() != nullptr;
}

FReply FDreamFlowEditorToolkit::HandleSpawnNodeByShortcutAtLocation(FInputChord InChord, const FVector2f& GraphPosition)
{
    (void)GraphPosition;

    if (InChord.Key == EKeys::C && !InChord.HasAnyModifierKeys())
    {
        CreateCommentNode();
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

void FDreamFlowEditorToolkit::QuickCreateNode()
{
    const TSubclassOf<UDreamFlowNode> NodeClass = FDreamFlowEditorUtils::PickNodeClass();
    if (NodeClass != nullptr)
    {
        FDreamFlowEditorUtils::CreateNodeBlueprintAsset(
            NodeClass,
            FDreamFlowEditorUtils::GetCurrentContentBrowserPath(),
            true);
    }
}

bool FDreamFlowEditorToolkit::CanQuickCreateNode() const
{
    return true;
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
        MarkValidationDirty();
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
    SetDetailsObject(ObjectToEdit);

    if (TSharedPtr<FTabManager> ToolkitTabManager = GetTabManager())
    {
        ToolkitTabManager->TryInvokeTab(DetailsTabId);
    }
}

void FDreamFlowEditorToolkit::SetDetailsObject(UObject* ObjectToEdit)
{
    if (!DetailsView.IsValid() || ObjectToEdit == nullptr)
    {
        return;
    }

    if (CurrentDetailsObject.Get() == ObjectToEdit)
    {
        return;
    }

    CurrentDetailsObject = ObjectToEdit;
    DetailsView->SetObject(ObjectToEdit);
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
    MarkValidationDirty();
}

void FDreamFlowEditorToolkit::MarkValidationDirty()
{
    bValidationDirty = true;
    RefreshValidation();

    if (GraphEditorWidget.IsValid())
    {
        TGuardValue<bool> RefreshGuard(bIsRefreshingValidationGraph, true);
        GraphEditorWidget->NotifyGraphChanged();
    }
}

void FDreamFlowEditorToolkit::RunValidation()
{
    if (FlowAsset == nullptr)
    {
        ValidationMessages.Reset();
        ValidationMessagesByNodeGuid.Reset();
        bHasValidationRun = false;
        bValidationDirty = true;
    }
    else
    {
        FlowAsset->ValidateFlow(ValidationMessages);
        ValidationMessagesByNodeGuid.Reset();
        for (const FDreamFlowValidationMessage& Message : ValidationMessages)
        {
            if (Message.NodeGuid.IsValid())
            {
                ValidationMessagesByNodeGuid.FindOrAdd(Message.NodeGuid).Add(Message);
            }
        }
        bHasValidationRun = true;
        bValidationDirty = false;
    }

    RefreshValidation();

    if (GraphEditorWidget.IsValid())
    {
        TGuardValue<bool> RefreshGuard(bIsRefreshingValidationGraph, true);
        GraphEditorWidget->NotifyGraphChanged();
    }
}

bool FDreamFlowEditorToolkit::CanRunValidation() const
{
    return FlowAsset != nullptr;
}

void FDreamFlowEditorToolkit::RefreshValidation()
{
    if (ValidationWidget.IsValid())
    {
        ValidationWidget->SetValidationState(ValidationMessages, bHasValidationRun, bValidationDirty);
    }
}

#undef LOCTEXT_NAMESPACE
