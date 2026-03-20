#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "DreamFlowEdGraph.generated.h"

class UDreamFlowAsset;

UCLASS()
class DREAMFLOWEDITOR_API UDreamFlowEdGraph : public UEdGraph
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<UDreamFlowAsset> FlowAsset;

    UDreamFlowAsset* GetFlowAsset() const;
    void SetFlowAsset(UDreamFlowAsset* InFlowAsset);
};
