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

    /** Returns true if the component's executor currently exposes a variable with this name. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool HasVariable(FName VariableName) const;

    /** Read a variable as a bool. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableBoolValue(FName VariableName, bool& OutValue) const;

    /** Read a variable as an int. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableIntValue(FName VariableName, int32& OutValue) const;

    /** Read a variable as a float. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableFloatValue(FName VariableName, float& OutValue) const;

    /** Read a variable as a name. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableNameValue(FName VariableName, FName& OutValue) const;

    /** Read a variable as a string. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableStringValue(FName VariableName, FString& OutValue) const;

    /** Read a variable as text. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableTextValue(FName VariableName, FText& OutValue) const;

    /** Read a variable as a gameplay tag. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableGameplayTagValue(FName VariableName, FGameplayTag& OutValue) const;

    /** Read a variable as an object reference. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableObjectValue(FName VariableName, UObject*& OutValue) const;

    /** Low-level raw struct variable access. Prefer typed getters in Blueprint when possible. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    bool GetVariableValue(FName VariableName, FDreamFlowValue& OutValue) const;

    /** Write a bool variable value. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableBoolValue(FName VariableName, bool InValue);

    /** Write an int variable value. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableIntValue(FName VariableName, int32 InValue);

    /** Write a float variable value. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableFloatValue(FName VariableName, float InValue);

    /** Write a name variable value. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableNameValue(FName VariableName, FName InValue);

    /** Write a string variable value. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableStringValue(FName VariableName, const FString& InValue);

    /** Write a text variable value. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableTextValue(FName VariableName, const FText& InValue);

    /** Write a gameplay tag variable value. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableGameplayTagValue(FName VariableName, FGameplayTag InValue);

    /** Write an object reference variable value. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableObjectValue(FName VariableName, UObject* InValue);

    /** Low-level raw struct variable write. Prefer typed setters in Blueprint when possible. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Low Level")
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
