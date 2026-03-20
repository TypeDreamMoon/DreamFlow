#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "DreamFlowDebuggerSubsystem.generated.h"

class UDreamFlowAsset;
class UDreamFlowExecutor;

DECLARE_MULTICAST_DELEGATE(FDreamFlowDebuggerChanged);

UCLASS()
class DREAMFLOW_API UDreamFlowDebuggerSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    void RegisterExecutor(UDreamFlowExecutor* Executor);
    void UnregisterExecutor(UDreamFlowExecutor* Executor);
    void NotifyExecutorChanged(UDreamFlowExecutor* Executor);

    TArray<UDreamFlowExecutor*> GetExecutorsForAsset(const UDreamFlowAsset* FlowAsset);
    const FDreamFlowDebuggerChanged& OnDebuggerChanged() const;

private:
    void CleanupExecutors();

private:
    TArray<TWeakObjectPtr<UDreamFlowExecutor>> ActiveExecutors;
    FDreamFlowDebuggerChanged DebuggerChanged;
};
