#include "Customizations/DreamFlowVariablesEditorDataDetails.h"

#include "DreamFlowVariablesEditorData.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DreamFlowVariablesEditorDataDetails"

TSharedRef<IDetailCustomization> FDreamFlowVariablesEditorDataDetails::MakeInstance()
{
    return MakeShared<FDreamFlowVariablesEditorDataDetails>();
}

void FDreamFlowVariablesEditorDataDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TSharedPtr<IPropertyHandle> VariablesHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowVariablesEditorData, Variables));
    if (!VariablesHandle.IsValid())
    {
        return;
    }

    VariablesHandle->MarkHiddenByCustomization();

    IDetailCategoryBuilder& VariablesCategory = DetailBuilder.EditCategory(TEXT("Variables"), LOCTEXT("VariablesCategory", "Variables"));
    VariablesCategory.SetSortOrder(0);
    VariablesCategory.HeaderContent(
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(SImage)
            .Image(FAppStyle::Get().GetBrush("Kismet.Tabs.Variables"))
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(FMargin(6.0f, 0.0f, 0.0f, 0.0f))
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("VariablesHeader", "Flow Variables"))
            .Font(IDetailLayoutBuilder::GetDetailFontBold())
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Center)
        [
            VariablesHandle->CreateDefaultPropertyButtonWidgets()
        ],
        true);

    TSharedRef<FDetailArrayBuilder> VariablesArrayBuilder = MakeShared<FDetailArrayBuilder>(VariablesHandle.ToSharedRef(), false, true, false);
    VariablesArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateLambda(
        [](TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
        {
            (void)ArrayIndex;
            ChildrenBuilder.AddProperty(PropertyHandle);
        }));

    VariablesCategory.AddCustomBuilder(VariablesArrayBuilder, false);
}

#undef LOCTEXT_NAMESPACE
