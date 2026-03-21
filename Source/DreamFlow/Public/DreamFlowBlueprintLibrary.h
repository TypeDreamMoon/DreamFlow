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
    static TArray<FDreamFlowNodeOutputPin> GetNodeOutputPins(const UDreamFlowNode* FlowNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static UDreamFlowNode* GetFirstChildForOutputPin(const UDreamFlowNode* FlowNode, FName OutputPinName);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static FText GetNodeTitle(const UDreamFlowNode* FlowNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static FText GetNodeCategory(const UDreamFlowNode* FlowNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static UDreamFlowNode* FindNodeByGuid(const UDreamFlowAsset* FlowAsset, FGuid NodeGuid);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static TArray<FDreamFlowVariableDefinition> GetFlowVariables(const UDreamFlowAsset* FlowAsset);

    /** Read an executor variable as a bool. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorBoolVariable(const UDreamFlowExecutor* Executor, FName VariableName, bool& OutValue);

    /** Read an executor variable as an int. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorIntVariable(const UDreamFlowExecutor* Executor, FName VariableName, int32& OutValue);

    /** Read an executor variable as a float. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorFloatVariable(const UDreamFlowExecutor* Executor, FName VariableName, float& OutValue);

    /** Read an executor variable as a name. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorNameVariable(const UDreamFlowExecutor* Executor, FName VariableName, FName& OutValue);

    /** Read an executor variable as a string. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorStringVariable(const UDreamFlowExecutor* Executor, FName VariableName, FString& OutValue);

    /** Read an executor variable as text. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorTextVariable(const UDreamFlowExecutor* Executor, FName VariableName, FText& OutValue);

    /** Read an executor variable as a gameplay tag. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorGameplayTagVariable(const UDreamFlowExecutor* Executor, FName VariableName, FGameplayTag& OutValue);

    /** Read an executor variable as an object reference. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool GetExecutorObjectVariable(const UDreamFlowExecutor* Executor, FName VariableName, UObject*& OutValue);

    /** Low-level raw struct access for custom variable workflows. */
    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static bool GetExecutorVariable(const UDreamFlowExecutor* Executor, FName VariableName, FDreamFlowValue& OutValue);

    /** Write an executor variable as a bool. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    static bool SetExecutorBoolVariable(UDreamFlowExecutor* Executor, FName VariableName, bool InValue);

    /** Write an executor variable as an int. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    static bool SetExecutorIntVariable(UDreamFlowExecutor* Executor, FName VariableName, int32 InValue);

    /** Write an executor variable as a float. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    static bool SetExecutorFloatVariable(UDreamFlowExecutor* Executor, FName VariableName, float InValue);

    /** Write an executor variable as a name. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    static bool SetExecutorNameVariable(UDreamFlowExecutor* Executor, FName VariableName, FName InValue);

    /** Write an executor variable as a string. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    static bool SetExecutorStringVariable(UDreamFlowExecutor* Executor, FName VariableName, const FString& InValue);

    /** Write an executor variable as text. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    static bool SetExecutorTextVariable(UDreamFlowExecutor* Executor, FName VariableName, const FText& InValue);

    /** Write an executor variable as a gameplay tag. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    static bool SetExecutorGameplayTagVariable(UDreamFlowExecutor* Executor, FName VariableName, FGameplayTag InValue);

    /** Write an executor variable as an object reference. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables")
    static bool SetExecutorObjectVariable(UDreamFlowExecutor* Executor, FName VariableName, UObject* InValue);

    /** Low-level raw struct write for custom variable workflows. */
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Low Level")
    static bool SetExecutorVariable(UDreamFlowExecutor* Executor, FName VariableName, const FDreamFlowValue& InValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FDreamFlowValue MakeBoolFlowValue(bool Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FDreamFlowValue MakeIntFlowValue(int32 Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FDreamFlowValue MakeFloatFlowValue(float Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FDreamFlowValue MakeNameFlowValue(FName Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FDreamFlowValue MakeStringFlowValue(const FString& Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FDreamFlowValue MakeTextFlowValue(const FText& Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FDreamFlowValue MakeGameplayTagFlowValue(FGameplayTag Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FDreamFlowValue MakeObjectFlowValue(UObject* Value);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Validation")
    static void ValidateFlow(const UDreamFlowAsset* FlowAsset, TArray<FDreamFlowValidationMessage>& OutMessages);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Validation")
    static bool HasValidationErrors(const TArray<FDreamFlowValidationMessage>& ValidationMessages);
};
