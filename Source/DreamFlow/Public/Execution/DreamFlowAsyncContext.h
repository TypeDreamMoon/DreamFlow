#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DreamFlowAsyncContext.generated.h"

class UDreamFlowAsset;
class UDreamFlowExecutor;
class UDreamFlowNode;

UCLASS(BlueprintType)
class DREAMFLOW_API UDreamFlowAsyncContext : public UObject
{
    GENERATED_BODY()

public:
    /** Returns true while this async context still points at the executor's currently pending async node. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    bool IsValidAsyncContext() const;

    /** Returns the executor that created this async context. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    UDreamFlowExecutor* GetExecutor() const;

    /** Returns the flow asset that owns the pending async node. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    UDreamFlowAsset* GetFlowAsset() const;

    /** Returns the async node this context was created for. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    UDreamFlowNode* GetNode() const;

    /** Returns the execution context object that is currently driving the flow. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Async")
    UObject* GetExecutionContext() const;

    /** Completes the pending async node and resumes through its default output. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Async")
    bool Complete();

    /** Completes the pending async node and resumes through the requested output pin. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Async")
    bool CompleteWithOutputPin(FName OutputPinName);

    /** Finishes the whole flow immediately. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Async")
    void FinishFlow();

    void Initialize(UDreamFlowExecutor* InExecutor, UDreamFlowNode* InNode);

protected:
    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowExecutor> Executor;

    UPROPERTY(Transient)
    TObjectPtr<UDreamFlowNode> Node;
};
