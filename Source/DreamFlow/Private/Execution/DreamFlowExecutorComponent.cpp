#include "Execution/DreamFlowExecutorComponent.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"

UDreamFlowExecutorComponent::UDreamFlowExecutorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDreamFlowExecutorComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoStart)
    {
        StartFlow();
    }
}

UDreamFlowExecutor* UDreamFlowExecutorComponent::CreateExecutor()
{
    if (Executor == nullptr)
    {
        UClass* EffectiveClass = ExecutorClass != nullptr ? ExecutorClass.Get() : UDreamFlowExecutor::StaticClass();
        Executor = NewObject<UDreamFlowExecutor>(this, EffectiveClass);
        Executor->OnFlowStarted.AddUniqueDynamic(this, &ThisClass::HandleFlowStarted);
        Executor->OnFlowFinished.AddUniqueDynamic(this, &ThisClass::HandleFlowFinished);
        Executor->OnNodeEntered.AddUniqueDynamic(this, &ThisClass::HandleNodeEntered);
        Executor->OnNodeExited.AddUniqueDynamic(this, &ThisClass::HandleNodeExited);
    }

    Executor->Initialize(FlowAsset, GetOwner());
    return Executor;
}

bool UDreamFlowExecutorComponent::StartFlow()
{
    UDreamFlowExecutor* RuntimeExecutor = CreateExecutor();
    return RuntimeExecutor != nullptr && RuntimeExecutor->StartFlow();
}

bool UDreamFlowExecutorComponent::RestartFlow()
{
    UDreamFlowExecutor* RuntimeExecutor = CreateExecutor();
    return RuntimeExecutor != nullptr && RuntimeExecutor->RestartFlow();
}

void UDreamFlowExecutorComponent::FinishFlow()
{
    if (Executor != nullptr)
    {
        Executor->FinishFlow();
    }
}

bool UDreamFlowExecutorComponent::Advance()
{
    return Executor != nullptr && Executor->Advance();
}

bool UDreamFlowExecutorComponent::MoveToChildByIndex(int32 ChildIndex)
{
    return Executor != nullptr && Executor->MoveToChildByIndex(ChildIndex);
}

bool UDreamFlowExecutorComponent::ChooseChild(UDreamFlowNode* ChildNode)
{
    return Executor != nullptr && Executor->ChooseChild(ChildNode);
}

UDreamFlowExecutor* UDreamFlowExecutorComponent::GetExecutor() const
{
    return Executor;
}

UDreamFlowNode* UDreamFlowExecutorComponent::GetCurrentNode() const
{
    return Executor != nullptr ? Executor->GetCurrentNode() : nullptr;
}

void UDreamFlowExecutorComponent::HandleFlowStarted()
{
    OnFlowStarted.Broadcast();
}

void UDreamFlowExecutorComponent::HandleFlowFinished()
{
    OnFlowFinished.Broadcast();
}

void UDreamFlowExecutorComponent::HandleNodeEntered(UDreamFlowNode* Node)
{
    OnNodeEntered.Broadcast(Node);
}

void UDreamFlowExecutorComponent::HandleNodeExited(UDreamFlowNode* Node)
{
    OnNodeExited.Broadcast(Node);
}
