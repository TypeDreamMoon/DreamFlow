#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "DreamFlowVariableTypes.h"
#include "UObject/Object.h"
#include "DreamFlowExecutor.generated.h"

class UDreamFlowAsset;
class UDreamFlowNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDreamFlowExecutorEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDreamFlowExecutorNodeEventSignature, UDreamFlowNode*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDreamFlowExecutorStateChangedSignature, EDreamFlowExecutorDebugState, DebugState, UDreamFlowNode*, FocusNode);
DECLARE_MULTICAST_DELEGATE_OneParam(FDreamFlowExecutorRuntimeStateChangedNativeSignature, UDreamFlowExecutor*);

UCLASS(BlueprintType, Blueprintable)
class DREAMFLOW_API UDreamFlowExecutor : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    void Initialize(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext = nullptr);

    void InitializeReplicatedMirror(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext = nullptr);

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

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    bool EnterNode(UDreamFlowNode* Node);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    void SetExecutionContext(UObject* InExecutionContext);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Debug")
    bool PauseExecution();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Debug")
    bool ContinueExecution();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Debug")
    bool StepExecution();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Debug")
    void SetPauseOnBreakpoints(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UObject* GetExecutionContext() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UDreamFlowAsset* GetFlowAsset() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UDreamFlowNode* GetCurrentNode() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    TArray<UDreamFlowNode*> GetAvailableChildren() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    TArray<UDreamFlowNode*> GetVisitedNodes() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    bool IsRunning() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    bool IsPaused() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    bool GetPauseOnBreakpoints() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    EDreamFlowExecutorDebugState GetDebugState() const;

    void BuildReplicatedState(FDreamFlowReplicatedExecutionState& OutState) const;
    void ApplyReplicatedState(const FDreamFlowReplicatedExecutionState& InState);
    FDreamFlowExecutorRuntimeStateChangedNativeSignature& OnRuntimeStateChangedNative();

    /** Returns true if the executor currently exposes a variable with this name. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool HasVariable(FName VariableName) const;

    /** Read a variable as a bool. Returns false if the variable is missing or cannot be converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableBoolValue(FName VariableName, bool& OutValue) const;

    /** Read a variable as an int. Returns false if the variable is missing or cannot be converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableIntValue(FName VariableName, int32& OutValue) const;

    /** Read a variable as a float. Returns false if the variable is missing or cannot be converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableFloatValue(FName VariableName, float& OutValue) const;

    /** Read a variable as a name. Returns false if the variable is missing or cannot be converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableNameValue(FName VariableName, FName& OutValue) const;

    /** Read a variable as a string. Returns false if the variable is missing or cannot be converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableStringValue(FName VariableName, FString& OutValue) const;

    /** Read a variable as text. Returns false if the variable is missing or cannot be converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableTextValue(FName VariableName, FText& OutValue) const;

    /** Read a variable as a gameplay tag. Returns false if the variable is missing or cannot be converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableGameplayTagValue(FName VariableName, FGameplayTag& OutValue) const;

    /** Read a variable as an object reference. Returns false if the variable is missing or not an object value. */
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

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool ResolveBindingValue(const FDreamFlowValueBinding& Binding, FDreamFlowValue& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool ResolveBindingAsBool(const FDreamFlowValueBinding& Binding, bool& OutValue) const;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Execution")
    FDreamFlowExecutorEventSignature OnFlowStarted;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Execution")
    FDreamFlowExecutorEventSignature OnFlowFinished;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Execution")
    FDreamFlowExecutorNodeEventSignature OnNodeEntered;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Execution")
    FDreamFlowExecutorNodeEventSignature OnNodeExited;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Debug")
    FDreamFlowExecutorNodeEventSignature OnExecutionPaused;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Debug")
    FDreamFlowExecutorNodeEventSignature OnExecutionResumed;

    UPROPERTY(BlueprintAssignable, Category = "DreamFlow|Debug")
    FDreamFlowExecutorStateChangedSignature OnDebugStateChanged;

protected:
    bool ActivateNode(UDreamFlowNode* Node, bool bExecuteNode);
    bool ExecuteCurrentNode();
    bool ShouldPauseAtNode(const UDreamFlowNode* Node, bool& bOutHitBreakpoint);
    void BroadcastDebugStateChanged();
    void RegisterWithDebugger();
    void UnregisterFromDebugger();
    void NotifyDebuggerStateChanged();
    void ResetRuntimeState(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext, bool bNotifyDebugger);

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowAsset> FlowAsset;

    UPROPERTY(Transient)
    TObjectPtr<UObject> ExecutionContext;

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowNode> CurrentNode;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UDreamFlowNode>> VisitedNodes;

    UPROPERTY(Transient)
    TMap<FName, FDreamFlowValue> RuntimeVariables;

    UPROPERTY(Transient)
    bool bIsRunning = false;

    UPROPERTY(Transient)
    bool bIsPaused = false;

    UPROPERTY(Transient)
    bool bPauseOnBreakpoints = true;

    UPROPERTY(Transient)
    bool bBreakOnNextNode = false;

    UPROPERTY(Transient)
    bool bHasFinished = false;

private:
    FDreamFlowExecutorRuntimeStateChangedNativeSignature RuntimeStateChangedNative;
};
