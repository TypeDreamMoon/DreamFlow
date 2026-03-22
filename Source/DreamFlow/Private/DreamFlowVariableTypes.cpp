#include "DreamFlowVariableTypes.h"

namespace
{
    static const TCHAR* GetValueTypeLabel(const EDreamFlowValueType Type)
    {
        switch (Type)
        {
        case EDreamFlowValueType::Bool:
            return TEXT("Bool");

        case EDreamFlowValueType::Int:
            return TEXT("Int");

        case EDreamFlowValueType::Float:
            return TEXT("Float");

        case EDreamFlowValueType::Name:
            return TEXT("Name");

        case EDreamFlowValueType::String:
            return TEXT("String");

        case EDreamFlowValueType::Text:
            return TEXT("Text");

        case EDreamFlowValueType::GameplayTag:
            return TEXT("GameplayTag");

        case EDreamFlowValueType::Object:
            return TEXT("Object");

        default:
            return TEXT("Unknown");
        }
    }

    static bool IsNumericType(const EDreamFlowValueType Type)
    {
        return Type == EDreamFlowValueType::Int || Type == EDreamFlowValueType::Float;
    }

    static double GetNumericValue(const FDreamFlowValue& Value)
    {
        switch (Value.Type)
        {
        case EDreamFlowValueType::Int:
            return static_cast<double>(Value.IntValue);

        case EDreamFlowValueType::Float:
            return static_cast<double>(Value.FloatValue);

        default:
            return 0.0;
        }
    }
}

FString FDreamFlowValue::DescribeType() const
{
    return GetValueTypeLabel(Type);
}

FString FDreamFlowValue::Describe() const
{
    switch (Type)
    {
    case EDreamFlowValueType::Bool:
        return BoolValue ? TEXT("True") : TEXT("False");

    case EDreamFlowValueType::Int:
        return FString::FromInt(IntValue);

    case EDreamFlowValueType::Float:
        return FString::SanitizeFloat(FloatValue);

    case EDreamFlowValueType::Name:
        return NameValue.ToString();

    case EDreamFlowValueType::String:
        return StringValue;

    case EDreamFlowValueType::Text:
        return TextValue.ToString();

    case EDreamFlowValueType::GameplayTag:
        return GameplayTagValue.ToString();

    case EDreamFlowValueType::Object:
        return GetNameSafe(ObjectValue);

    default:
        return TEXT("<unset>");
    }
}

FString FDreamFlowValue::DescribeCompact() const
{
    return FString::Printf(TEXT("Type: %s  Value: %s"), *DescribeType(), *Describe());
}

FString FDreamFlowValueBinding::Describe() const
{
    switch (SourceType)
    {
    case EDreamFlowValueSourceType::FlowVariable:
        return VariableName.IsNone() ? TEXT("<variable>") : VariableName.ToString();

    case EDreamFlowValueSourceType::ExecutionContextProperty:
        return PropertyPath.IsEmpty() ? TEXT("<property path>") : PropertyPath;

    case EDreamFlowValueSourceType::Literal:
    default:
        return LiteralValue.Describe();
    }
}

FString FDreamFlowValueBinding::DescribeCompact() const
{
    if (SourceType == EDreamFlowValueSourceType::FlowVariable)
    {
        return FString::Printf(
            TEXT("Type: Variable  Value: %s"),
            VariableName.IsNone() ? TEXT("<variable>") : *VariableName.ToString());
    }

    if (SourceType == EDreamFlowValueSourceType::ExecutionContextProperty)
    {
        return FString::Printf(
            TEXT("Type: Property  Value: %s"),
            PropertyPath.IsEmpty() ? TEXT("<property path>") : *PropertyPath);
    }

    return LiteralValue.DescribeCompact();
}

bool DreamFlowVariable::TryConvertValue(const FDreamFlowValue& InValue, EDreamFlowValueType TargetType, FDreamFlowValue& OutValue)
{
    OutValue = InValue;
    OutValue.Type = TargetType;

    if (InValue.Type == TargetType)
    {
        return true;
    }

    if (IsNumericType(InValue.Type) && IsNumericType(TargetType))
    {
        if (TargetType == EDreamFlowValueType::Int)
        {
            OutValue.IntValue = FMath::RoundToInt(static_cast<float>(GetNumericValue(InValue)));
        }
        else
        {
            OutValue.FloatValue = static_cast<float>(GetNumericValue(InValue));
        }
        return true;
    }

    switch (TargetType)
    {
    case EDreamFlowValueType::Bool:
        if (InValue.Type == EDreamFlowValueType::Int)
        {
            OutValue.BoolValue = InValue.IntValue != 0;
            return true;
        }
        if (InValue.Type == EDreamFlowValueType::Float)
        {
            OutValue.BoolValue = !FMath::IsNearlyZero(InValue.FloatValue);
            return true;
        }
        break;

    case EDreamFlowValueType::Name:
        if (InValue.Type == EDreamFlowValueType::String)
        {
            OutValue.NameValue = FName(*InValue.StringValue);
            return true;
        }
        if (InValue.Type == EDreamFlowValueType::Text)
        {
            OutValue.NameValue = FName(*InValue.TextValue.ToString());
            return true;
        }
        break;

    case EDreamFlowValueType::String:
        if (InValue.Type == EDreamFlowValueType::Name)
        {
            OutValue.StringValue = InValue.NameValue.ToString();
            return true;
        }
        if (InValue.Type == EDreamFlowValueType::Text)
        {
            OutValue.StringValue = InValue.TextValue.ToString();
            return true;
        }
        break;

    case EDreamFlowValueType::Text:
        if (InValue.Type == EDreamFlowValueType::String)
        {
            OutValue.TextValue = FText::FromString(InValue.StringValue);
            return true;
        }
        if (InValue.Type == EDreamFlowValueType::Name)
        {
            OutValue.TextValue = FText::FromName(InValue.NameValue);
            return true;
        }
        break;

    case EDreamFlowValueType::Object:
        break;

    default:
        break;
    }

    return false;
}

bool DreamFlowVariable::TryCompareValues(const FDreamFlowValue& LeftValue, const FDreamFlowValue& RightValue, EDreamFlowComparisonOperation Operation, bool& OutResult)
{
    OutResult = false;

    if (IsNumericType(LeftValue.Type) && IsNumericType(RightValue.Type))
    {
        const double LeftNumeric = GetNumericValue(LeftValue);
        const double RightNumeric = GetNumericValue(RightValue);

        switch (Operation)
        {
        case EDreamFlowComparisonOperation::Equal:
            OutResult = FMath::IsNearlyEqual(LeftNumeric, RightNumeric);
            return true;

        case EDreamFlowComparisonOperation::NotEqual:
            OutResult = !FMath::IsNearlyEqual(LeftNumeric, RightNumeric);
            return true;

        case EDreamFlowComparisonOperation::Less:
            OutResult = LeftNumeric < RightNumeric;
            return true;

        case EDreamFlowComparisonOperation::LessOrEqual:
            OutResult = LeftNumeric <= RightNumeric;
            return true;

        case EDreamFlowComparisonOperation::Greater:
            OutResult = LeftNumeric > RightNumeric;
            return true;

        case EDreamFlowComparisonOperation::GreaterOrEqual:
            OutResult = LeftNumeric >= RightNumeric;
            return true;
        }
    }

    FDreamFlowValue ConvertedRightValue;
    if (!TryConvertValue(RightValue, LeftValue.Type, ConvertedRightValue))
    {
        return false;
    }

    switch (Operation)
    {
    case EDreamFlowComparisonOperation::Equal:
    case EDreamFlowComparisonOperation::NotEqual:
        switch (LeftValue.Type)
        {
        case EDreamFlowValueType::Bool:
            OutResult = LeftValue.BoolValue == ConvertedRightValue.BoolValue;
            break;

        case EDreamFlowValueType::Name:
            OutResult = LeftValue.NameValue == ConvertedRightValue.NameValue;
            break;

        case EDreamFlowValueType::String:
            OutResult = LeftValue.StringValue == ConvertedRightValue.StringValue;
            break;

        case EDreamFlowValueType::Text:
            OutResult = LeftValue.TextValue.EqualTo(ConvertedRightValue.TextValue);
            break;

        case EDreamFlowValueType::GameplayTag:
            OutResult = LeftValue.GameplayTagValue == ConvertedRightValue.GameplayTagValue;
            break;

        case EDreamFlowValueType::Object:
            OutResult = LeftValue.ObjectValue == ConvertedRightValue.ObjectValue;
            break;

        default:
            return false;
        }

        if (Operation == EDreamFlowComparisonOperation::NotEqual)
        {
            OutResult = !OutResult;
        }
        return true;

    default:
        return false;
    }
}
