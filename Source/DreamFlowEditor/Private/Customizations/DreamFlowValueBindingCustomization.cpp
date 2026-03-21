#include "Customizations/DreamFlowValueBindingCustomization.h"

#include "DreamFlowAsset.h"
#include "DreamFlowCoreNodes.h"
#include "DreamFlowVariableTypes.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FDreamFlowValueBindingCustomization::MakeInstance()
{
    return MakeShared<FDreamFlowValueBindingCustomization>();
}

void FDreamFlowValueBindingCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    StructHandle = StructPropertyHandle;
    PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
    SourceTypeHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValueBinding, SourceType));
    VariableNameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValueBinding, VariableName));
    LiteralValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValueBinding, LiteralValue));

    auto RequestRefresh = [WeakPropertyUtilities = TWeakPtr<IPropertyUtilities>(PropertyUtilities)]()
    {
        if (const TSharedPtr<IPropertyUtilities> PinnedPropertyUtilities = WeakPropertyUtilities.Pin())
        {
            PinnedPropertyUtilities->ForceRefresh();
        }
    };

    if (SourceTypeHandle.IsValid())
    {
        SourceTypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda(RequestRefresh));
    }

    if (VariableNameHandle.IsValid())
    {
        VariableNameHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda(RequestRefresh));
    }

    HeaderRow
        .NameContent()
        [
            StructPropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .MinDesiredWidth(360.0f)
        [
            SNew(STextBlock)
            .Text(this, &FDreamFlowValueBindingCustomization::GetBindingSummary)
            .ToolTipText(this, &FDreamFlowValueBindingCustomization::GetBindingSummary)
        ];
}

void FDreamFlowValueBindingCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    (void)StructPropertyHandle;
    (void)CustomizationUtils;

    StructBuilder.AddCustomRow(FText::FromString(TEXT("Binding Source")))
        .NameContent()
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("Source")))
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        .ValueContent()
        .MinDesiredWidth(360.0f)
        [
            SNew(SComboButton)
            .OnGetMenuContent(this, &FDreamFlowValueBindingCustomization::BuildSourceTypeMenu)
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text(this, &FDreamFlowValueBindingCustomization::GetSourceTypeLabel)
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
        ];

    if (const TOptional<EDreamFlowValueType> ExpectedValueType = ResolveExpectedValueType())
    {
        StructBuilder.AddCustomRow(FText::FromString(TEXT("Expected Type")))
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Expected Type")))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            .MinDesiredWidth(360.0f)
            [
                SNew(STextBlock)
                .Text(this, &FDreamFlowValueBindingCustomization::GetExpectedTypeLabel)
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ];
    }

    if (GetCurrentSourceType() == EDreamFlowValueSourceType::FlowVariable)
    {
        StructBuilder.AddCustomRow(FText::FromString(TEXT("Flow Variable")))
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Variable")))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            .MinDesiredWidth(360.0f)
            [
                SNew(SComboButton)
                .OnGetMenuContent(this, &FDreamFlowValueBindingCustomization::BuildVariableMenu)
                .ButtonContent()
                [
                    SNew(STextBlock)
                    .Text(this, &FDreamFlowValueBindingCustomization::GetVariablePickerLabel)
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                ]
            ];
    }
    else if (LiteralValueHandle.IsValid())
    {
        EnsureLiteralValueMatchesExpectedType();
        StructBuilder.AddProperty(LiteralValueHandle.ToSharedRef());
    }
}

EDreamFlowValueSourceType FDreamFlowValueBindingCustomization::GetCurrentSourceType() const
{
    uint8 EnumValue = static_cast<uint8>(EDreamFlowValueSourceType::Literal);
    return (SourceTypeHandle.IsValid() && SourceTypeHandle->GetValue(EnumValue) == FPropertyAccess::Success)
        ? static_cast<EDreamFlowValueSourceType>(EnumValue)
        : EDreamFlowValueSourceType::Literal;
}

UDreamFlowAsset* FDreamFlowValueBindingCustomization::ResolveOwningFlowAsset() const
{
    if (!StructHandle.IsValid())
    {
        return nullptr;
    }

    TArray<UObject*> OuterObjects;
    StructHandle->GetOuterObjects(OuterObjects);

    for (UObject* OuterObject : OuterObjects)
    {
        for (UObject* Current = OuterObject; Current != nullptr; Current = Current->GetOuter())
        {
            if (UDreamFlowAsset* FlowAsset = Cast<UDreamFlowAsset>(Current))
            {
                return FlowAsset;
            }
        }
    }

    return nullptr;
}

TOptional<EDreamFlowValueType> FDreamFlowValueBindingCustomization::ResolveExpectedValueType() const
{
    if (StructHandle.IsValid())
    {
        const FString MetaValue = StructHandle->GetMetaData(TEXT("DreamFlowExpectedValueType"));
        if (!MetaValue.IsEmpty())
        {
            if (const UEnum* ValueTypeEnum = StaticEnum<EDreamFlowValueType>())
            {
                int64 EnumValue = ValueTypeEnum->GetValueByNameString(MetaValue);
                if (EnumValue == INDEX_NONE)
                {
                    EnumValue = ValueTypeEnum->GetValueByNameString(FString::Printf(TEXT("EDreamFlowValueType::%s"), *MetaValue));
                }

                if (EnumValue != INDEX_NONE)
                {
                    return static_cast<EDreamFlowValueType>(EnumValue);
                }
            }
        }
    }

    TArray<UObject*> OuterObjects;
    if (StructHandle.IsValid())
    {
        StructHandle->GetOuterObjects(OuterObjects);
    }

    for (UObject* OuterObject : OuterObjects)
    {
        for (UObject* Current = OuterObject; Current != nullptr; Current = Current->GetOuter())
        {
            if (const UDreamFlowSetVariableNode* SetVariableNode = Cast<UDreamFlowSetVariableNode>(Current))
            {
                if (const UDreamFlowAsset* FlowAsset = ResolveOwningFlowAsset())
                {
                    if (const FDreamFlowVariableDefinition* VariableDefinition = FlowAsset->FindVariableDefinition(SetVariableNode->TargetVariable))
                    {
                        return VariableDefinition->DefaultValue.Type;
                    }
                }
            }
        }
    }

    return {};
}

bool FDreamFlowValueBindingCustomization::IsVariableDefinitionCompatible(const FDreamFlowVariableDefinition& VariableDefinition) const
{
    const TOptional<EDreamFlowValueType> ExpectedValueType = ResolveExpectedValueType();
    if (!ExpectedValueType.IsSet())
    {
        return true;
    }

    FDreamFlowValue ConvertedValue;
    return DreamFlowVariable::TryConvertValue(VariableDefinition.DefaultValue, ExpectedValueType.GetValue(), ConvertedValue);
}

FText FDreamFlowValueBindingCustomization::GetExpectedTypeLabel() const
{
    if (const TOptional<EDreamFlowValueType> ExpectedValueType = ResolveExpectedValueType())
    {
        if (const UEnum* ValueTypeEnum = StaticEnum<EDreamFlowValueType>())
        {
            return ValueTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(ExpectedValueType.GetValue()));
        }
    }

    return FText::FromString(TEXT("Any"));
}

FText FDreamFlowValueBindingCustomization::GetBindingSummary() const
{
    if (GetCurrentSourceType() == EDreamFlowValueSourceType::FlowVariable)
    {
        FName VariableName = NAME_None;
        if (VariableNameHandle.IsValid())
        {
            VariableNameHandle->GetValue(VariableName);
        }

        if (VariableName.IsNone())
        {
            return FText::FromString(TEXT("Flow Variable: <Select Variable>"));
        }

        if (const UDreamFlowAsset* FlowAsset = ResolveOwningFlowAsset())
        {
            if (const FDreamFlowVariableDefinition* VariableDefinition = FlowAsset->FindVariableDefinition(VariableName))
            {
                static const UEnum* ValueTypeEnum = StaticEnum<EDreamFlowValueType>();
                const FString TypeLabel = ValueTypeEnum != nullptr
                    ? ValueTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(VariableDefinition->DefaultValue.Type)).ToString()
                    : TEXT("Value");
                return FText::FromString(FString::Printf(TEXT("Flow Variable: %s (%s)"), *VariableName.ToString(), *TypeLabel));
            }
        }

        return FText::FromString(FString::Printf(TEXT("Flow Variable: %s"), *VariableName.ToString()));
    }

    FText LiteralDisplayText;
    if (LiteralValueHandle.IsValid() && LiteralValueHandle->GetValueAsDisplayText(LiteralDisplayText) == FPropertyAccess::Success)
    {
        return FText::Format(FText::FromString(TEXT("Literal: {0}")), LiteralDisplayText);
    }

    return FText::FromString(TEXT("Literal"));
}

FText FDreamFlowValueBindingCustomization::GetSourceTypeLabel() const
{
    return GetCurrentSourceType() == EDreamFlowValueSourceType::FlowVariable
        ? FText::FromString(TEXT("Flow Variable"))
        : FText::FromString(TEXT("Literal"));
}

FText FDreamFlowValueBindingCustomization::GetVariablePickerLabel() const
{
    FName VariableName = NAME_None;
    if (VariableNameHandle.IsValid())
    {
        VariableNameHandle->GetValue(VariableName);
    }

    if (VariableName.IsNone())
    {
        return FText::FromString(TEXT("Select Variable"));
    }

    if (const UDreamFlowAsset* FlowAsset = ResolveOwningFlowAsset())
    {
        if (const FDreamFlowVariableDefinition* VariableDefinition = FlowAsset->FindVariableDefinition(VariableName))
        {
            if (const UEnum* ValueTypeEnum = StaticEnum<EDreamFlowValueType>())
            {
                return FText::Format(
                    FText::FromString(TEXT("{0} ({1})")),
                    FText::FromName(VariableName),
                    ValueTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(VariableDefinition->DefaultValue.Type)));
            }
        }
    }

    return FText::FromName(VariableName);
}

void FDreamFlowValueBindingCustomization::EnsureLiteralValueMatchesExpectedType() const
{
    if (!LiteralValueHandle.IsValid())
    {
        return;
    }

    const TOptional<EDreamFlowValueType> ExpectedValueType = ResolveExpectedValueType();
    if (!ExpectedValueType.IsSet())
    {
        return;
    }

    TSharedPtr<IPropertyHandle> TypeHandle = LiteralValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, Type));
    if (!TypeHandle.IsValid())
    {
        return;
    }

    uint8 CurrentTypeValue = static_cast<uint8>(EDreamFlowValueType::Bool);
    if (TypeHandle->GetValue(CurrentTypeValue) != FPropertyAccess::Success
        || static_cast<EDreamFlowValueType>(CurrentTypeValue) != ExpectedValueType.GetValue())
    {
        TypeHandle->SetValue(static_cast<uint8>(ExpectedValueType.GetValue()));
    }
}

void FDreamFlowValueBindingCustomization::SetSourceType(EDreamFlowValueSourceType NewSourceType) const
{
    if (SourceTypeHandle.IsValid())
    {
        SourceTypeHandle->SetValue(static_cast<uint8>(NewSourceType));
    }
}

void FDreamFlowValueBindingCustomization::SetVariableName(FName NewVariableName) const
{
    if (VariableNameHandle.IsValid())
    {
        VariableNameHandle->SetValue(NewVariableName);
    }
}

TSharedRef<SWidget> FDreamFlowValueBindingCustomization::BuildSourceTypeMenu() const
{
    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.AddMenuEntry(
        FText::FromString(TEXT("Literal")),
        FText::FromString(TEXT("Use a literal value directly on this node.")),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateSP(const_cast<FDreamFlowValueBindingCustomization*>(this), &FDreamFlowValueBindingCustomization::SetSourceType, EDreamFlowValueSourceType::Literal)));

    MenuBuilder.AddMenuEntry(
        FText::FromString(TEXT("Flow Variable")),
        FText::FromString(TEXT("Read the value from one of the flow's environment variables.")),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateSP(const_cast<FDreamFlowValueBindingCustomization*>(this), &FDreamFlowValueBindingCustomization::SetSourceType, EDreamFlowValueSourceType::FlowVariable)));

    return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FDreamFlowValueBindingCustomization::BuildVariableMenu() const
{
    FMenuBuilder MenuBuilder(true, nullptr);
    MenuBuilder.BeginSection(NAME_None, FText::FromString(TEXT("Flow Variables")));

    bool bAddedAnyVariable = false;
    if (const UDreamFlowAsset* FlowAsset = ResolveOwningFlowAsset())
    {
        static const UEnum* ValueTypeEnum = StaticEnum<EDreamFlowValueType>();
        for (const FDreamFlowVariableDefinition& VariableDefinition : FlowAsset->Variables)
        {
            if (VariableDefinition.Name.IsNone() || !IsVariableDefinitionCompatible(VariableDefinition))
            {
                continue;
            }

            bAddedAnyVariable = true;
            const FString TypeLabel = ValueTypeEnum != nullptr
                ? ValueTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(VariableDefinition.DefaultValue.Type)).ToString()
                : TEXT("Value");

            const FText ToolTipText = VariableDefinition.Description.IsEmpty()
                ? FText::FromString(TypeLabel)
                : FText::Format(FText::FromString(TEXT("{0}\nType: {1}")), VariableDefinition.Description, FText::FromString(TypeLabel));

            MenuBuilder.AddMenuEntry(
                FText::FromString(FString::Printf(TEXT("%s (%s)"), *VariableDefinition.Name.ToString(), *TypeLabel)),
                ToolTipText,
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateSP(const_cast<FDreamFlowValueBindingCustomization*>(this), &FDreamFlowValueBindingCustomization::SetVariableName, VariableDefinition.Name)));
        }
    }

    if (!bAddedAnyVariable)
    {
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("No Compatible Variables")),
            FText::FromString(TEXT("No flow variables match the expected type for this binding.")),
            FSlateIcon(),
            FUIAction());
    }

    MenuBuilder.EndSection();
    return MenuBuilder.MakeWidget();
}
