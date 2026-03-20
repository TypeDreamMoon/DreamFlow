#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "DreamFlowEdGraphNode.generated.h"

class UDreamFlowNode;

UCLASS()
class DREAMFLOWEDITOR_API UDreamFlowEdGraphNode : public UEdGraphNode
{
    GENERATED_BODY()

public:
    static const FName InputPinName;
    static const FName OutputPinName;
    static const FName PinCategory;

    UDreamFlowEdGraphNode();

    UPROPERTY()
    TObjectPtr<UDreamFlowNode> RuntimeNode;

    virtual void AllocateDefaultPins() override;
    virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetTooltipText() const override;
    virtual bool CanDuplicateNode() const override;
    virtual bool CanUserDeleteNode() const override;
    virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
    virtual bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
    virtual TSharedPtr<class SGraphNode> CreateVisualWidget() override;
    virtual void PrepareForCopying() override;
    virtual void PostPasteNode() override;
    virtual void NodeConnectionListChanged() override;
    virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
#if WITH_EDITOR
    virtual void PostEditUndo() override;
#endif
    virtual void DestroyNode() override;

    void SetRuntimeNode(UDreamFlowNode* InRuntimeNode);
    UDreamFlowNode* GetRuntimeNode() const;
    bool IsEntryNode() const;
    bool HasBreakpoint() const;
    void SetBreakpointEnabled(bool bEnabled);
    void ToggleBreakpoint();
    void RestoreRuntimeNodeOwner();

private:
    void SyncOwningAsset() const;
};
