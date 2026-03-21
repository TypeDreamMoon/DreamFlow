#pragma once

#include "CoreMinimal.h"

class UDreamFlowAsset;
class UBlueprint;
class UDreamFlowEdGraph;
class UDreamFlowEdGraphNode;
class UDreamFlowEdGraphRerouteNode;
class UDreamFlowNode;
class UEdGraph;
class UEdGraphPin;

class DREAMFLOWEDITOR_API FDreamFlowEditorUtils
{
public:
    static UDreamFlowEdGraph* GetOrCreateGraph(UDreamFlowAsset* FlowAsset);
    static UDreamFlowAsset* GetFlowAssetFromGraph(const UEdGraph* Graph);
    static TSubclassOf<UDreamFlowNode> PickNodeClass(UDreamFlowAsset* FlowAsset = nullptr);
    static FString GetCurrentContentBrowserPath();
    static UBlueprint* CreateNodeBlueprintAsset(
        TSubclassOf<UDreamFlowNode> NodeClass,
        const FString& TargetPath = FString(),
        bool bOpenInEditor = true);
    static FVector2f GetSuggestedNodeLocation(const UEdGraph* Graph);
    static UDreamFlowEdGraphNode* CreateNodeInGraph(
        UEdGraph* Graph,
        TSubclassOf<UDreamFlowNode> NodeClass,
        const FVector2f& Location,
        UEdGraphPin* FromPin = nullptr,
        bool bSelectNewNode = true);
    static UDreamFlowEdGraphNode* QuickCreateNode(
        UDreamFlowAsset* FlowAsset,
        TSubclassOf<UDreamFlowNode> NodeClass,
        bool bSelectNewNode = true);
    static UDreamFlowEdGraphRerouteNode* CreateRerouteNodeInGraph(
        UEdGraph* Graph,
        const FVector2f& Location,
        UEdGraphPin* FromPin = nullptr,
        bool bSelectNewNode = true);
    static void SynchronizeAssetFromGraph(UDreamFlowAsset* FlowAsset);
    static TArray<TSubclassOf<UDreamFlowNode>> GetLoadedCreatableNodeClasses(const UDreamFlowAsset* FlowAsset = nullptr);
    static bool IsNodeClassCreatable(const UClass* NodeClass, const UDreamFlowAsset* FlowAsset = nullptr);

private:
    static FString GetNodeBlueprintBaseName(const UClass* NodeClass);
    static void RebuildGraphFromAsset(UDreamFlowAsset* FlowAsset, UDreamFlowEdGraph* Graph);
    static void CreateDefaultEntryNode(UDreamFlowAsset* FlowAsset, UDreamFlowEdGraph* Graph);
};
