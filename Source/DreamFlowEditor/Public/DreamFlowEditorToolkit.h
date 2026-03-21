#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "EdGraph/EdGraph.h"
#include "EditorUndoClient.h"
#include "Types/SlateEnums.h"
#include "UObject/GCObject.h"

class IDetailsView;
class SGraphEditor;
class SDreamFlowDebuggerView;
class SDreamFlowNodePalette;
class SDreamFlowValidationView;
class UDreamFlowAsset;
class UDreamFlowVariablesEditorData;
class UEdGraph;
class UEdGraphNode;
class UDreamFlowNode;
class FExtender;
class FReply;
class FToolBarBuilder;
struct FInputChord;
struct FPropertyChangedEvent;
struct FEdGraphEditAction;

class DREAMFLOWEDITOR_API FDreamFlowEditorToolkit : public FAssetEditorToolkit, public FGCObject, public FEditorUndoClient
{
public:
    void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UDreamFlowAsset* InFlowAsset);
    static void OpenNodeEditorForGraph(UEdGraph* Graph, UObject* ObjectToEdit);
    static bool OpenAssetAndFocusNode(UDreamFlowAsset* InFlowAsset, const FGuid& NodeGuid);
    static bool GetValidationMessagesForGraphNode(UEdGraph* Graph, const FGuid& NodeGuid, TArray<FDreamFlowValidationMessage>& OutMessages);
    static bool HasCurrentValidationResults(UEdGraph* Graph);

    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FText GetToolkitName() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;

    virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override;
    virtual void PostUndo(bool bSuccess) override;
    virtual void PostRedo(bool bSuccess) override;

    virtual ~FDreamFlowEditorToolkit() override;

private:
    TSharedRef<class SDockTab> SpawnGraphTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnPaletteTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnDetailsTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnVariablesTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnDebuggerTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnValidationTab(const class FSpawnTabArgs& Args);
    void CreateWidgets();
    void BindCommands();
    void ExtendToolbar();
    void FillToolbar(FToolBarBuilder& ToolbarBuilder);
    void HandleSelectedNodesChanged(const TSet<UObject*>& NewSelection);
    void HandleObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
    bool HandleNodeVerifyTitleCommit(const FText& NewText, UEdGraphNode* NodeBeingChanged, FText& OutErrorMessage);
    void HandleNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);
    void HandleGraphChanged(const FEdGraphEditAction& Action);
    bool ShouldSynchronizeGraphChange(const FEdGraphEditAction& Action) const;
    void DeleteSelectedNodes();
    void DeleteSelectedDuplicatableNodes();
    bool CanDeleteSelectedNodes() const;
    void SelectAllNodes();
    bool CanSelectAllNodes() const;
    void CopySelectedNodes();
    bool CanCopySelectedNodes() const;
    void CutSelectedNodes();
    bool CanCutSelectedNodes() const;
    void PasteNodes();
    void PasteNodesHere(const FVector2D& Location);
    bool CanPasteNodes() const;
    void DuplicateNodes();
    bool CanDuplicateNodes() const;
    void UndoGraphAction();
    void RedoGraphAction();
    void ToggleBreakpointsOnSelectedNodes();
    bool CanToggleBreakpointsOnSelectedNodes() const;
    void CreateCommentNode();
    bool CanCreateComment() const;
    FReply HandleSpawnNodeByShortcutAtLocation(FInputChord InChord, const FVector2f& GraphPosition);
    void QuickCreateNode();
    bool CanQuickCreateNode() const;
    void CreateNodeFromPalette(TSubclassOf<UDreamFlowNode> NodeClass);
    void CreateNodeFromPaletteAtPosition(TSubclassOf<UDreamFlowNode> NodeClass, const FVector2f& GraphPosition);
    void HandleNodeGuidActivated(const FGuid& NodeGuid);
    bool JumpToNodeGuid(const FGuid& NodeGuid);
    void OpenNodeEditor(UObject* ObjectToEdit);
    void SetDetailsObject(UObject* ObjectToEdit);
    void SyncVariableEditorDataFromAsset();
    void SyncVariablesFromEditorData();
    void MarkValidationDirty();
    void RunValidation();
    bool CanRunValidation() const;
    void RefreshValidation();

private:
    static const FName PaletteTabId;
    static const FName GraphTabId;
    static const FName DetailsTabId;
    static const FName VariablesTabId;
    static const FName DebuggerTabId;
    static const FName ValidationTabId;
    static TMap<UDreamFlowAsset*, FDreamFlowEditorToolkit*> ActiveEditors;

    TObjectPtr<UDreamFlowAsset> FlowAsset = nullptr;
    TObjectPtr<UDreamFlowVariablesEditorData> VariablesEditorData = nullptr;
    TObjectPtr<UEdGraph> EditingGraph = nullptr;
    TWeakObjectPtr<UObject> CurrentDetailsObject;
    TArray<FDreamFlowValidationMessage> ValidationMessages;
    TSharedPtr<SDreamFlowNodePalette> PaletteWidget;
    TSharedPtr<SGraphEditor> GraphEditorWidget;
    TSharedPtr<IDetailsView> DetailsView;
    TSharedPtr<IDetailsView> VariablesDetailsView;
    TSharedPtr<SDreamFlowDebuggerView> DebuggerWidget;
    TSharedPtr<SDreamFlowValidationView> ValidationWidget;
    TSharedPtr<FUICommandList> GraphEditorCommands;
    TSharedPtr<FExtender> ToolbarExtender;
    FDelegateHandle GraphChangedHandle;
    FDelegateHandle ObjectPropertyChangedHandle;
    bool bIsSynchronizingVariableEditorData = false;
    bool bIsRefreshingValidationGraph = false;
    bool bHasValidationRun = false;
    bool bValidationDirty = true;
};
