#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNode.h"
#include "DreamFlowEntryNode.generated.h"

UCLASS(NotBlueprintable)
class DREAMFLOW_API UDreamFlowEntryNode : public UDreamFlowNode
{
    GENERATED_BODY()

public:
    UDreamFlowEntryNode();

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual bool SupportsMultipleParents_Implementation() const override;
    virtual bool IsEntryNode_Implementation() const override;
    virtual bool IsUserCreatable_Implementation() const override;
    virtual bool CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const override;
};
