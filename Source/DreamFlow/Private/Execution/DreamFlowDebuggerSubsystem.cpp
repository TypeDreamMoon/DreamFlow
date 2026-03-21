#include "Execution/DreamFlowDebuggerSubsystem.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "Execution/DreamFlowExecutor.h"

void UDreamFlowDebuggerSubsystem::RegisterExecutor(UDreamFlowExecutor* Executor)
{
    CleanupExecutors();

    if (Executor != nullptr)
    {
        ActiveExecutors.AddUnique(Executor);
        DebuggerChanged.Broadcast();
    }
}

void UDreamFlowDebuggerSubsystem::UnregisterExecutor(UDreamFlowExecutor* Executor)
{
    const int32 RemovedCount = ActiveExecutors.Remove(Executor);
    ClearCurrentExecutionLocationForExecutor(Executor);
    CleanupExecutors();

    if (RemovedCount > 0)
    {
        DebuggerChanged.Broadcast();
    }
}

void UDreamFlowDebuggerSubsystem::NotifyExecutorChanged(UDreamFlowExecutor* Executor)
{
    CleanupExecutors();

    if (Executor != nullptr)
    {
        ActiveExecutors.AddUnique(Executor);

        if (Executor->IsPaused() && Executor->GetCurrentNode() != nullptr)
        {
            const bool bPreserveBreakpointHit =
                CurrentExecutionLocation.Executor.Get() == Executor
                && CurrentExecutionLocation.NodeGuid == Executor->GetCurrentNode()->NodeGuid
                && CurrentExecutionLocation.bIsBreakpointHit;
            SetCurrentExecutionLocation(Executor, Executor->GetCurrentNode(), bPreserveBreakpointHit);
        }
        else
        {
            ClearCurrentExecutionLocationForExecutor(Executor);
        }
    }

    DebuggerChanged.Broadcast();
}

void UDreamFlowDebuggerSubsystem::NotifyExecutionPaused(UDreamFlowExecutor* Executor, const UDreamFlowNode* Node, bool bHitBreakpoint)
{
    CleanupExecutors();

    if (Executor != nullptr)
    {
        ActiveExecutors.AddUnique(Executor);
        SetCurrentExecutionLocation(Executor, Node, bHitBreakpoint);

        if (bHitBreakpoint && CurrentExecutionLocation.IsValid())
        {
            MostRecentBreakpointHit = CurrentExecutionLocation;
            ++BreakpointHitSerial;
        }
    }

    DebuggerChanged.Broadcast();
}

TArray<UDreamFlowExecutor*> UDreamFlowDebuggerSubsystem::GetExecutorsForAsset(const UDreamFlowAsset* FlowAsset)
{
    CleanupExecutors();

    TArray<UDreamFlowExecutor*> Result;
    for (const TWeakObjectPtr<UDreamFlowExecutor>& WeakExecutor : ActiveExecutors)
    {
        UDreamFlowExecutor* Executor = WeakExecutor.Get();
        if (Executor != nullptr && (FlowAsset == nullptr || Executor->GetFlowAsset() == FlowAsset))
        {
            Result.Add(Executor);
        }
    }

    return Result;
}

bool UDreamFlowDebuggerSubsystem::IsNodeCurrentExecutionLocation(const UDreamFlowAsset* FlowAsset, const FGuid& NodeGuid, bool& bOutIsBreakpointHit) const
{
    bOutIsBreakpointHit = false;

    if (FlowAsset == nullptr || !NodeGuid.IsValid() || !CurrentExecutionLocation.IsValid())
    {
        return false;
    }

    const bool bMatches =
        CurrentExecutionLocation.FlowAsset.Get() == FlowAsset
        && CurrentExecutionLocation.NodeGuid == NodeGuid;

    if (bMatches)
    {
        bOutIsBreakpointHit = CurrentExecutionLocation.bIsBreakpointHit;
    }

    return bMatches;
}

bool UDreamFlowDebuggerSubsystem::GetMostRecentBreakpointHit(FDreamFlowExecutionLocation& OutLocation) const
{
    if (!MostRecentBreakpointHit.IsValid())
    {
        OutLocation.Reset();
        return false;
    }

    OutLocation = MostRecentBreakpointHit;
    return true;
}

uint64 UDreamFlowDebuggerSubsystem::GetBreakpointHitSerial() const
{
    return BreakpointHitSerial;
}

const FDreamFlowDebuggerChanged& UDreamFlowDebuggerSubsystem::OnDebuggerChanged() const
{
    return DebuggerChanged;
}

void UDreamFlowDebuggerSubsystem::SetCurrentExecutionLocation(UDreamFlowExecutor* Executor, const UDreamFlowNode* Node, bool bHitBreakpoint)
{
    if (Executor == nullptr || Node == nullptr || Executor->GetFlowAsset() == nullptr || !Node->NodeGuid.IsValid())
    {
        return;
    }

    CurrentExecutionLocation.FlowAsset = Executor->GetFlowAsset();
    CurrentExecutionLocation.Executor = Executor;
    CurrentExecutionLocation.NodeGuid = Node->NodeGuid;
    CurrentExecutionLocation.bIsBreakpointHit = bHitBreakpoint;
}

void UDreamFlowDebuggerSubsystem::ClearCurrentExecutionLocationForExecutor(UDreamFlowExecutor* Executor)
{
    if (Executor != nullptr && CurrentExecutionLocation.Executor.Get() == Executor)
    {
        CurrentExecutionLocation.Reset();
    }
}

void UDreamFlowDebuggerSubsystem::CleanupExecutors()
{
    ActiveExecutors.RemoveAll([](const TWeakObjectPtr<UDreamFlowExecutor>& WeakExecutor)
    {
        return !WeakExecutor.IsValid();
    });

    if (!CurrentExecutionLocation.IsValid())
    {
        CurrentExecutionLocation.Reset();
    }

    if (!MostRecentBreakpointHit.IsValid())
    {
        MostRecentBreakpointHit.Reset();
    }
}
