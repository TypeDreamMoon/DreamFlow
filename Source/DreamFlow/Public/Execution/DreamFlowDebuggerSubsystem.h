#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "DreamFlowDebuggerSubsystem.generated.h"

class UDreamFlowAsset;
class UDreamFlowExecutor;
class UDreamFlowNode;

DECLARE_MULTICAST_DELEGATE(FDreamFlowDebuggerChanged);

struct DREAMFLOW_API FDreamFlowExecutionLocation
{
    TWeakObjectPtr<UDreamFlowAsset> FlowAsset;
    TWeakObjectPtr<UDreamFlowExecutor> Executor;
    FGuid NodeGuid;
    bool bIsBreakpointHit = false;

    bool IsValid() const
    {
        return FlowAsset.IsValid() && Executor.IsValid() && NodeGuid.IsValid();
    }

    void Reset()
    {
        FlowAsset.Reset();
        Executor.Reset();
        NodeGuid.Invalidate();
        bIsBreakpointHit = false;
    }
};

UCLASS()
class DREAMFLOW_API UDreamFlowDebuggerSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    void RegisterExecutor(UDreamFlowExecutor* Executor);
    void UnregisterExecutor(UDreamFlowExecutor* Executor);
    void NotifyExecutorChanged(UDreamFlowExecutor* Executor);
    void NotifyExecutionPaused(UDreamFlowExecutor* Executor, const UDreamFlowNode* Node, bool bHitBreakpoint);

    TArray<UDreamFlowExecutor*> GetExecutorsForAsset(const UDreamFlowAsset* FlowAsset);
    TArray<UDreamFlowExecutor*> GetExecutorsForNode(const UDreamFlowNode* Node);
    bool IsNodeCurrentExecutionLocation(const UDreamFlowAsset* FlowAsset, const FGuid& NodeGuid, bool& bOutIsBreakpointHit) const;
    bool GetMostRecentBreakpointHit(FDreamFlowExecutionLocation& OutLocation) const;
    uint64 GetBreakpointHitSerial() const;
    const FDreamFlowDebuggerChanged& OnDebuggerChanged() const;

private:
    void SetCurrentExecutionLocation(UDreamFlowExecutor* Executor, const UDreamFlowNode* Node, bool bHitBreakpoint);
    void ClearCurrentExecutionLocationForExecutor(UDreamFlowExecutor* Executor);
    void CleanupExecutors();

private:
    TArray<TWeakObjectPtr<UDreamFlowExecutor>> ActiveExecutors;
    FDreamFlowExecutionLocation CurrentExecutionLocation;
    FDreamFlowExecutionLocation MostRecentBreakpointHit;
    uint64 BreakpointHitSerial = 0;
    FDreamFlowDebuggerChanged DebuggerChanged;
};
