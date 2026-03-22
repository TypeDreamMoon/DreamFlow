#include "DreamFlowCoreNodes.h"

#include "DreamFlowAsset.h"
#include "DreamFlowLog.h"
#include "Execution/DreamFlowAsyncContext.h"
#include "Execution/DreamFlowExecutor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

    static bool IsDefaultLiteralBinding(const FDreamFlowValueBinding& Binding, const FDreamFlowValue& DefaultValue)
    {
        if (Binding.SourceType != EDreamFlowValueSourceType::Literal || !Binding.VariableName.IsNone())
        {
            return false;
        }

        bool bEqual = false;
        return DreamFlowVariable::TryCompareValues(Binding.LiteralValue, DefaultValue, EDreamFlowComparisonOperation::Equal, bEqual) && bEqual;
    }

    static void SyncSharedVariables(UDreamFlowExecutor* SourceExecutor, UDreamFlowExecutor* TargetExecutor)
    {
        if (SourceExecutor == nullptr || TargetExecutor == nullptr || TargetExecutor->GetFlowAsset() == nullptr)
        {
            return;
        }

        for (const FDreamFlowVariableDefinition& TargetDefinition : TargetExecutor->GetFlowAsset()->Variables)
        {
            if (TargetDefinition.Name.IsNone() || !SourceExecutor->HasVariable(TargetDefinition.Name))
            {
                continue;
            }

            FDreamFlowValue SourceValue;
            if (SourceExecutor->GetVariableValue(TargetDefinition.Name, SourceValue))
            {
                TargetExecutor->SetVariableValue(TargetDefinition.Name, SourceValue);
            }
        }
    }
}

FText UDreamFlowCoreNode::GetNodeCategory_Implementation() const
{
    return FText::FromString(TEXT("Core"));
}

void UDreamFlowSubFlowAsyncProxy::Initialize(
    UDreamFlowAsyncContext* InParentAsyncContext,
    UDreamFlowExecutor* InParentExecutor,
    UDreamFlowExecutor* InChildExecutor,
    const bool bInCopyParentVariablesToChild,
    const bool bInCopyChildVariablesToParent)
{
    ParentAsyncContext = InParentAsyncContext;
    ParentExecutor = InParentExecutor;
    ChildExecutor = InChildExecutor;
    bCopyParentVariablesToChild = bInCopyParentVariablesToChild;
    bCopyChildVariablesToParent = bInCopyChildVariablesToParent;
}

void UDreamFlowSubFlowAsyncProxy::StartSubFlow()
{
    if (ParentAsyncContext == nullptr || ParentExecutor == nullptr || ChildExecutor == nullptr)
    {
        FinalizeSubFlow(TEXT("Failed"), false);
        return;
    }

    ChildRuntimeStateChangedHandle = ChildExecutor->OnRuntimeStateChangedNative().AddUObject(this, &ThisClass::HandleChildRuntimeStateChanged);

    if (bCopyParentVariablesToChild)
    {
        SyncVariablesFromParentToChild();
    }

    if (!ChildExecutor->StartFlow())
    {
        DREAMFLOW_LOG(
            Execution,
            Warning,
            "Sub flow node failed to start child flow '%s'.",
            *GetNameSafe(ChildExecutor->GetFlowAsset()));
        FinalizeSubFlow(TEXT("Failed"), false);
        return;
    }

    if (!bHasCompleted)
    {
        DriveChildFlow();
    }
}

void UDreamFlowSubFlowAsyncProxy::BeginDestroy()
{
    if (ChildExecutor != nullptr && ChildRuntimeStateChangedHandle.IsValid())
    {
        ChildExecutor->OnRuntimeStateChangedNative().Remove(ChildRuntimeStateChangedHandle);
        ChildRuntimeStateChangedHandle.Reset();
    }

    if (!bHasCompleted && ChildExecutor != nullptr && ChildExecutor->IsRunning())
    {
        ChildExecutor->FinishFlow();
    }

    Super::BeginDestroy();
}

void UDreamFlowSubFlowAsyncProxy::DriveChildFlow()
{
    if (bHasCompleted || ChildExecutor == nullptr)
    {
        return;
    }

    while (ChildExecutor->IsRunning())
    {
        if (ChildExecutor->IsPaused())
        {
            DREAMFLOW_LOG(
                Execution,
                Warning,
                "Sub flow '%s' paused while running inside another flow. Completing through Failed.",
                *GetNameSafe(ChildExecutor->GetFlowAsset()));
            FinalizeSubFlow(TEXT("Failed"), false);
            return;
        }

        if (ChildExecutor->IsWaitingForAsyncNode())
        {
            return;
        }

        if (ChildExecutor->IsWaitingForAdvance())
        {
            if (!ChildExecutor->Advance())
            {
                DREAMFLOW_LOG(
                    Execution,
                    Warning,
                    "Sub flow '%s' blocked on a manual choice while running as a child flow. Completing through Failed.",
                    *GetNameSafe(ChildExecutor->GetFlowAsset()));
                FinalizeSubFlow(TEXT("Failed"), false);
                return;
            }

            continue;
        }

        break;
    }

    if (!ChildExecutor->IsRunning())
    {
        FinalizeSubFlow(TEXT("Completed"), true);
    }
}

void UDreamFlowSubFlowAsyncProxy::FinalizeSubFlow(const FName OutputPinName, const bool bSyncChildVariables)
{
    if (bHasCompleted)
    {
        return;
    }

    bHasCompleted = true;

    if (ChildExecutor != nullptr && ChildRuntimeStateChangedHandle.IsValid())
    {
        ChildExecutor->OnRuntimeStateChangedNative().Remove(ChildRuntimeStateChangedHandle);
        ChildRuntimeStateChangedHandle.Reset();
    }

    if (bSyncChildVariables && bCopyChildVariablesToParent)
    {
        SyncVariablesFromChildToParent();
    }

    if (ParentAsyncContext != nullptr && ParentAsyncContext->IsValidAsyncContext())
    {
        ParentAsyncContext->CompleteWithOutputPin(OutputPinName);
    }
}

void UDreamFlowSubFlowAsyncProxy::HandleChildRuntimeStateChanged(UDreamFlowExecutor* InUpdatedExecutor)
{
    if (bHasCompleted || InUpdatedExecutor == nullptr || InUpdatedExecutor != ChildExecutor)
    {
        return;
    }

    DriveChildFlow();
}

void UDreamFlowSubFlowAsyncProxy::SyncVariablesFromParentToChild() const
{
    SyncSharedVariables(ParentExecutor, ChildExecutor);
}

void UDreamFlowSubFlowAsyncProxy::SyncVariablesFromChildToParent() const
{
    SyncSharedVariables(ChildExecutor, ParentExecutor);
}

UDreamFlowBranchNode::UDreamFlowBranchNode()
{
    Title = FText::FromString(TEXT("Branch"));
    Description = FText::FromString(TEXT("Chooses the first child when the condition is true, otherwise the second child."));
    ConditionBinding.LiteralValue.Type = EDreamFlowValueType::Bool;
    TransitionMode = EDreamFlowNodeTransitionMode::Automatic;

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
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Condition")), ConditionBinding.DescribeCompact()));
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
    return Executor != nullptr && GetTransitionMode() == EDreamFlowNodeTransitionMode::Automatic;
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
    TransitionMode = EDreamFlowNodeTransitionMode::Automatic;

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
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Left")), LeftValue.DescribeCompact()));
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Op")), MakeComparisonLabel(Operation).ToString()));
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Right")), RightValue.DescribeCompact()));
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
    return Executor != nullptr && GetTransitionMode() == EDreamFlowNodeTransitionMode::Automatic;
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
    TransitionMode = EDreamFlowNodeTransitionMode::Automatic;

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
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Value")), ValueBinding.DescribeCompact()));
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
    return GetTransitionMode() == EDreamFlowNodeTransitionMode::Automatic;
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

UDreamFlowRunSubFlowNode::UDreamFlowRunSubFlowNode()
{
    Title = FText::FromString(TEXT("Run Sub Flow"));
    Description = FText::FromString(TEXT("Runs another flow asset as a child flow, waits for it to finish, then continues."));
    SubFlowExecutorClass = UDreamFlowExecutor::StaticClass();

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.16f, 0.44f, 0.68f, 1.0f);
#endif
}

FText UDreamFlowRunSubFlowNode::GetNodeDisplayName_Implementation() const
{
    return Title;
}

FLinearColor UDreamFlowRunSubFlowNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.16f, 0.44f, 0.68f, 1.0f);
}

FText UDreamFlowRunSubFlowNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("Sub Flow"));
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowRunSubFlowNode::GetNodeDisplayItems_Implementation() const
{
    TArray<FDreamFlowNodeDisplayItem> Items = Super::GetNodeDisplayItems_Implementation();
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Flow")), GetNameSafe(SubFlowAsset)));
    Items.Add(MakeTextPreviewItem(
        FText::FromString(TEXT("Sync In")),
        bCopyParentVariablesToChild ? TEXT("Shared variables") : TEXT("Disabled")));
    Items.Add(MakeTextPreviewItem(
        FText::FromString(TEXT("Sync Out")),
        bCopyChildVariablesToParent ? TEXT("Shared variables") : TEXT("Disabled")));
    return Items;
}

TArray<FDreamFlowNodeOutputPin> UDreamFlowRunSubFlowNode::GetOutputPins_Implementation() const
{
    FDreamFlowNodeOutputPin CompletedPin;
    CompletedPin.PinName = TEXT("Completed");
    CompletedPin.DisplayName = FText::FromString(TEXT("Completed"));

    FDreamFlowNodeOutputPin FailedPin;
    FailedPin.PinName = TEXT("Failed");
    FailedPin.DisplayName = FText::FromString(TEXT("Failed"));

    return { CompletedPin, FailedPin };
}

void UDreamFlowRunSubFlowNode::StartAsyncNode_Implementation(UObject* Context, UDreamFlowExecutor* Executor, UDreamFlowAsyncContext* AsyncContext)
{
    if (AsyncContext == nullptr)
    {
        return;
    }

    if (Executor == nullptr || SubFlowAsset == nullptr)
    {
        DREAMFLOW_LOG(
            Execution,
            Warning,
            "Run Sub Flow node '%s' is missing an executor or child flow asset. Completing through Failed.",
            *GetNodeDisplayName().ToString());
        AsyncContext->CompleteWithOutputPin(TEXT("Failed"));
        return;
    }

    UClass* EffectiveExecutorClass = SubFlowExecutorClass != nullptr ? SubFlowExecutorClass.Get() : UDreamFlowExecutor::StaticClass();
    UDreamFlowSubFlowAsyncProxy* Proxy = NewObject<UDreamFlowSubFlowAsyncProxy>(AsyncContext, NAME_None, RF_Transient);
    UDreamFlowExecutor* ChildExecutor = Proxy != nullptr
        ? NewObject<UDreamFlowExecutor>(Proxy, EffectiveExecutorClass, NAME_None, RF_Transient)
        : nullptr;

    if (Proxy == nullptr || ChildExecutor == nullptr)
    {
        DREAMFLOW_LOG(
            Execution,
            Warning,
            "Run Sub Flow node '%s' failed to create a child executor for asset '%s'.",
            *GetNodeDisplayName().ToString(),
            *GetNameSafe(SubFlowAsset));
        AsyncContext->CompleteWithOutputPin(TEXT("Failed"));
        return;
    }

    UObject* ChildExecutionContext = Context != nullptr ? Context : Executor->GetExecutionContext();
    ChildExecutor->Initialize(SubFlowAsset, ChildExecutionContext);

    Proxy->Initialize(AsyncContext, Executor, ChildExecutor, bCopyParentVariablesToChild, bCopyChildVariablesToParent);
    Proxy->StartSubFlow();
}

void UDreamFlowRunSubFlowNode::ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    if (SubFlowAsset == nullptr)
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Error,
            FText::FromString(TEXT("Run Sub Flow requires a child flow asset.")));
    }
    else
    {
        if (SubFlowAsset->GetEntryNode() == nullptr)
        {
            AddValidationMessage(
                OutMessages,
                this,
                EDreamFlowValidationSeverity::Error,
                FText::FromString(TEXT("The selected child flow asset does not have a valid entry node.")));
        }

        if (SubFlowAsset == OwningAsset)
        {
            AddValidationMessage(
                OutMessages,
                this,
                EDreamFlowValidationSeverity::Warning,
                FText::FromString(TEXT("Run Sub Flow references its own flow asset. This can recurse indefinitely if the child path reaches the same node again.")));
        }
    }

    if (GetChildrenCopy().Num() > 2)
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Info,
            FText::FromString(TEXT("Run Sub Flow only uses the Completed and Failed outputs. Additional links are ignored.")));
    }
}

UDreamFlowFinishFlowNode::UDreamFlowFinishFlowNode()
{
    Title = FText::FromString(TEXT("Finish Flow"));
    Description = FText::FromString(TEXT("Immediately ends the current flow execution."));

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.78f, 0.20f, 0.25f, 1.0f);
#endif
}

FText UDreamFlowFinishFlowNode::GetNodeDisplayName_Implementation() const
{
    return Title;
}

FLinearColor UDreamFlowFinishFlowNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.78f, 0.20f, 0.25f, 1.0f);
}

FText UDreamFlowFinishFlowNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("Finish"));
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowFinishFlowNode::GetNodeDisplayItems_Implementation() const
{
    TArray<FDreamFlowNodeDisplayItem> Items = Super::GetNodeDisplayItems_Implementation();
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Action")), TEXT("End Execution")));
    return Items;
}

TArray<FDreamFlowNodeOutputPin> UDreamFlowFinishFlowNode::GetOutputPins_Implementation() const
{
    return {};
}

bool UDreamFlowFinishFlowNode::SupportsMultipleChildren_Implementation() const
{
    return false;
}

bool UDreamFlowFinishFlowNode::IsTerminalNode_Implementation() const
{
    return true;
}

bool UDreamFlowFinishFlowNode::CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const
{
    (void)OtherNode;
    return false;
}

void UDreamFlowFinishFlowNode::ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor)
{
    Super::ExecuteNodeWithExecutor_Implementation(Context, Executor);
    (void)Context;

    if (Executor != nullptr)
    {
        Executor->FinishFlow();
    }
}

void UDreamFlowFinishFlowNode::ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    (void)OwningAsset;

    if (GetChildrenCopy().Num() > 0)
    {
        AddValidationMessage(
            OutMessages,
            this,
            EDreamFlowValidationSeverity::Info,
            FText::FromString(TEXT("Finish Flow ends execution immediately and ignores outgoing links.")));
    }
}

UDreamFlowDelayNode::UDreamFlowDelayNode()
{
    Title = FText::FromString(TEXT("Delay"));
    Description = FText::FromString(TEXT("Waits for a duration, then resumes through the Completed output."));
    DurationBinding.LiteralValue.Type = EDreamFlowValueType::Float;
    DurationBinding.LiteralValue.FloatValue = 1.0f;

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.58f, 0.34f, 0.84f, 1.0f);
#endif
}

void UDreamFlowDelayNode::PostLoad()
{
    Super::PostLoad();

    FDreamFlowValue DefaultDurationValue;
    DefaultDurationValue.Type = EDreamFlowValueType::Float;
    DefaultDurationValue.FloatValue = 1.0f;

    if (IsDefaultLiteralBinding(DurationBinding, DefaultDurationValue) && !FMath::IsNearlyEqual(DurationSeconds, 1.0f))
    {
        DurationBinding.SourceType = EDreamFlowValueSourceType::Literal;
        DurationBinding.VariableName = NAME_None;
        DurationBinding.LiteralValue = DefaultDurationValue;
        DurationBinding.LiteralValue.FloatValue = DurationSeconds;
    }
}

FText UDreamFlowDelayNode::GetNodeDisplayName_Implementation() const
{
    return Title;
}

FLinearColor UDreamFlowDelayNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.58f, 0.34f, 0.84f, 1.0f);
}

FText UDreamFlowDelayNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("Delay"));
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowDelayNode::GetNodeDisplayItems_Implementation() const
{
    TArray<FDreamFlowNodeDisplayItem> Items = Super::GetNodeDisplayItems_Implementation();
    Items.Add(MakeTextPreviewItem(FText::FromString(TEXT("Duration")), DurationBinding.DescribeCompact()));
    return Items;
}

void UDreamFlowDelayNode::StartAsyncNode_Implementation(UObject* Context, UDreamFlowExecutor* Executor, UDreamFlowAsyncContext* AsyncContext)
{
    if (AsyncContext == nullptr)
    {
        return;
    }

    float ResolvedDurationSeconds = 0.0f;
    if (Executor == nullptr || !Executor->GetBindingFloatValue(DurationBinding, ResolvedDurationSeconds))
    {
        DREAMFLOW_LOG(Execution, Warning, "Delay node '%s' failed to resolve its duration binding. Completing immediately.", *GetNodeDisplayName().ToString());
        AsyncContext->Complete();
        return;
    }

    if (ResolvedDurationSeconds <= KINDA_SMALL_NUMBER)
    {
        AsyncContext->Complete();
        return;
    }

    UObject* WorldContextObject = Context != nullptr
        ? Context
        : (Executor != nullptr ? Executor->GetExecutionContext() : nullptr);
    UWorld* World = GEngine != nullptr
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
        : nullptr;

    if (World == nullptr)
    {
        DREAMFLOW_LOG(Execution, Warning, "Delay node '%s' could not resolve a world from the execution context. Completing immediately.", *GetNodeDisplayName().ToString());
        AsyncContext->Complete();
        return;
    }

    TWeakObjectPtr<UDreamFlowAsyncContext> WeakAsyncContext = AsyncContext;
    FTimerDelegate CompletionDelegate = FTimerDelegate::CreateLambda([WeakAsyncContext]()
    {
        if (WeakAsyncContext.IsValid())
        {
            WeakAsyncContext->Complete();
        }
    });

    FTimerHandle DelayTimerHandle;
    World->GetTimerManager().SetTimer(DelayTimerHandle, CompletionDelegate, ResolvedDurationSeconds, false);
}

void UDreamFlowDelayNode::ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    ValidateBinding(OwningAsset, this, DurationBinding, FText::FromString(TEXT("Duration binding")), OutMessages);

    if (DurationBinding.SourceType == EDreamFlowValueSourceType::Literal)
    {
        FDreamFlowValue ConvertedValue;
        if (!DreamFlowVariable::TryConvertValue(DurationBinding.LiteralValue, EDreamFlowValueType::Float, ConvertedValue))
        {
            AddValidationMessage(
                OutMessages,
                this,
                EDreamFlowValidationSeverity::Warning,
                FText::FromString(TEXT("The delay duration must resolve to a float-compatible value.")));
        }
    }
}
