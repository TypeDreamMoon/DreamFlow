#include "Execution/DreamFlowAsyncContext.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "Execution/DreamFlowExecutor.h"

bool UDreamFlowAsyncContext::IsValidAsyncContext() const
{
    return Executor != nullptr
        && Node != nullptr
        && Executor->IsWaitingForAsyncNode()
        && Executor->GetPendingAsyncNode() == Node;
}

UDreamFlowExecutor* UDreamFlowAsyncContext::GetExecutor() const
{
    return Executor;
}

UDreamFlowAsset* UDreamFlowAsyncContext::GetFlowAsset() const
{
    return Executor != nullptr ? Executor->GetFlowAsset() : nullptr;
}

UDreamFlowNode* UDreamFlowAsyncContext::GetNode() const
{
    return Node;
}

UObject* UDreamFlowAsyncContext::GetExecutionContext() const
{
    return Executor != nullptr ? Executor->GetExecutionContext() : nullptr;
}

bool UDreamFlowAsyncContext::Complete()
{
    return CompleteWithOutputPin(NAME_None);
}

bool UDreamFlowAsyncContext::CompleteWithOutputPin(FName OutputPinName)
{
    return Executor != nullptr ? Executor->CompleteAsyncNodeForNode(Node, OutputPinName) : false;
}

void UDreamFlowAsyncContext::FinishFlow()
{
    if (Executor != nullptr && Node != nullptr && Executor->GetPendingAsyncNode() == Node)
    {
        Executor->FinishFlow();
    }
}

void UDreamFlowAsyncContext::Initialize(UDreamFlowExecutor* InExecutor, UDreamFlowNode* InNode)
{
    Executor = InExecutor;
    Node = InNode;
}
