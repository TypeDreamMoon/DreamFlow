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

bool UDreamFlowExecutorComponent::MoveToOutputPin(FName OutputPinName)
{
    return Executor != nullptr && Executor->MoveToOutputPin(OutputPinName);
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

bool UDreamFlowExecutorComponent::HasVariable(FName VariableName) const
{
    return Executor != nullptr && Executor->HasVariable(VariableName);
}

bool UDreamFlowExecutorComponent::GetVariableValue(FName VariableName, FDreamFlowValue& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableBoolValue(FName VariableName, bool& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableBoolValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableIntValue(FName VariableName, int32& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableIntValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableFloatValue(FName VariableName, float& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableFloatValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableNameValue(FName VariableName, FName& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableNameValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableStringValue(FName VariableName, FString& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableStringValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableTextValue(FName VariableName, FText& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableTextValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableGameplayTagValue(FName VariableName, FGameplayTag& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableGameplayTagValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::GetVariableObjectValue(FName VariableName, UObject*& OutValue) const
{
    return Executor != nullptr && Executor->GetVariableObjectValue(VariableName, OutValue);
}

bool UDreamFlowExecutorComponent::SetVariableValue(FName VariableName, const FDreamFlowValue& InValue)
{
    return Executor != nullptr && Executor->SetVariableValue(VariableName, InValue);
}

bool UDreamFlowExecutorComponent::SetVariableBoolValue(FName VariableName, bool InValue)
{
    return Executor != nullptr && Executor->SetVariableBoolValue(VariableName, InValue);
}

bool UDreamFlowExecutorComponent::SetVariableIntValue(FName VariableName, int32 InValue)
{
    return Executor != nullptr && Executor->SetVariableIntValue(VariableName, InValue);
}

bool UDreamFlowExecutorComponent::SetVariableFloatValue(FName VariableName, float InValue)
{
    return Executor != nullptr && Executor->SetVariableFloatValue(VariableName, InValue);
}

bool UDreamFlowExecutorComponent::SetVariableNameValue(FName VariableName, FName InValue)
{
    return Executor != nullptr && Executor->SetVariableNameValue(VariableName, InValue);
}

bool UDreamFlowExecutorComponent::SetVariableStringValue(FName VariableName, const FString& InValue)
{
    return Executor != nullptr && Executor->SetVariableStringValue(VariableName, InValue);
}

bool UDreamFlowExecutorComponent::SetVariableTextValue(FName VariableName, const FText& InValue)
{
    return Executor != nullptr && Executor->SetVariableTextValue(VariableName, InValue);
}

bool UDreamFlowExecutorComponent::SetVariableGameplayTagValue(FName VariableName, FGameplayTag InValue)
{
    return Executor != nullptr && Executor->SetVariableGameplayTagValue(VariableName, InValue);
}

bool UDreamFlowExecutorComponent::SetVariableObjectValue(FName VariableName, UObject* InValue)
{
    return Executor != nullptr && Executor->SetVariableObjectValue(VariableName, InValue);
}

void UDreamFlowExecutorComponent::ResetVariablesToDefaults()
{
    if (Executor != nullptr)
    {
        Executor->ResetVariablesToDefaults();
    }
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
