#include "Customizations/DreamFlowValueCustomization.h"

#include "DetailWidgetRow.h"
#include "IPropertyUtilities.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SBoxPanel.h"

TSharedRef<IPropertyTypeCustomization> FDreamFlowValueCustomization::MakeInstance()
{
    return MakeShared<FDreamFlowValueCustomization>();
}

void FDreamFlowValueCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
    TypeHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, Type));
    BoolHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, BoolValue));
    IntHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, IntValue));
    FloatHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, FloatValue));
    NameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, NameValue));
    StringHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, StringValue));
    TextHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, TextValue));
    GameplayTagHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, GameplayTagValue));

    if (TypeHandle.IsValid() && PropertyUtilities.IsValid())
    {
        TypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([WeakPropertyUtilities = TWeakPtr<IPropertyUtilities>(PropertyUtilities)]()
        {
            if (const TSharedPtr<IPropertyUtilities> PinnedPropertyUtilities = WeakPropertyUtilities.Pin())
            {
                PinnedPropertyUtilities->ForceRefresh();
            }
        }));
    }

    HeaderRow
        .NameContent()
        [
            StructPropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .MinDesiredWidth(320.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                TypeHandle.IsValid() ? TypeHandle->CreatePropertyValueWidget() : SNullWidget::NullWidget
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
            .VAlign(VAlign_Center)
            [
                GetActiveValueHandle().IsValid() ? GetActiveValueHandle()->CreatePropertyValueWidget() : SNullWidget::NullWidget
            ]
        ];
}

void FDreamFlowValueCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    (void)StructPropertyHandle;
    (void)StructBuilder;
    (void)CustomizationUtils;
}

TSharedPtr<IPropertyHandle> FDreamFlowValueCustomization::GetActiveValueHandle() const
{
    switch (GetCurrentValueType())
    {
    case EDreamFlowValueType::Bool:
        return BoolHandle;
    case EDreamFlowValueType::Int:
        return IntHandle;
    case EDreamFlowValueType::Float:
        return FloatHandle;
    case EDreamFlowValueType::Name:
        return NameHandle;
    case EDreamFlowValueType::String:
        return StringHandle;
    case EDreamFlowValueType::Text:
        return TextHandle;
    case EDreamFlowValueType::GameplayTag:
        return GameplayTagHandle;
    default:
        return nullptr;
    }
}

EDreamFlowValueType FDreamFlowValueCustomization::GetCurrentValueType() const
{
    uint8 EnumValue = static_cast<uint8>(EDreamFlowValueType::Bool);
    return (TypeHandle.IsValid() && TypeHandle->GetValue(EnumValue) == FPropertyAccess::Success)
        ? static_cast<EDreamFlowValueType>(EnumValue)
        : EDreamFlowValueType::Bool;
}
