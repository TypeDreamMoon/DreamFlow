#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "DreamFlowEdGraphSchema.generated.h"

UCLASS()
class DREAMFLOWEDITOR_API UDreamFlowEdGraphSchema : public UEdGraphSchema
{
    GENERATED_BODY()

public:
    virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    virtual void GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
    virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const override;
    virtual bool TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const override;
    virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
    virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
    virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;
    virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
    virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
    virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
};
