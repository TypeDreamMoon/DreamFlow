#pragma once

#include "CoreMinimal.h"
#include "DreamFlowVariableTypes.h"
#include "DreamFlowTypes.generated.h"

UENUM(BlueprintType)
enum class EDreamFlowValidationSeverity : uint8
{
    Info,
    Warning,
    Error,
};

UENUM(BlueprintType)
enum class EDreamFlowExecutorDebugState : uint8
{
    Idle,
    Running,
    Waiting,
    Paused,
    Finished,
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowValidationMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    EDreamFlowValidationSeverity Severity = EDreamFlowValidationSeverity::Info;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    FGuid NodeGuid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    FText NodeTitle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    FText Message;
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowReplicatedVariableValue
{
    GENERATED_BODY()

    UPROPERTY()
    FName Name;

    UPROPERTY()
    FDreamFlowValue Value;
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowReplicatedExecutionState
{
    GENERATED_BODY()

    UPROPERTY()
    EDreamFlowExecutorDebugState DebugState = EDreamFlowExecutorDebugState::Idle;

    UPROPERTY()
    FGuid CurrentNodeGuid;

    UPROPERTY()
    TArray<FGuid> VisitedNodeGuids;

    UPROPERTY()
    TArray<FDreamFlowReplicatedVariableValue> Variables;

    UPROPERTY()
    bool bPauseOnBreakpoints = true;
};
