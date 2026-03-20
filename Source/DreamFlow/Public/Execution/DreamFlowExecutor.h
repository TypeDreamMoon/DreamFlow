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

UCLASS(BlueprintType, Blueprintable)
class DREAMFLOW_API UDreamFlowExecutor : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution")
    void Initialize(UDreamFlowAsset* InFlowAsset, UObject* InExecutionContext = nullptr);

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

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool HasVariable(FName VariableName) const;

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    bool GetVariableValue(FName VariableName, FDreamFlowValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    bool SetVariableValue(FName VariableName, const FDreamFlowValue& InValue);

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
    bool ExecuteCurrentNode();
    bool ShouldPauseAtNode(const UDreamFlowNode* Node);
    void BroadcastDebugStateChanged();
    void RegisterWithDebugger();
    void UnregisterFromDebugger();
    void NotifyDebuggerStateChanged();

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
};
