#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "EdGraph/EdGraph.h"
#include "EditorUndoClient.h"
#include "UObject/GCObject.h"

class IDetailsView;
class SGraphEditor;
class SDreamFlowDebuggerView;
class SDreamFlowNodePalette;
class SDreamFlowValidationView;
class UDreamFlowAsset;
class UDreamFlowVariablesEditorData;
class UEdGraph;
class UDreamFlowNode;
struct FPropertyChangedEvent;
struct FEdGraphEditAction;

class DREAMFLOWEDITOR_API FDreamFlowEditorToolkit : public FAssetEditorToolkit, public FGCObject, public FEditorUndoClient
{
public:
    void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UDreamFlowAsset* InFlowAsset);
    static void OpenNodeEditorForGraph(UEdGraph* Graph, UObject* ObjectToEdit);

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
    void HandleSelectedNodesChanged(const TSet<UObject*>& NewSelection);
    void HandleObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
    void HandleGraphChanged(const FEdGraphEditAction& Action);
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
    void CreateNodeFromPalette(TSubclassOf<UDreamFlowNode> NodeClass);
    void JumpToNodeGuid(const FGuid& NodeGuid);
    void OpenNodeEditor(UObject* ObjectToEdit);
    void SyncVariableEditorDataFromAsset();
    void SyncVariablesFromEditorData();
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
    TArray<FDreamFlowValidationMessage> ValidationMessages;
    TSharedPtr<SDreamFlowNodePalette> PaletteWidget;
    TSharedPtr<SGraphEditor> GraphEditorWidget;
    TSharedPtr<IDetailsView> DetailsView;
    TSharedPtr<IDetailsView> VariablesDetailsView;
    TSharedPtr<SDreamFlowDebuggerView> DebuggerWidget;
    TSharedPtr<SDreamFlowValidationView> ValidationWidget;
    TSharedPtr<FUICommandList> GraphEditorCommands;
    FDelegateHandle GraphChangedHandle;
    FDelegateHandle ObjectPropertyChangedHandle;
    bool bIsSynchronizingVariableEditorData = false;
};
