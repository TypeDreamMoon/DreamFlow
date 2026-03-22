#pragma once

#include "CoreMinimal.h"
#include "DreamFlowAsyncNode.h"
#include "DreamFlowNode.h"
#include "DreamFlowVariableTypes.h"
#include "DreamFlowCoreNodes.generated.h"

class UDreamFlowAsset;
class UDreamFlowAsyncContext;
class UDreamFlowExecutor;

UCLASS(Transient)
class DREAMFLOW_API UDreamFlowSubFlowAsyncProxy : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(
        UDreamFlowAsyncContext* InParentAsyncContext,
        UDreamFlowExecutor* InParentExecutor,
        UDreamFlowExecutor* InChildExecutor,
        bool bInCopyParentVariablesToChild,
        bool bInCopyChildVariablesToParent);

    void StartSubFlow();
    virtual void BeginDestroy() override;

private:
    void DriveChildFlow();
    void FinalizeSubFlow(FName OutputPinName, bool bSyncChildVariables);
    void HandleChildRuntimeStateChanged(UDreamFlowExecutor* InUpdatedExecutor);
    void SyncVariablesFromParentToChild() const;
    void SyncVariablesFromChildToParent() const;

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowAsyncContext> ParentAsyncContext;

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowExecutor> ParentExecutor;

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowExecutor> ChildExecutor;

    bool bCopyParentVariablesToChild = true;
    bool bCopyChildVariablesToParent = true;
    bool bHasCompleted = false;
    FDelegateHandle ChildRuntimeStateChangedHandle;
};

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowCoreNode : public UDreamFlowNode
{
    GENERATED_BODY()

public:
    virtual FText GetNodeCategory_Implementation() const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowBranchNode : public UDreamFlowCoreNode
{
    GENERATED_BODY()

public:
    UDreamFlowBranchNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch", meta = (DreamFlowExpectedValueType = "Bool", DreamFlowInlineEditable, DreamFlowInlinePriority = "20"))
    FDreamFlowValueBinding ConditionBinding;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual TArray<FDreamFlowNodeOutputPin> GetOutputPins_Implementation() const override;
    virtual bool SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual FName ResolveAutomaticTransitionOutputPin_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowCompareNode : public UDreamFlowCoreNode
{
    GENERATED_BODY()

public:
    UDreamFlowCompareNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compare", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "20"))
    FDreamFlowValueBinding LeftValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compare", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "30"))
    EDreamFlowComparisonOperation Operation = EDreamFlowComparisonOperation::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compare", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "40"))
    FDreamFlowValueBinding RightValue;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual TArray<FDreamFlowNodeOutputPin> GetOutputPins_Implementation() const override;
    virtual bool SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual FName ResolveAutomaticTransitionOutputPin_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowSetVariableNode : public UDreamFlowCoreNode
{
    GENERATED_BODY()

public:
    UDreamFlowSetVariableNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "20", DreamFlowVariablePicker))
    FName TargetVariable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "30"))
    FDreamFlowValueBinding ValueBinding;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual void ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor) override;
    virtual bool SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual FName ResolveAutomaticTransitionOutputPin_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowRunSubFlowNode : public UDreamFlowAsyncNode
{
    GENERATED_BODY()

public:
    UDreamFlowRunSubFlowNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sub Flow", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "20"))
    TObjectPtr<UDreamFlowAsset> SubFlowAsset = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sub Flow", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "30", AllowAbstract = "false"))
    TSubclassOf<UDreamFlowExecutor> SubFlowExecutorClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sub Flow", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "40"))
    bool bCopyParentVariablesToChild = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sub Flow", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "50"))
    bool bCopyChildVariablesToParent = true;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual TArray<FDreamFlowNodeOutputPin> GetOutputPins_Implementation() const override;
    virtual void StartAsyncNode_Implementation(UObject* Context, UDreamFlowExecutor* Executor, UDreamFlowAsyncContext* AsyncContext) override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowFinishFlowNode : public UDreamFlowCoreNode
{
    GENERATED_BODY()

public:
    UDreamFlowFinishFlowNode();

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual TArray<FDreamFlowNodeOutputPin> GetOutputPins_Implementation() const override;
    virtual bool SupportsMultipleChildren_Implementation() const override;
    virtual bool IsTerminalNode_Implementation() const override;
    virtual bool CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const override;
    virtual void ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor) override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowDelayNode : public UDreamFlowAsyncNode
{
    GENERATED_BODY()

public:
    UDreamFlowDelayNode();
    virtual void PostLoad() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Async", meta = (DreamFlowExpectedValueType = "Float", DreamFlowInlineEditable, DreamFlowInlinePriority = "20"))
    FDreamFlowValueBinding DurationBinding;

    UPROPERTY()
    float DurationSeconds = 1.0f;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual void StartAsyncNode_Implementation(UObject* Context, UDreamFlowExecutor* Executor, UDreamFlowAsyncContext* AsyncContext) override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};
