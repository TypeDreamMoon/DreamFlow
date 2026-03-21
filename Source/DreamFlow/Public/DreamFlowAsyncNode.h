#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNode.h"
#include "DreamFlowAsyncNode.generated.h"

class UDreamFlowAsyncContext;
class UDreamFlowExecutor;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOW_API UDreamFlowAsyncNode : public UDreamFlowNode
{
    GENERATED_BODY()

public:
    UDreamFlowAsyncNode();

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override;
    virtual TArray<FDreamFlowNodeOutputPin> GetOutputPins_Implementation() const override;
    virtual void ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor) override;

    /** Starts this node's async work. Call AsyncContext->Complete() or CompleteWithOutputPin() when finished. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Flow|Async")
    void StartAsyncNode(UObject* Context, UDreamFlowExecutor* Executor, UDreamFlowAsyncContext* AsyncContext);
    virtual void StartAsyncNode_Implementation(UObject* Context, UDreamFlowExecutor* Executor, UDreamFlowAsyncContext* AsyncContext);
};
