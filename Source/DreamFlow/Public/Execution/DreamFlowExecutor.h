#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "DreamFlowVariableTypes.h"
#include "UObject/Object.h"
#include "DreamFlowExecutor.generated.h"

class UDreamFlowAsset;
class UDreamFlowAsyncContext;
class UDreamFlowNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDreamFlowExecutorEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDreamFlowExecutorNodeEventSignature, UDreamFlowNode*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDreamFlowExecutorStateChangedSignature, EDreamFlowExecutorDebugState, DebugState, UDreamFlowNode*, FocusNode);
DECLARE_MULTICAST_DELEGATE_OneParam(FDreamFlowExecutorRuntimeStateChangedNativeSignature, UDreamFlowExecutor*);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDreamFlowExecutorVariableChangedNativeSignature, UDreamFlowExecutor*, FName, const FDreamFlowValue&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDreamFlowExecutorExecutionContextPropertyChangedNativeSignature, UDreamFlowExecutor*, const FString&, const FDreamFlowValue&);

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
    UDreamFlowNode* GetEntryNode() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    TArray<UDreamFlowNode*> GetNodes() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    UDreamFlowNode* FindNodeByGuid(FGuid NodeGuid) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution")
    bool OwnsNode(const UDreamFlowNode* Node) const;

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

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    bool IsPaused() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    bool GetPauseOnBreakpoints() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Debug")
    EDreamFlowExecutorDebugState GetDebugState() const;

    /** Returns true while the executor is waiting for an async node to complete. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    bool IsWaitingForAsyncNode() const;

    /** Returns the async node currently waiting for completion, if any. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    UDreamFlowNode* GetPendingAsyncNode() const;

    /**
     * Marks the current node as asynchronous and returns a completion handle.
     * Async nodes usually call this from ExecuteNodeWithExecutor before starting latent work.
     */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Async")
    UDreamFlowAsyncContext* BeginAsyncNode(UDreamFlowNode* Node);

    /** Completes the pending async node using the default output or the supplied pin name. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Async")
    bool CompleteAsyncNode(FName OutputPinName = NAME_None);

    /** Completes the pending async node only if it matches the supplied node instance. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Async")
    bool CompleteAsyncNodeForNode(UDreamFlowNode* Node, FName OutputPinName = NAME_None);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Snapshot")
    FDreamFlowExecutionSnapshot BuildExecutionSnapshot() const;

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Snapshot")
    void ApplyExecutionSnapshot(const FDreamFlowExecutionSnapshot& InSnapshot);

    void RegisterChildExecutor(UDreamFlowNode* OwnerNode, UDreamFlowExecutor* ChildExecutor);
    void UnregisterChildExecutor(UDreamFlowNode* OwnerNode, UDreamFlowExecutor* ChildExecutor);
    UDreamFlowExecutor* StartParallelBranchAtNode(UDreamFlowNode* StartNode);
    UDreamFlowExecutor* StartParallelBranchAtNodeWithOwnerKey(UDreamFlowNode* StartNode, FGuid OwnerKey);
    UObject* GetPersistentRuntimeObject(FGuid OwnerNodeGuid) const;
    void SetPersistentRuntimeObject(FGuid OwnerNodeGuid, UObject* RuntimeObject);
    void ClearPersistentRuntimeObject(FGuid OwnerNodeGuid);

    void BuildReplicatedState(FDreamFlowReplicatedExecutionState& OutState) const;
    void ApplyReplicatedState(const FDreamFlowReplicatedExecutionState& InState);
    FDreamFlowExecutorRuntimeStateChangedNativeSignature& OnRuntimeStateChangedNative();
    FDreamFlowExecutorVariableChangedNativeSignature& OnVariableChangedNative();
    FDreamFlowExecutorExecutionContextPropertyChangedNativeSignature& OnExecutionContextPropertyChangedNative();

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

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool HasNodeStateValue(FGuid NodeGuid, FName StateKey) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateValue(FGuid NodeGuid, FName StateKey, FDreamFlowValue& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateBoolValue(FGuid NodeGuid, FName StateKey, bool& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateIntValue(FGuid NodeGuid, FName StateKey, int32& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateFloatValue(FGuid NodeGuid, FName StateKey, float& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateNameValue(FGuid NodeGuid, FName StateKey, FName& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateStringValue(FGuid NodeGuid, FName StateKey, FString& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateTextValue(FGuid NodeGuid, FName StateKey, FText& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateGameplayTagValue(FGuid NodeGuid, FName StateKey, FGameplayTag& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    bool GetNodeStateObjectValue(FGuid NodeGuid, FName StateKey, UObject*& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateValue(FGuid NodeGuid, FName StateKey, const FDreamFlowValue& InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateBoolValue(FGuid NodeGuid, FName StateKey, bool InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateIntValue(FGuid NodeGuid, FName StateKey, int32 InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateFloatValue(FGuid NodeGuid, FName StateKey, float InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateNameValue(FGuid NodeGuid, FName StateKey, FName InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateStringValue(FGuid NodeGuid, FName StateKey, const FString& InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateTextValue(FGuid NodeGuid, FName StateKey, const FText& InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateGameplayTagValue(FGuid NodeGuid, FName StateKey, FGameplayTag InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    bool SetNodeStateObjectValue(FGuid NodeGuid, FName StateKey, UObject* InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    void ResetNodeState(FGuid NodeGuid);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    void ResetAllNodeStates();

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    UDreamFlowExecutor* GetParentExecutor() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    UDreamFlowExecutor* GetChildExecutorForNode(const UDreamFlowNode* Node) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    UDreamFlowExecutor* GetCurrentChildExecutor() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    TArray<UDreamFlowExecutor*> GetActiveChildExecutors() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    TArray<FDreamFlowSubFlowStackEntry> GetActiveSubFlowStack() const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Property")
    bool GetExecutionContextPropertyValue(const FString& PropertyPath, FDreamFlowValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Property")
    bool SetExecutionContextPropertyValue(const FString& PropertyPath, const FDreamFlowValue& InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Property")
    void NotifyExecutionContextChanged();

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Property")
    bool NotifyExecutionContextPropertyChanged(const FString& PropertyPath);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool ResolveBindingValue(const FDreamFlowValueBinding& Binding, FDreamFlowValue& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool ResolveBindingAsBool(const FDreamFlowValueBinding& Binding, bool& OutValue) const;

    /** Returns the bound flow-variable name when this binding targets a writable runtime variable. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingVariableName(const FDreamFlowValueBinding& Binding, FName& OutVariableName) const;

    /** Returns true when this binding points at a writable flow variable on the current executor. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool CanWriteBindingValue(const FDreamFlowValueBinding& Binding) const;

    /** Read a binding as a bool. Returns false when the binding cannot be resolved or converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingBoolValue(const FDreamFlowValueBinding& Binding, bool& OutValue) const;

    /** Read a binding as an int. Returns false when the binding cannot be resolved or converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingIntValue(const FDreamFlowValueBinding& Binding, int32& OutValue) const;

    /** Read a binding as a float. Returns false when the binding cannot be resolved or converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingFloatValue(const FDreamFlowValueBinding& Binding, float& OutValue) const;

    /** Read a binding as a name. Returns false when the binding cannot be resolved or converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingNameValue(const FDreamFlowValueBinding& Binding, FName& OutValue) const;

    /** Read a binding as a string. Returns false when the binding cannot be resolved or converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingStringValue(const FDreamFlowValueBinding& Binding, FString& OutValue) const;

    /** Read a binding as text. Returns false when the binding cannot be resolved or converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingTextValue(const FDreamFlowValueBinding& Binding, FText& OutValue) const;

    /** Read a binding as a gameplay tag. Returns false when the binding cannot be resolved or converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingGameplayTagValue(const FDreamFlowValueBinding& Binding, FGameplayTag& OutValue) const;

    /** Read a binding as an object reference. Returns false when the binding cannot be resolved or converted. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    bool GetBindingObjectValue(const FDreamFlowValueBinding& Binding, UObject*& OutValue) const;

    /** Write a bool through a variable-backed binding. Returns false for literal bindings. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    bool SetBindingBoolValue(const FDreamFlowValueBinding& Binding, bool InValue);

    /** Write an int through a variable-backed binding. Returns false for literal bindings. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    bool SetBindingIntValue(const FDreamFlowValueBinding& Binding, int32 InValue);

    /** Write a float through a variable-backed binding. Returns false for literal bindings. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    bool SetBindingFloatValue(const FDreamFlowValueBinding& Binding, float InValue);

    /** Write a name through a variable-backed binding. Returns false for literal bindings. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    bool SetBindingNameValue(const FDreamFlowValueBinding& Binding, FName InValue);

    /** Write a string through a variable-backed binding. Returns false for literal bindings. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    bool SetBindingStringValue(const FDreamFlowValueBinding& Binding, const FString& InValue);

    /** Write a text through a variable-backed binding. Returns false for literal bindings. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    bool SetBindingTextValue(const FDreamFlowValueBinding& Binding, const FText& InValue);

    /** Write a gameplay tag through a variable-backed binding. Returns false for literal bindings. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    bool SetBindingGameplayTagValue(const FDreamFlowValueBinding& Binding, FGameplayTag InValue);

    /** Write an object reference through a variable-backed binding. Returns false for literal bindings. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    bool SetBindingObjectValue(const FDreamFlowValueBinding& Binding, UObject* InValue);

    /** Low-level raw struct binding write. Returns false when the binding does not target a flow variable. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding|Low Level")
    bool SetBindingValue(const FDreamFlowValueBinding& Binding, const FDreamFlowValue& InValue);

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
    bool TryConsumeCompletedAsyncNode(FName RequestedOutputPinName);
    FName ResolveCompletedAsyncOutputPin(const UDreamFlowNode* AsyncNode, FName RequestedOutputPinName) const;
    bool ShouldPauseAtNode(const UDreamFlowNode* Node, bool& bOutHitBreakpoint);
    void BroadcastDebugStateChanged();
    void RegisterWithDebugger();
    void UnregisterFromDebugger();
    void NotifyDebuggerStateChanged();
    void DetachFromParentExecutor();
    void ResetRuntimeState(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext, bool bNotifyDebugger);
    void ClearAsyncExecutionState();

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
    TArray<FDreamFlowNodeRuntimeState> NodeRuntimeStates;

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowNode> PendingAsyncNode;

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowAsyncContext> ActiveAsyncContext;

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

    UPROPERTY(Transient)
    bool bIsWaitingForAsyncNode = false;

    UPROPERTY(Transient)
    bool bHasQueuedAsyncCompletion = false;

    UPROPERTY(Transient)
    FName QueuedAsyncCompletionOutputPin;

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowExecutor> ParentExecutor;

    UPROPERTY(Transient)
    TMap<FGuid, TObjectPtr<UDreamFlowExecutor>> ActiveChildExecutorsByNodeGuid;

    UPROPERTY(Transient)
    FGuid ParentLinkNodeGuid;

    UPROPERTY(Transient)
    bool bSharesRuntimeVariablesWithParent = false;

    UPROPERTY(Transient)
    TArray<FDreamFlowSubFlowStackEntry> CachedSubFlowStack;

    UPROPERTY(Transient)
    TMap<FGuid, TObjectPtr<UObject>> PersistentRuntimeObjectsByNodeGuid;

private:
    FDreamFlowExecutorRuntimeStateChangedNativeSignature RuntimeStateChangedNative;
    FDreamFlowExecutorVariableChangedNativeSignature VariableChangedNative;
    FDreamFlowExecutorExecutionContextPropertyChangedNativeSignature ExecutionContextPropertyChangedNative;
};
