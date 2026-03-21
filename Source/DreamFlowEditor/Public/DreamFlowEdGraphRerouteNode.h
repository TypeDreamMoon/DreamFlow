#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "DreamFlowEdGraphRerouteNode.generated.h"

class INameValidatorInterface;

UCLASS()
class DREAMFLOWEDITOR_API UDreamFlowEdGraphRerouteNode : public UEdGraphNode
{
    GENERATED_BODY()

public:
    UDreamFlowEdGraphRerouteNode();

    virtual void AllocateDefaultPins() override;
    virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
    virtual bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
    virtual bool ShouldOverridePinNames() const override { return true; }
    virtual FText GetPinNameOverride(const UEdGraphPin& Pin) const override;
    virtual void OnRenameNode(const FString& NewName) override;
    virtual TSharedPtr<INameValidatorInterface> MakeNameValidator() const override;
    virtual bool CanSplitPin(const UEdGraphPin* Pin) const override;
    virtual UEdGraphPin* GetPassThroughPin(const UEdGraphPin* FromPin) const override;
    virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override;
    virtual TSharedPtr<class SGraphNode> CreateVisualWidget() override;
    virtual void NodeConnectionListChanged() override;
    virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
#if WITH_EDITOR
    virtual void PostEditUndo() override;
#endif
    virtual void DestroyNode() override;

    UEdGraphPin* GetInputPin() const;
    UEdGraphPin* GetOutputPin() const;

private:
    void SyncOwningAsset() const;
};
