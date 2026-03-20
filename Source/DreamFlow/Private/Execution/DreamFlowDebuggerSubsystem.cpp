#include "Execution/DreamFlowDebuggerSubsystem.h"

#include "DreamFlowAsset.h"
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

const FDreamFlowDebuggerChanged& UDreamFlowDebuggerSubsystem::OnDebuggerChanged() const
{
    return DebuggerChanged;
}

void UDreamFlowDebuggerSubsystem::CleanupExecutors()
{
    ActiveExecutors.RemoveAll([](const TWeakObjectPtr<UDreamFlowExecutor>& WeakExecutor)
    {
        return !WeakExecutor.IsValid();
    });
}
