#pragma once

#include "CoreMinimal.h"
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
