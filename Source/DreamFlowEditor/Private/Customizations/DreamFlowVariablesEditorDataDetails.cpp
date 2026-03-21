#include "Customizations/DreamFlowVariablesEditorDataDetails.h"

#include "DreamFlowVariablesEditorData.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
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
    VariablesHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowVariablesEditorData, Variables));
    PropertyUtilities = DetailBuilder.GetPropertyUtilities();
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
            SNew(SComboButton)
            .OnGetMenuContent(this, &FDreamFlowVariablesEditorDataDetails::BuildAddVariableMenu)
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("AddVariableButton", "Add Variable"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
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

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildAddVariableMenu() const
{
    FMenuBuilder MenuBuilder(true, nullptr);

    const auto AddTypeEntry = [this, &MenuBuilder](const EDreamFlowValueType ValueType, const TCHAR* Label, const TCHAR* Tooltip)
    {
        MenuBuilder.AddMenuEntry(
            FText::FromString(Label),
            FText::FromString(Tooltip),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(const_cast<FDreamFlowVariablesEditorDataDetails*>(this), &FDreamFlowVariablesEditorDataDetails::AddVariable, ValueType)));
    };

    AddTypeEntry(EDreamFlowValueType::Bool, TEXT("Bool"), TEXT("Add a boolean flow variable."));
    AddTypeEntry(EDreamFlowValueType::Int, TEXT("Int"), TEXT("Add an integer flow variable."));
    AddTypeEntry(EDreamFlowValueType::Float, TEXT("Float"), TEXT("Add a float flow variable."));
    AddTypeEntry(EDreamFlowValueType::Name, TEXT("Name"), TEXT("Add a name flow variable."));
    AddTypeEntry(EDreamFlowValueType::String, TEXT("String"), TEXT("Add a string flow variable."));
    AddTypeEntry(EDreamFlowValueType::Text, TEXT("Text"), TEXT("Add a localized text flow variable."));
    AddTypeEntry(EDreamFlowValueType::GameplayTag, TEXT("Gameplay Tag"), TEXT("Add a gameplay tag flow variable."));

    return MenuBuilder.MakeWidget();
}

void FDreamFlowVariablesEditorDataDetails::AddVariable(EDreamFlowValueType ValueType) const
{
    if (!VariablesHandle.IsValid())
    {
        return;
    }

    TSharedPtr<IPropertyHandleArray> VariablesArrayHandle = VariablesHandle->AsArray();
    if (!VariablesArrayHandle.IsValid())
    {
        return;
    }

    const FPropertyHandleItemAddResult AddResult = VariablesArrayHandle->AddItem();
    if (AddResult.GetAccessResult() != FPropertyAccess::Success || AddResult.GetIndex() == INDEX_NONE)
    {
        return;
    }

    TSharedPtr<IPropertyHandle> ElementHandle = VariablesArrayHandle->GetElement(AddResult.GetIndex());
    if (!ElementHandle.IsValid())
    {
        return;
    }

    TSet<FName> ExistingNames;
    uint32 ElementCount = 0;
    VariablesArrayHandle->GetNumElements(ElementCount);
    for (uint32 Index = 0; Index < ElementCount; ++Index)
    {
        if (static_cast<int32>(Index) == AddResult.GetIndex())
        {
            continue;
        }

        TSharedPtr<IPropertyHandle> ExistingElementHandle = VariablesArrayHandle->GetElement(Index);
        TSharedPtr<IPropertyHandle> ExistingNameHandle = ExistingElementHandle.IsValid()
            ? ExistingElementHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, Name))
            : nullptr;

        FName ExistingName = NAME_None;
        if (ExistingNameHandle.IsValid() && ExistingNameHandle->GetValue(ExistingName) == FPropertyAccess::Success && !ExistingName.IsNone())
        {
            ExistingNames.Add(ExistingName);
        }
    }

    const UEnum* ValueTypeEnum = StaticEnum<EDreamFlowValueType>();
    const FString TypeName = ValueTypeEnum != nullptr
        ? ValueTypeEnum->GetNameStringByValue(static_cast<int64>(ValueType))
        : TEXT("Variable");

    FString CandidateName = FString::Printf(TEXT("New%s"), *TypeName);
    int32 Suffix = 1;
    while (ExistingNames.Contains(FName(*CandidateName)))
    {
        CandidateName = FString::Printf(TEXT("New%s%d"), *TypeName, Suffix++);
    }

    if (TSharedPtr<IPropertyHandle> NameHandle = ElementHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, Name)))
    {
        NameHandle->SetValue(FName(*CandidateName));
    }

    if (TSharedPtr<IPropertyHandle> DefaultValueHandle = ElementHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, DefaultValue)))
    {
        if (TSharedPtr<IPropertyHandle> TypeHandle = DefaultValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowValue, Type)))
        {
            TypeHandle->SetValue(static_cast<uint8>(ValueType));
        }
    }

    if (PropertyUtilities.IsValid())
    {
        PropertyUtilities->ForceRefresh();
    }
}

#undef LOCTEXT_NAMESPACE
