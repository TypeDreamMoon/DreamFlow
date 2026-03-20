#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNode.h"
#include "DreamFlowVariableTypes.h"
#include "DreamFlowCoreNodes.generated.h"

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch")
    FDreamFlowValueBinding ConditionBinding;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual bool SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual int32 ResolveAutomaticTransitionChildIndex_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowCompareNode : public UDreamFlowCoreNode
{
    GENERATED_BODY()

public:
    UDreamFlowCompareNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compare")
    FDreamFlowValueBinding LeftValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compare")
    EDreamFlowComparisonOperation Operation = EDreamFlowComparisonOperation::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compare")
    FDreamFlowValueBinding RightValue;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual bool SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual int32 ResolveAutomaticTransitionChildIndex_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowSetVariableNode : public UDreamFlowCoreNode
{
    GENERATED_BODY()

public:
    UDreamFlowSetVariableNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
    FName TargetVariable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
    FDreamFlowValueBinding ValueBinding;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual void ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor) override;
    virtual bool SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual int32 ResolveAutomaticTransitionChildIndex_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const override;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const override;
};
