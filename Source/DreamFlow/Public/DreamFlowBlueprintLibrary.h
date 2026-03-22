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
    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", DeterminesOutputType = "ExecutorClass"))
    static UDreamFlowExecutor* CreateFlowExecutor(UObject* WorldContextObject, UDreamFlowAsset* FlowAsset, UObject* ExecutionContext, TSubclassOf<UDreamFlowExecutor> ExecutorClass);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static UDreamFlowNode* GetEntryNode(const UDreamFlowAsset* FlowAsset);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static TArray<UDreamFlowNode*> GetNodes(const UDreamFlowAsset* FlowAsset);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static TArray<UDreamFlowNode*> GetChildren(const UDreamFlowNode* FlowNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static TArray<FDreamFlowNodeOutputPin> GetNodeOutputPins(const UDreamFlowNode* FlowNode);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static TArray<FDreamFlowNodeOutputLink> GetNodeOutputLinks(const UDreamFlowNode* FlowNode);

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

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool HasFlowVariableDefinition(const UDreamFlowAsset* FlowAsset, FName VariableName);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables")
    static bool FindFlowVariableDefinition(const UDreamFlowAsset* FlowAsset, FName VariableName, FDreamFlowVariableDefinition& OutDefinition);

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

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding", meta = (AutoCreateRefTerm = "LiteralValue"))
    static FDreamFlowValueBinding MakeLiteralFlowBinding(const FDreamFlowValue& LiteralValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static FDreamFlowValueBinding MakeVariableFlowBinding(FName VariableName);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static FDreamFlowValueBinding MakeExecutionContextPropertyFlowBinding(const FString& PropertyPath);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static EDreamFlowValueSourceType GetFlowBindingSourceType(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool IsLiteralFlowBinding(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool IsVariableFlowBinding(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool IsExecutionContextPropertyFlowBinding(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static FName GetFlowBindingVariableName(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static FString GetFlowBindingPropertyPath(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static FDreamFlowValue GetFlowBindingLiteralValue(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingVariableName(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FName& OutVariableName);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool CanExecutorWriteBinding(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingBoolValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, bool& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingIntValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, int32& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingFloatValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, float& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingNameValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FName& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingStringValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FString& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingTextValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FText& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingGameplayTagValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FGameplayTag& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding")
    static bool GetExecutorBindingObjectValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, UObject*& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding|Low Level")
    static bool GetExecutorBindingValue(const UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FDreamFlowValue& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding", meta = (AutoCreateRefTerm = "Binding"))
    static FDreamFlowValueBinding SetFlowBindingSourceType(const FDreamFlowValueBinding& Binding, EDreamFlowValueSourceType SourceType);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding", meta = (AutoCreateRefTerm = "Binding"))
    static FDreamFlowValueBinding SetFlowBindingVariableName(const FDreamFlowValueBinding& Binding, FName VariableName);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding", meta = (AutoCreateRefTerm = "Binding"))
    static FDreamFlowValueBinding SetFlowBindingPropertyPath(const FDreamFlowValueBinding& Binding, const FString& PropertyPath);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding", meta = (AutoCreateRefTerm = "Binding,LiteralValue"))
    static FDreamFlowValueBinding SetFlowBindingLiteralValue(const FDreamFlowValueBinding& Binding, const FDreamFlowValue& LiteralValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding", meta = (AutoCreateRefTerm = "Binding,LiteralValue"))
    static FDreamFlowValueBinding SetFlowBindingAsLiteral(const FDreamFlowValueBinding& Binding, const FDreamFlowValue& LiteralValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding", meta = (AutoCreateRefTerm = "Binding"))
    static FDreamFlowValueBinding SetFlowBindingAsVariable(const FDreamFlowValueBinding& Binding, FName VariableName);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Binding", meta = (AutoCreateRefTerm = "Binding"))
    static FDreamFlowValueBinding SetFlowBindingAsExecutionContextProperty(const FDreamFlowValueBinding& Binding, const FString& PropertyPath);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    static bool SetExecutorBindingBoolValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, bool InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    static bool SetExecutorBindingIntValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, int32 InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    static bool SetExecutorBindingFloatValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, float InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    static bool SetExecutorBindingNameValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FName InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    static bool SetExecutorBindingStringValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, const FString& InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    static bool SetExecutorBindingTextValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, const FText& InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    static bool SetExecutorBindingGameplayTagValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, FGameplayTag InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding")
    static bool SetExecutorBindingObjectValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, UObject* InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Binding|Low Level")
    static bool SetExecutorBindingValue(UDreamFlowExecutor* Executor, const FDreamFlowValueBinding& Binding, const FDreamFlowValue& InValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Property")
    static bool GetExecutorExecutionContextProperty(const UDreamFlowExecutor* Executor, const FString& PropertyPath, FDreamFlowValue& OutValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Variables|Property")
    static bool SetExecutorExecutionContextProperty(UDreamFlowExecutor* Executor, const FString& PropertyPath, const FDreamFlowValue& InValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Snapshot")
    static FDreamFlowExecutionSnapshot CaptureExecutorSnapshot(const UDreamFlowExecutor* Executor);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Execution|Snapshot")
    static void ApplyExecutorSnapshot(UDreamFlowExecutor* Executor, const FDreamFlowExecutionSnapshot& Snapshot);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Node State")
    static bool GetExecutorNodeState(const UDreamFlowExecutor* Executor, FGuid NodeGuid, FName StateKey, FDreamFlowValue& OutValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    static bool SetExecutorNodeState(UDreamFlowExecutor* Executor, FGuid NodeGuid, FName StateKey, const FDreamFlowValue& InValue);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    static void ResetExecutorNodeState(UDreamFlowExecutor* Executor, FGuid NodeGuid);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Node State")
    static void ResetAllExecutorNodeStates(UDreamFlowExecutor* Executor);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    static UDreamFlowExecutor* GetExecutorParent(const UDreamFlowExecutor* Executor);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    static UDreamFlowExecutor* GetExecutorCurrentChild(const UDreamFlowExecutor* Executor);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    static TArray<UDreamFlowExecutor*> GetExecutorActiveChildren(const UDreamFlowExecutor* Executor);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Execution|Sub Flow")
    static TArray<FDreamFlowSubFlowStackEntry> GetExecutorSubFlowStack(const UDreamFlowExecutor* Executor);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FString DescribeFlowValue(const FDreamFlowValue& Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FString DescribeCompactFlowValue(const FDreamFlowValue& Value);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FString DescribeFlowBinding(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static FString DescribeCompactFlowBinding(const FDreamFlowValueBinding& Binding);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static bool ConvertFlowValue(const FDreamFlowValue& InValue, EDreamFlowValueType TargetType, FDreamFlowValue& OutValue);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Variables|Low Level")
    static bool CompareFlowValues(const FDreamFlowValue& LeftValue, const FDreamFlowValue& RightValue, EDreamFlowComparisonOperation Operation, bool& OutResult);

    UFUNCTION(BlueprintPure, Category = "DreamFlow")
    static bool NodeSupportsFlowAsset(const UDreamFlowNode* FlowNode, const UDreamFlowAsset* FlowAsset);

    UFUNCTION(BlueprintCallable, Category = "DreamFlow|Validation")
    static void ValidateFlow(const UDreamFlowAsset* FlowAsset, TArray<FDreamFlowValidationMessage>& OutMessages);

    UFUNCTION(BlueprintPure, Category = "DreamFlow|Validation")
    static bool HasValidationErrors(const TArray<FDreamFlowValidationMessage>& ValidationMessages);
};
