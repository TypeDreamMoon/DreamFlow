#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DreamFlowVariableTypes.generated.h"

UENUM(BlueprintType)
enum class EDreamFlowValueType : uint8
{
    Bool,
    Int,
    Float,
    Name,
    String,
    Text,
    GameplayTag,
    Object,
};

UENUM(BlueprintType)
enum class EDreamFlowValueSourceType : uint8
{
    Literal,
    FlowVariable,
    ExecutionContextProperty,
};

UENUM(BlueprintType)
enum class EDreamFlowComparisonOperation : uint8
{
    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    EDreamFlowValueType Type = EDreamFlowValueType::Bool;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value", meta = (EditCondition = "Type == EDreamFlowValueType::Bool", EditConditionHides))
    bool BoolValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value", meta = (EditCondition = "Type == EDreamFlowValueType::Int", EditConditionHides))
    int32 IntValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value", meta = (EditCondition = "Type == EDreamFlowValueType::Float", EditConditionHides))
    float FloatValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value", meta = (EditCondition = "Type == EDreamFlowValueType::Name", EditConditionHides))
    FName NameValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value", meta = (EditCondition = "Type == EDreamFlowValueType::String", EditConditionHides))
    FString StringValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value", meta = (MultiLine = true, EditCondition = "Type == EDreamFlowValueType::Text", EditConditionHides))
    FText TextValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value", meta = (EditCondition = "Type == EDreamFlowValueType::GameplayTag", EditConditionHides))
    FGameplayTag GameplayTagValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value", meta = (EditCondition = "Type == EDreamFlowValueType::Object", EditConditionHides, AllowedClasses = "/Script/CoreUObject.Object"))
    TObjectPtr<UObject> ObjectValue = nullptr;

    FString DescribeType() const;
    FString Describe() const;
    FString DescribeCompact() const;
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowVariableDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
    FName Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
    FDreamFlowValue DefaultValue;
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowValueBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
    EDreamFlowValueSourceType SourceType = EDreamFlowValueSourceType::Literal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding", meta = (EditCondition = "SourceType == EDreamFlowValueSourceType::FlowVariable", EditConditionHides))
    FName VariableName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding", meta = (EditCondition = "SourceType == EDreamFlowValueSourceType::ExecutionContextProperty", EditConditionHides))
    FString PropertyPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding", meta = (EditCondition = "SourceType == EDreamFlowValueSourceType::Literal", EditConditionHides))
    FDreamFlowValue LiteralValue;

    FString Describe() const;
    FString DescribeCompact() const;
};

namespace DreamFlowVariable
{
    DREAMFLOW_API bool TryConvertValue(const FDreamFlowValue& InValue, EDreamFlowValueType TargetType, FDreamFlowValue& OutValue);
    DREAMFLOW_API bool TryCompareValues(const FDreamFlowValue& LeftValue, const FDreamFlowValue& RightValue, EDreamFlowComparisonOperation Operation, bool& OutResult);
}
