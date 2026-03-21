#pragma once

#include "CoreMinimal.h"
#include "DreamFlowVariableTypes.h"
#include "UObject/Object.h"
#include "DreamFlowVariablesEditorData.generated.h"

UCLASS(Transient)
class DREAMFLOWEDITOR_API UDreamFlowVariablesEditorData : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Variables")
    TArray<FDreamFlowVariableDefinition> Variables;

    int32 SelectedVariableIndex = INDEX_NONE;
};
