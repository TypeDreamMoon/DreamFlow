#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Logging/LogVerbosity.h"
#include "DreamFlowSettings.generated.h"

UENUM(BlueprintType)
enum class EDreamFlowLogVerbositySetting : uint8
{
    Off,
    Error,
    Warning,
    Display,
    Log,
    Verbose,
    VeryVerbose,
};

enum class EDreamFlowLogChannel : uint8
{
    General,
    Execution,
    Variables,
    Replication,
    Validation,
    Tests,
};

UCLASS(Config = Game, DefaultConfig, DisplayName = "Dream Flow")
class DREAMFLOW_API UDreamFlowSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const UDreamFlowSettings* Get();

    virtual FName GetContainerName() const override { return FName(TEXT("Project")); }
    virtual FName GetCategoryName() const override { return FName(TEXT("DreamPlugin")); }
    virtual FName GetSectionName() const override { return FName(TEXT("DreamFlow")); }

#if WITH_EDITOR
    virtual FText GetSectionText() const override;
    virtual FText GetSectionDescription() const override;
#endif

    UPROPERTY(EditAnywhere, Config, Category = "Logging")
    bool bEnableLogging = true;

    UPROPERTY(EditAnywhere, Config, Category = "Logging")
    EDreamFlowLogVerbositySetting LogVerbosity = EDreamFlowLogVerbositySetting::Log;

    UPROPERTY(EditAnywhere, Config, Category = "Logging|Channels")
    bool bLogGeneral = true;

    UPROPERTY(EditAnywhere, Config, Category = "Logging|Channels")
    bool bLogExecution = true;

    UPROPERTY(EditAnywhere, Config, Category = "Logging|Channels")
    bool bLogVariables = true;

    UPROPERTY(EditAnywhere, Config, Category = "Logging|Channels")
    bool bLogReplication = true;

    UPROPERTY(EditAnywhere, Config, Category = "Logging|Channels")
    bool bLogValidation = false;

    UPROPERTY(EditAnywhere, Config, Category = "Logging|Channels")
    bool bLogAutomationTests = true;

    ELogVerbosity::Type GetRuntimeLogVerbosity() const;
    bool IsChannelEnabled(EDreamFlowLogChannel Channel) const;
    bool ShouldLog(ELogVerbosity::Type Verbosity, EDreamFlowLogChannel Channel) const;
};
