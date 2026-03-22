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
struct DREAMFLOW_API FDreamFlowNodeRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    FGuid NodeGuid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    TArray<FDreamFlowReplicatedVariableValue> Values;
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowSubFlowStackEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    FGuid OwnerNodeGuid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    TSoftObjectPtr<class UDreamFlowAsset> FlowAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    FGuid CurrentNodeGuid;
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowExecutionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    EDreamFlowExecutorDebugState DebugState = EDreamFlowExecutorDebugState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    FGuid CurrentNodeGuid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    TArray<FGuid> VisitedNodeGuids;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    TArray<FDreamFlowReplicatedVariableValue> Variables;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    TArray<FDreamFlowNodeRuntimeState> NodeStates;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    TArray<FDreamFlowSubFlowStackEntry> SubFlowStack;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    bool bPauseOnBreakpoints = true;
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
    TArray<FDreamFlowNodeRuntimeState> NodeStates;

    UPROPERTY()
    TArray<FDreamFlowSubFlowStackEntry> SubFlowStack;

    UPROPERTY()
    bool bPauseOnBreakpoints = true;
};
