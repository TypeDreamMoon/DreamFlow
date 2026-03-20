#pragma once

#include "CoreMinimal.h"
#include "Execution/DreamFlowExecutor.h"
#include "Components/ActorComponent.h"
#include "DreamFlowExecutorComponent.generated.h"

class UDreamFlowAsset;

UCLASS(ClassGroup = (Dream), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class DREAMFLOW_API UDreamFlowExecutorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDreamFlowExecutorComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    UDreamFlowExecutor* CreateExecutor();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    bool StartFlow();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    bool RestartFlow();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    void FinishFlow();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    bool Advance();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    bool MoveToChildByIndex(int32 ChildIndex);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    bool MoveToOutputPin(FName OutputPinName);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    bool ChooseChild(UDreamFlowNode* ChildNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UDreamFlowExecutor* GetExecutor() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UDreamFlowNode* GetCurrentNode() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool HasVariable(FName VariableName) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableValue(FName VariableName, FDreamFlowValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableValue(FName VariableName, const FDreamFlowValue& InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    void ResetVariablesToDefaults();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    TObjectPtr<UDreamFlowAsset> FlowAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    TSubclassOf<UDreamFlowExecutor> ExecutorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamFlow")
    bool bAutoStart = false;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Execution")
    FDreamFlowExecutorEventSignature OnFlowStarted;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Execution")
    FDreamFlowExecutorEventSignature OnFlowFinished;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Execution")
    FDreamFlowExecutorNodeEventSignature OnNodeEntered;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Execution")
    FDreamFlowExecutorNodeEventSignature OnNodeExited;

protected:
    UFUNCTION()
    void HandleFlowStarted();

    UFUNCTION()
    void HandleFlowFinished();

    UFUNCTION()
    void HandleNodeEntered(UDreamFlowNode* Node);

    UFUNCTION()
    void HandleNodeExited(UDreamFlowNode* Node);

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowExecutor> Executor;
};
