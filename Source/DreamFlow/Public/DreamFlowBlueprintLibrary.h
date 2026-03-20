#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "DreamFlowVariableTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DreamFlowBlueprintLibrary.generated.h"

class UDreamFlowAsset;
class UDreamFlowExecutor;
class UDreamFlowNode;

UCLASS()
class DREAMFLOW_API UDreamFlowBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static UDreamFlowNode* GetEntryNode(const UDreamFlowAsset* FlowAsset);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static TArray<UDreamFlowNode*> GetNodes(const UDreamFlowAsset* FlowAsset);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static TArray<UDreamFlowNode*> GetChildren(const UDreamFlowNode* FlowNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static FText GetNodeTitle(const UDreamFlowNode* FlowNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static FText GetNodeCategory(const UDreamFlowNode* FlowNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static UDreamFlowNode* FindNodeByGuid(const UDreamFlowAsset* FlowAsset, FGuid NodeGuid);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static TArray<FDreamFlowVariableDefinition> GetFlowVariables(const UDreamFlowAsset* FlowAsset);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorVariable(const UDreamFlowExecutor* Executor, FName VariableName, FDreamFlowValue& OutValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Validation")
    static void ValidateFlow(const UDreamFlowAsset* FlowAsset, TArray<FDreamFlowValidationMessage>& OutMessages);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Validation")
    static bool HasValidationErrors(const TArray<FDreamFlowValidationMessage>& ValidationMessages);
};
