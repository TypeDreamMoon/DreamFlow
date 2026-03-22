#pragma once

#include "CoreMinimal.h"
#include "Execution/DreamFlowExecutor.h"
#include "Components/ActorComponent.h"
#include "DreamFlowExecutorComponent.generated.h"

class FLifetimeProperty;
class UDreamFlowAsset;

UCLASS(ClassGroup = (Dream), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class DREAMFLOW_API UDreamFlowExecutorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDreamFlowExecutorComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

    /** Returns true while the component's executor is waiting for an async node to complete. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    bool IsWaitingForAsyncNode() const;

    /** Returns the node currently waiting for async completion. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    UDreamFlowNode* GetPendingAsyncNode() const;

    /** Completes the current async node using the default output or the supplied pin name. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Async")
    bool CompleteAsyncNode(FName OutputPinName = NAME_None);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UDreamFlowExecutor* GetExecutor() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UDreamFlowAsset* GetFlowAsset() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UObject* GetExecutionContext() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UDreamFlowNode* GetCurrentNode() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    TArray<UDreamFlowNode*> GetAvailableChildren() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    TArray<FDreamFlowNodeOutputPin> GetAvailableOutputPins() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    bool IsCurrentNodeAutomatic() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    bool IsWaitingForAdvance() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    TArray<UDreamFlowNode*> GetVisitedNodes() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    bool IsRunning() const;

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Debug")
    bool PauseExecution();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Debug")
    bool ContinueExecution();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Debug")
    bool StepExecution();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Debug")
    void SetPauseOnBreakpoints(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    bool IsPaused() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    bool GetPauseOnBreakpoints() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    EDreamFlowExecutorDebugState GetDebugState() const;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FlowAsset, Category = "DreamFlow")
    TObjectPtr<UDreamFlowAsset> FlowAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "DreamFlow")
    TSubclassOf<UDreamFlowExecutor> ExecutorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "DreamFlow")
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

    UFUNCTION()
    void OnRep_FlowAsset();

    UFUNCTION()
    void OnRep_ReplicatedExecutionState();

    UFUNCTION(Server, Reliable)
    void ServerStartFlow();

    UFUNCTION(Server, Reliable)
    void ServerRestartFlow();

    UFUNCTION(Server, Reliable)
    void ServerFinishFlow();

    UFUNCTION(Server, Reliable)
    void ServerAdvance();

    UFUNCTION(Server, Reliable)
    void ServerPauseExecution();

    UFUNCTION(Server, Reliable)
    void ServerContinueExecution();

    UFUNCTION(Server, Reliable)
    void ServerStepExecution();

    UFUNCTION(Server, Reliable)
    void ServerSetPauseOnBreakpoints(bool bEnabled);

    UFUNCTION(Server, Reliable)
    void ServerMoveToChildByIndex(int32 ChildIndex);

    UFUNCTION(Server, Reliable)
    void ServerMoveToOutputPin(FName OutputPinName);

    UFUNCTION(Server, Reliable)
    void ServerChooseChildByGuid(FGuid ChildNodeGuid);

    UFUNCTION(Server, Reliable)
    void ServerCompleteAsyncNode(FName OutputPinName);

    UFUNCTION(Server, Reliable)
    void ServerSetVariableValue(FName VariableName, FDreamFlowValue InValue);

    UFUNCTION(Server, Reliable)
    void ServerResetVariablesToDefaults();

    bool IsServerAuthority() const;
    UDreamFlowExecutor* GetOrCreateExecutor(bool bInitializeFromCurrentConfig);
    UDreamFlowExecutor* ResetExecutorToCurrentConfig();
    void BindExecutorDelegates(UDreamFlowExecutor* InExecutor);
    void UnbindExecutorDelegates(UDreamFlowExecutor* InExecutor);
    void SyncReplicatedStateFromExecutor();
    bool ApplyReplicatedStateToMirror();
    bool StartFlowLocal();
    bool RestartFlowLocal();
    void FinishFlowLocal();
    bool AdvanceLocal();
    bool PauseExecutionLocal();
    bool ContinueExecutionLocal();
    bool StepExecutionLocal();
    void SetPauseOnBreakpointsLocal(bool bEnabled);
    bool MoveToChildByIndexLocal(int32 ChildIndex);
    bool MoveToOutputPinLocal(FName OutputPinName);
    bool ChooseChildByGuidLocal(const FGuid& ChildNodeGuid);
    bool CompleteAsyncNodeLocal(FName OutputPinName);
    bool SetVariableValueLocal(FName VariableName, const FDreamFlowValue& InValue);
    void ResetVariablesToDefaultsLocal();
    void BroadcastReplicatedStateEvents(const FDreamFlowReplicatedExecutionState& PreviousState, const FDreamFlowReplicatedExecutionState& NewState);
    void HandleExecutorRuntimeStateChanged(UDreamFlowExecutor* InExecutor);

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowExecutor> Executor;

    UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedExecutionState)
    FDreamFlowReplicatedExecutionState ReplicatedExecutionState;

    UPROPERTY(Transient)
    FDreamFlowReplicatedExecutionState LastAppliedReplicatedState;

private:
    FDelegateHandle ExecutorRuntimeStateChangedHandle;
};
