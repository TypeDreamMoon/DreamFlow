#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "EdGraph/EdGraph.h"
#include "UObject/GCObject.h"

class IDetailsView;
class SGraphEditor;
class SDreamFlowDebuggerView;
class SDreamFlowNodePalette;
class SDreamFlowValidationView;
class UDreamFlowAsset;
class UEdGraph;
class UDreamFlowNode;
struct FPropertyChangedEvent;
struct FEdGraphEditAction;

class DREAMFLOWEDITOR_API FDreamFlowEditorToolkit : public FAssetEditorToolkit, public FGCObject
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

    virtual ~FDreamFlowEditorToolkit() override;

private:
    TSharedRef<class SDockTab> SpawnGraphTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnPaletteTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnDetailsTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnDebuggerTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnValidationTab(const class FSpawnTabArgs& Args);
    void CreateWidgets();
    void BindCommands();
    void HandleSelectedNodesChanged(const TSet<UObject*>& NewSelection);
    void HandleObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
    void HandleGraphChanged(const FEdGraphEditAction& Action);
    void DeleteSelectedNodes();
    bool CanDeleteSelectedNodes() const;
    void SelectAllNodes();
    void CreateNodeFromPalette(TSubclassOf<UDreamFlowNode> NodeClass);
    void JumpToNodeGuid(const FGuid& NodeGuid);
    void OpenNodeEditor(UObject* ObjectToEdit);
    void RefreshValidation();

private:
    static const FName PaletteTabId;
    static const FName GraphTabId;
    static const FName DetailsTabId;
    static const FName DebuggerTabId;
    static const FName ValidationTabId;
    static TMap<UDreamFlowAsset*, FDreamFlowEditorToolkit*> ActiveEditors;

    TObjectPtr<UDreamFlowAsset> FlowAsset = nullptr;
    TObjectPtr<UEdGraph> EditingGraph = nullptr;
    TArray<FDreamFlowValidationMessage> ValidationMessages;
    TSharedPtr<SDreamFlowNodePalette> PaletteWidget;
    TSharedPtr<SGraphEditor> GraphEditorWidget;
    TSharedPtr<IDetailsView> DetailsView;
    TSharedPtr<SDreamFlowDebuggerView> DebuggerWidget;
    TSharedPtr<SDreamFlowValidationView> ValidationWidget;
    TSharedPtr<FUICommandList> GraphEditorCommands;
    FDelegateHandle GraphChangedHandle;
    FDelegateHandle ObjectPropertyChangedHandle;
};
