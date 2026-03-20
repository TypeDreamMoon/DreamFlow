#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "UObject/Object.h"
#include "DreamFlowAsset.generated.h"

class UDreamFlowNode;
class UEdGraph;

UCLASS(BlueprintType)
class DREAMFLOW_API UDreamFlowAsset : public UObject
{
    GENERATED_BODY()

public:
    UDreamFlowAsset();

    UPROPERTY(VisibleAnywhere, Instanced, BlueprintReadOnly, Category = "Flow")
    TObjectPtr<UDreamFlowNode> EntryNode;

    UPROPERTY(VisibleAnywhere, Instanced, BlueprintReadOnly, Category = "Flow")
    TArray<TObjectPtr<UDreamFlowNode>> Nodes;

#if WITH_EDITORONLY_DATA
    UPROPERTY(Instanced)
    TObjectPtr<UEdGraph> EditorGraph;

    UPROPERTY()
    TArray<FGuid> BreakpointNodeGuids;
#endif

    UFUNCTION(BlueprintPure, Category = "Flow")
    UDreamFlowNode* GetEntryNode() const;

    UFUNCTION(BlueprintPure, Category = "Flow")
    TArray<UDreamFlowNode*> GetNodesCopy() const;

    UFUNCTION(BlueprintPure, Category = "Flow")
    UDreamFlowNode* FindNodeByGuid(const FGuid& InNodeGuid) const;

    UFUNCTION(BlueprintCallable, Category = "Flow|Validation")
    void ValidateFlow(TArray<FDreamFlowValidationMessage>& OutMessages) const;

    bool HasBreakpointOnNode(const FGuid& InNodeGuid) const;
    void SetBreakpointOnNode(const FGuid& InNodeGuid, bool bEnabled);
    const TArray<FGuid>& GetBreakpointNodeGuids() const;

    void ReplaceNodes(const TArray<UDreamFlowNode*>& InNodes);
    void SetEntryNodeInternal(UDreamFlowNode* InEntryNode);

    const TArray<TObjectPtr<UDreamFlowNode>>& GetNodes() const;

#if WITH_EDITOR
    UEdGraph* GetEditorGraph() const;
    void SetEditorGraph(UEdGraph* InEditorGraph);
#endif
};
