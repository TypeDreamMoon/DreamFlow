#include "DreamFlowCoreNodes.h"

#include "DreamFlowAsset.h"
#include "Execution/DreamFlowExecutor.h"

namespace
{
    static FDreamFlowNodeDisplayItem MakeTextPreviewItem(const FText& Label, const FString& Value)
    {
        FDreamFlowNodeDisplayItem Item;
        Item.Type = EDreamFlowNodeDisplayItemType::Text;
        Item.Label = Label;
        Item.TextValue = FText::FromString(Value);
        return Item;
    }

    static FText MakeComparisonLabel(const EDreamFlowComparisonOperation Operation)
    {
        switch (Operation)
        {
        case EDreamFlowComparisonOperation::Equal:
            return FText::FromString(TEXT("=="));

        case EDreamFlowComparisonOperation::NotEqual:
            return FText::FromString(TEXT("!="));

        case EDreamFlowComparisonOperation::Less:
            return FText::FromString(TEXT("<"));

        case EDreamFlowComparisonOperation::LessOrEqual:
            return FText::FromString(TEXT("<="));

        case EDreamFlowComparisonOperation::Greater:
            return FText::FromString(TEXT(">"));

        case EDreamFlowComparisonOperation::GreaterOrEqual:
            return FText::FromString(TEXT(">="));

        default:
            return FText::FromString(TEXT("?"));
        }
    }

    static void AddValidationMessage(
        TArray<FDreamFlowValidationMessage>& OutMessages,
        const UDreamFlowNode* Node,
        const EDreamFlowValidationSeverity Severity,
        const FText& Message)
    {
        FDreamFlowValidationMessage& ValidationMessage = OutMessages.AddDefaulted_GetRef();
        ValidationMessage.Severity = Severity;
        ValidationMessage.NodeGuid = Node != nullptr ? Node->NodeGuid : FGuid();
        ValidationMessage.NodeTitle = Node != nullptr ? Node->GetNodeDisplayName() : FText::GetEmpty();
        ValidationMessage.Message = Message;
    }

    static void ValidateBinding(
        const UDreamFlowAsset* OwningAsset,
        const UDreamFlowNode* Node,
        const FDreamFlowValueBinding& Binding,
        const FText& BindingLabel,
        TArray<FDreamFlowValidationMessage>& OutMessages)
    {
        if (Binding.SourceType == EDreamFlowValueSourceType::FlowVariable)
        {
            if (Binding.VariableName.IsNone())
            {
                AddValidationMessage(
                    OutMessages,
                    Node,
                    EDreamFlowValidationSeverity::Error,
                    FText::Format(FText::FromString(TEXT("{0} is set to use a flow variable, but no variable name is assigned.")), BindingLabel));
            }
            else if (OwningAsset != nullptr && !OwningAsset->HasVariableDefinition(Binding.VariableName))
            {
                AddValidationMessage(
                    OutMessages,
                    Node,
                    EDreamFlowValidationSeverity::Error,
                    FText::Format(
                        FText::FromString(TEXT("{0} references the missing flow variable '{1}'.")),
                        BindingLabel,
                        FText::FromName(Binding.VariableName)));
            }
        }
    }
}

FText UDreamFlowCoreNode::GetNodeCategory_Implementation() const
{
    return FText::FromString(TEXT("Core"));
}

UDreamFlowBranchNode::UDreamFlowBranchNode()
{
    Title = FText::FromString(TEXT("Branch"));
    Description = FText::FromString(TEXT("Chooses the first child when the condition is true, otherwise the second child."));
    ConditionBinding.LiteralValue.Type = EDreamFlowValueType::Bool;

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.80f, 0.52f, 0.15f, 1.0f);
#endif
}

FText UDreamFlowBranchNode::GetNodeDisplayName_Implementation() const
{
    return Title;
}

FLinearColor UDreamFlowBranchNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.80f, 0.52f, 0.15f, 1.0f);
}

FText UDreamFlowBranchNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("If"));
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowBranchNode::GetNodeDisplayItems_Implementation() const
{
    TArray<FDreamFlowNodeDisplayItem> Items = Super::GetNodeDisplayItems_Implementation();
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Condition")), ConditionBinding.Describe()));
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Outputs")), TEXT("True / False")));
    return Items;
}

TArray<FDreamFlowNodeOutputPin> UDreamFlowBranchNode::GetOutputPins_Implementation() const
{
    FDreamFlowNodeOutputPin TruePin;
    TruePin.PinName = TEXT("True");
    TruePin.DisplayName = FText::FromString(TEXT("True"));

    FDreamFlowNodeOutputPin FalsePin;
    FalsePin.PinName = TEXT("False");
    FalsePin.DisplayName = FText::FromString(TEXT("False"));

    return { TruePin, FalsePin };
}

bool UDreamFlowBranchNode::SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;
    return Executor != nullptr;
}

FName UDreamFlowBranchNode::ResolveAutomaticTransitionOutputPin_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;

    bool bCondition = false;
    if (Executor == nullptr || !Executor->ResolveBindingAsBool(ConditionBinding, bCondition))
    {
        return NAME_None;
    }

    return bCondition ? FName(TEXT("True")) : FName(TEXT("False"));
}

void UDreamFlowBranchNode::ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    ValidateBinding(OwningAsset, this, ConditionBinding, FText::FromString(TEXT("Condition")), OutMessages);

    const int32 ChildCount = GetChildrenCopy().Num();
    if (ChildCount < 2)
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Warning,
            FText::FromString(TEXT("Branch nodes work best with two children. Child 0 is True and child 1 is False.")));
    }
    else if (ChildCount > 2)
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Info,
            FText::FromString(TEXT("Branch nodes only use the first two children. Additional links are ignored.")));
    }
}

UDreamFlowCompareNode::UDreamFlowCompareNode()
{
    Title = FText::FromString(TEXT("Compare"));
    Description = FText::FromString(TEXT("Compares two values and branches to child 0 when the comparison is true, otherwise child 1."));
    LeftValue.LiteralValue.Type = EDreamFlowValueType::Int;
    RightValue.LiteralValue.Type = EDreamFlowValueType::Int;

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.23f, 0.55f, 0.80f, 1.0f);
#endif
}

FText UDreamFlowCompareNode::GetNodeDisplayName_Implementation() const
{
    return Title;
}

FLinearColor UDreamFlowCompareNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.23f, 0.55f, 0.80f, 1.0f);
}

FText UDreamFlowCompareNode::GetNodeAccentLabel_Implementation() const
{
    return MakeComparisonLabel(Operation);
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowCompareNode::GetNodeDisplayItems_Implementation() const
{
    TArray<FDreamFlowNodeDisplayItem> Items = Super::GetNodeDisplayItems_Implementation();
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Left")), LeftValue.Describe()));
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Op")), MakeComparisonLabel(Operation).ToString()));
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Right")), RightValue.Describe()));
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Outputs")), TEXT("True / False")));
    return Items;
}

TArray<FDreamFlowNodeOutputPin> UDreamFlowCompareNode::GetOutputPins_Implementation() const
{
    FDreamFlowNodeOutputPin TruePin;
    TruePin.PinName = TEXT("True");
    TruePin.DisplayName = FText::FromString(TEXT("True"));

    FDreamFlowNodeOutputPin FalsePin;
    FalsePin.PinName = TEXT("False");
    FalsePin.DisplayName = FText::FromString(TEXT("False"));

    return { TruePin, FalsePin };
}

bool UDreamFlowCompareNode::SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;
    return Executor != nullptr;
}

FName UDreamFlowCompareNode::ResolveAutomaticTransitionOutputPin_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;

    FDreamFlowValue LeftResolvedValue;
    FDreamFlowValue RightResolvedValue;
    bool bComparisonResult = false;

    if (Executor == nullptr
        || !Executor->ResolveBindingValue(LeftValue, LeftResolvedValue)
        || !Executor->ResolveBindingValue(RightValue, RightResolvedValue)
        || !DreamFlowVariable::TryCompareValues(LeftResolvedValue, RightResolvedValue, Operation, bComparisonResult))
    {
        return NAME_None;
    }

    return bComparisonResult ? FName(TEXT("True")) : FName(TEXT("False"));
}

void UDreamFlowCompareNode::ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    ValidateBinding(OwningAsset, this, LeftValue, FText::FromString(TEXT("Left value")), OutMessages);
    ValidateBinding(OwningAsset, this, RightValue, FText::FromString(TEXT("Right value")), OutMessages);

    const int32 ChildCount = GetChildrenCopy().Num();
    if (ChildCount < 2)
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Warning,
            FText::FromString(TEXT("Compare nodes work best with two children. Child 0 is True and child 1 is False.")));
    }
    else if (ChildCount > 2)
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Info,
            FText::FromString(TEXT("Compare nodes only use the first two children. Additional links are ignored.")));
    }

    if (LeftValue.SourceType == EDreamFlowValueSourceType::Literal
        && RightValue.SourceType == EDreamFlowValueSourceType::Literal)
    {
        bool bCanCompare = false;
        bool bDummyResult = false;
        bCanCompare = DreamFlowVariable::TryCompareValues(LeftValue.LiteralValue, RightValue.LiteralValue, Operation, bDummyResult);
        if (!bCanCompare)
        {
            AddValidationMessage(
                OutMessages,
                this,
                EDreamFlowValidationSeverity::Warning,
                FText::FromString(TEXT("The current literal values cannot be compared with the selected operation.")));
        }
    }
}

UDreamFlowSetVariableNode::UDreamFlowSetVariableNode()
{
    Title = FText::FromString(TEXT("Set Variable"));
    Description = FText::FromString(TEXT("Writes a flow variable, then continues to the first child if one exists."));
    ValueBinding.LiteralValue.Type = EDreamFlowValueType::Bool;

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.18f, 0.66f, 0.42f, 1.0f);
#endif
}

FText UDreamFlowSetVariableNode::GetNodeDisplayName_Implementation() const
{
    return Title;
}

FLinearColor UDreamFlowSetVariableNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.18f, 0.66f, 0.42f, 1.0f);
}

FText UDreamFlowSetVariableNode::GetNodeAccentLabel_Implementation() const
{
    return TargetVariable.IsNone() ? FText::FromString(TEXT("Set")) : FText::FromName(TargetVariable);
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowSetVariableNode::GetNodeDisplayItems_Implementation() const
{
    TArray<FDreamFlowNodeDisplayItem> Items = Super::GetNodeDisplayItems_Implementation();
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Target")), TargetVariable.IsNone() ? TEXT("<variable>") : TargetVariable.ToString()));
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Value")), ValueBinding.Describe()));
    return Items;
}

void UDreamFlowSetVariableNode::ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor)
{
    Super::ExecuteNodeWithExecutor_Implementation(Context, Executor);

    if (Executor == nullptr || TargetVariable.IsNone())
    {
        return;
    }

    FDreamFlowValue ResolvedValue;
    if (Executor->ResolveBindingValue(ValueBinding, ResolvedValue))
    {
        Executor->SetVariableValue(TargetVariable, ResolvedValue);
    }
}

bool UDreamFlowSetVariableNode::SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;
    (void)Executor;
    return true;
}

FName UDreamFlowSetVariableNode::ResolveAutomaticTransitionOutputPin_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;
    (void)Executor;
    return FName(TEXT("Out"));
}

void UDreamFlowSetVariableNode::ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    if (TargetVariable.IsNone())
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Error,
            FText::FromString(TEXT("Set Variable nodes require a target variable name.")));
    }
    else if (OwningAsset != nullptr && !OwningAsset->HasVariableDefinition(TargetVariable))
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Error,
            FText::Format(
                FText::FromString(TEXT("Set Variable references the missing flow variable '{0}'.")),
                FText::FromName(TargetVariable)));
    }

    ValidateBinding(OwningAsset, this, ValueBinding, FText::FromString(TEXT("Value binding")), OutMessages);

    if (OwningAsset != nullptr && ValueBinding.SourceType == EDreamFlowValueSourceType::Literal)
    {
        const FDreamFlowVariableDefinition* VariableDefinition = OwningAsset->FindVariableDefinition(TargetVariable);
        if (VariableDefinition != nullptr)
        {
            FDreamFlowValue ConvertedValue;
            if (!DreamFlowVariable::TryConvertValue(ValueBinding.LiteralValue, VariableDefinition->DefaultValue.Type, ConvertedValue))
            {
                AddValidationMessage(
                    OutMessages,
                    this,
                    EDreamFlowValidationSeverity::Warning,
                    FText::FromString(TEXT("The literal value cannot be converted to the target variable's type.")));
            }
        }
    }

    if (GetChildrenCopy().Num() > 1)
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Info,
            FText::FromString(TEXT("Set Variable only auto-continues to the first child. Additional links are ignored.")));
    }
}
