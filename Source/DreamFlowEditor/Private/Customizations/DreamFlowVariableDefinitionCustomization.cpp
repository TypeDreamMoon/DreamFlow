#include "Customizations/DreamFlowVariableDefinitionCustomization.h"

#include "DreamFlowVariableTypes.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FDreamFlowVariableDefinitionCustomization::MakeInstance()
{
    return MakeShared<FDreamFlowVariableDefinitionCustomization>();
}

void FDreamFlowVariableDefinitionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
    NameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, Name));
    DescriptionHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, Description));
    DefaultValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, DefaultValue));

    if (NameHandle.IsValid() && PropertyUtilities.IsValid())
    {
        NameHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([WeakPropertyUtilities = TWeakPtr<IPropertyUtilities>(PropertyUtilities)]()
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
            SNew(STextBlock)
            .Text(this, &FDreamFlowVariableDefinitionCustomization::GetHeaderText)
            .ToolTipText(this, &FDreamFlowVariableDefinitionCustomization::GetHeaderText)
        ];
}

void FDreamFlowVariableDefinitionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    (void)StructPropertyHandle;
    (void)CustomizationUtils;

    if (NameHandle.IsValid())
    {
        StructBuilder.AddProperty(NameHandle.ToSharedRef());
    }

    if (DescriptionHandle.IsValid())
    {
        StructBuilder.AddProperty(DescriptionHandle.ToSharedRef());
    }

    if (DefaultValueHandle.IsValid())
    {
        StructBuilder.AddProperty(DefaultValueHandle.ToSharedRef());
    }
}

FText FDreamFlowVariableDefinitionCustomization::GetHeaderText() const
{
    FName VariableName = NAME_None;
    if (NameHandle.IsValid())
    {
        NameHandle->GetValue(VariableName);
    }

    FText DefaultValueText;
    if (DefaultValueHandle.IsValid())
    {
        DefaultValueHandle->GetValueAsDisplayText(DefaultValueText);
    }

    const FText NameText = VariableName.IsNone() ? FText::FromString(TEXT("<Variable>")) : FText::FromName(VariableName);
    return DefaultValueText.IsEmpty()
        ? NameText
        : FText::Format(FText::FromString(TEXT("{0}  |  {1}")), NameText, DefaultValueText);
}
