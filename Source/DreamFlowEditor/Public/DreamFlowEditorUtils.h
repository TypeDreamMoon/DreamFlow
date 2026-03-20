#pragma once

#include "CoreMinimal.h"

class UDreamFlowAsset;
class UDreamFlowEdGraph;
class UDreamFlowEdGraphNode;
class UDreamFlowNode;
class UEdGraph;
class UEdGraphPin;

class DREAMFLOWEDITOR_API FDreamFlowEditorUtils
{
public:
    static UDreamFlowEdGraph* GetOrCreateGraph(UDreamFlowAsset* FlowAsset);
    static UDreamFlowAsset* GetFlowAssetFromGraph(const UEdGraph* Graph);
    static UDreamFlowEdGraphNode* CreateNodeInGraph(
        UEdGraph* Graph,
        TSubclassOf<UDreamFlowNode> NodeClass,
        const FVector2f& Location,
        UEdGraphPin* FromPin = nullptr,
        bool bSelectNewNode = true);
    static void SynchronizeAssetFromGraph(UDreamFlowAsset* FlowAsset);
    static TArray<TSubclassOf<UDreamFlowNode>> GetLoadedCreatableNodeClasses();
    static bool IsNodeClassCreatable(const UClass* NodeClass);

private:
    static void RebuildGraphFromAsset(UDreamFlowAsset* FlowAsset, UDreamFlowEdGraph* Graph);
    static void CreateDefaultEntryNode(UDreamFlowAsset* FlowAsset, UDreamFlowEdGraph* Graph);
};
