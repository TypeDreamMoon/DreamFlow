#include "Customizations/DreamFlowVariablesEditorDataDetails.h"

#include "DreamFlowVariablesEditorData.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DreamFlowVariablesEditorDataDetails"

namespace DreamFlowVariablesDetails
{
    static FText GetValueTypeLabel(const EDreamFlowValueType ValueType)
    {
        const UEnum* ValueTypeEnum = StaticEnum<EDreamFlowValueType>();
        return ValueTypeEnum != nullptr
            ? ValueTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(ValueType))
            : FText::FromString(TEXT("Value"));
    }

    static FLinearColor GetValueTypeColor(const EDreamFlowValueType ValueType)
    {
        switch (ValueType)
        {
        case EDreamFlowValueType::Bool:
            return FLinearColor(0.33f, 0.75f, 0.47f, 1.0f);
        case EDreamFlowValueType::Int:
            return FLinearColor(0.25f, 0.61f, 0.92f, 1.0f);
        case EDreamFlowValueType::Float:
            return FLinearColor(0.34f, 0.73f, 0.93f, 1.0f);
        case EDreamFlowValueType::Name:
            return FLinearColor(0.84f, 0.60f, 0.26f, 1.0f);
        case EDreamFlowValueType::String:
            return FLinearColor(0.77f, 0.47f, 0.29f, 1.0f);
        case EDreamFlowValueType::Text:
            return FLinearColor(0.66f, 0.53f, 0.93f, 1.0f);
        case EDreamFlowValueType::GameplayTag:
            return FLinearColor(0.90f, 0.36f, 0.55f, 1.0f);
        case EDreamFlowValueType::Object:
            return FLinearColor(0.77f, 0.56f, 0.26f, 1.0f);
        default:
            return FLinearColor::White;
        }
    }
}

TSharedRef<IDetailCustomization> FDreamFlowVariablesEditorDataDetails::MakeInstance()
{
    return MakeShared<FDreamFlowVariablesEditorDataDetails>();
}

void FDreamFlowVariablesEditorDataDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    VariablesHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowVariablesEditorData, Variables));
    PropertyUtilities = DetailBuilder.GetPropertyUtilities();
    DetailsView = DetailBuilder.GetDetailsViewSharedPtr();

    TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
    DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
    for (const TWeakObjectPtr<UObject>& Object : CustomizedObjects)
    {
        if (UDreamFlowVariablesEditorData* EditorData = Cast<UDreamFlowVariablesEditorData>(Object.Get()))
        {
            VariablesEditorData = EditorData;
            break;
        }
    }

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
        ],
        true);

    VariablesCategory.AddCustomRow(LOCTEXT("VariablesSearch", "Flow Variables"))
    .WholeRowContent()
    [
        BuildVariablesPanel()
    ];
}

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildVariablesPanel()
{
    TSharedPtr<IPropertyHandleArray> VariablesArrayHandle = GetVariablesArrayHandle();
    uint32 VariableCount = 0;
    if (VariablesArrayHandle.IsValid())
    {
        VariablesArrayHandle->GetNumElements(VariableCount);
    }

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(0.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor(EStyleColor::Header))
                .Padding(FMargin(14.0f, 12.0f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("VariablesPanelTitle", "Flow Variables"))
                            .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(FText::Format(
                                LOCTEXT("VariablesPanelSubtitle", "{0} environment variables available to DreamFlow bindings and runtime execution."),
                                FText::AsNumber(VariableCount)))
                            .TextStyle(FAppStyle::Get(), "SmallText")
                            .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SComboButton)
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .OnGetMenuContent(this, &FDreamFlowVariablesEditorDataDetails::BuildAddVariableMenu)
                        .ButtonContent()
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            [
                                SNew(SImage)
                                .Image(FAppStyle::Get().GetBrush("Icons.Plus"))
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("AddVariableButton", "Add Variable"))
                                .Font(IDetailLayoutBuilder::GetDetailFont())
                            ]
                        ]
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))
            [
                SNew(SSplitter)
                .PhysicalSplitterHandleSize(2.0f)
                + SSplitter::Slot()
                .Value(0.38f)
                [
                    BuildVariablesSidebar()
                ]
                + SSplitter::Slot()
                .Value(0.62f)
                [
                    BuildSelectedVariableEditor()
                ]
            ]
        ];
}

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildVariablesSidebar() const
{
    const TArray<int32> FilteredIndices = GetFilteredVariableIndices();

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(0.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
                .Padding(FMargin(10.0f, 8.0f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SSearchBox)
                        .HintText(LOCTEXT("VariablesSearchHint", "Search variables"))
                        .InitialText(FText::FromString(GetStoredSearchText()))
                        .OnTextChanged(const_cast<FDreamFlowVariablesEditorDataDetails*>(this), &FDreamFlowVariablesEditorDataDetails::HandleSearchTextChanged)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                    [
                        SAssignNew(VariablesListCountTextBlock, STextBlock)
                        .Text(this, &FDreamFlowVariablesEditorDataDetails::GetVariablesListCountText)
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .VAlign(VAlign_Fill)
            [
                SAssignNew(VariablesListContainer, SBox)
                [
                    BuildVariablesList()
                ]
            ]
        ];
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
    AddTypeEntry(EDreamFlowValueType::Object, TEXT("Object"), TEXT("Add an object reference flow variable."));

    return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildVariablesList() const
{
    TSharedPtr<IPropertyHandleArray> VariablesArrayHandle = GetVariablesArrayHandle();
    uint32 VariableCount = 0;
    if (VariablesArrayHandle.IsValid())
    {
        VariablesArrayHandle->GetNumElements(VariableCount);
    }

    const TArray<int32> FilteredIndices = GetFilteredVariableIndices();
    if (VariableCount == 0)
    {
        return BuildEmptyState(
            LOCTEXT("NoVariablesTitle", "No Variables Yet"),
            LOCTEXT("NoVariablesDescription", "Add a variable from the header above to expose runtime data to bindings and flow logic."));
    }

    if (FilteredIndices.Num() == 0)
    {
        return BuildEmptyState(
            LOCTEXT("NoSearchMatchesTitle", "No Matching Variables"),
            LOCTEXT("NoSearchMatchesDescription", "Try another name, type, description, or default value search term."));
    }

    TSharedRef<SVerticalBox> CardsBox = SNew(SVerticalBox);
    for (int32 EntryIndex = 0; EntryIndex < FilteredIndices.Num(); ++EntryIndex)
    {
        CardsBox->AddSlot()
            .AutoHeight()
            .Padding(FMargin(8.0f, EntryIndex == 0 ? 8.0f : 4.0f, 8.0f, EntryIndex == FilteredIndices.Num() - 1 ? 8.0f : 0.0f))
            [
                BuildVariableCard(FilteredIndices[EntryIndex])
            ];
    }

    return SNew(SScrollBox)
        + SScrollBox::Slot()
        [
            CardsBox
        ];
}

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildVariableCard(int32 VariableIndex) const
{
    const UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData();
    if (EditorData == nullptr || !EditorData->Variables.IsValidIndex(VariableIndex))
    {
        return SNullWidget::NullWidget;
    }

    const FDreamFlowVariableDefinition& VariableDefinition = EditorData->Variables[VariableIndex];
    const bool bIsSelected = GetSelectedVariableIndex() == VariableIndex;
    const FLinearColor AccentColor = DreamFlowVariablesDetails::GetValueTypeColor(VariableDefinition.DefaultValue.Type);

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .BorderBackgroundColor(bIsSelected ? FSlateColor(EStyleColor::Select) : FSlateColor(EStyleColor::InputOutline))
        .Padding(1.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(AccentColor)
                .Padding(FMargin(2.5f, 0.0f))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
                .ContentPadding(0.0f)
                .OnClicked_Lambda([this, VariableIndex]()
                {
                    SelectVariable(VariableIndex);
                    return FReply::Handled();
                })
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(bIsSelected ? FSlateColor(EStyleColor::Header) : FSlateColor(EStyleColor::Panel))
                    .Padding(FMargin(10.0f, 8.0f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SNew(STextBlock)
                                    .Text(GetVariableDisplayName(VariableIndex))
                                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(SBorder)
                                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                                    .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
                                    .Padding(FMargin(6.0f, 2.0f))
                                    [
                                        SNew(STextBlock)
                                        .Text(DreamFlowVariablesDetails::GetValueTypeLabel(VariableDefinition.DefaultValue.Type))
                                        .TextStyle(FAppStyle::Get(), "SmallText")
                                    ]
                                ]
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(GetVariableSummary(VariableIndex))
                                .TextStyle(FAppStyle::Get(), "SmallText")
                                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                                .WrapTextAt(260.0f)
                            ]
                        ]
                    ]
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(FMargin(6.0f, 0.0f, 0.0f, 0.0f))
            .VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SButton)
                    .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                    .ToolTipText(LOCTEXT("DuplicateVariableTooltip", "Duplicate this variable."))
                    .OnClicked_Lambda([this, VariableIndex]()
                    {
                        DuplicateVariable(VariableIndex);
                        return FReply::Handled();
                    })
                    [
                        SNew(SImage)
                        .Image(FAppStyle::Get().GetBrush("Icons.Duplicate"))
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                    .ToolTipText(LOCTEXT("RemoveVariableTooltip", "Remove this variable."))
                    .OnClicked_Lambda([this, VariableIndex]()
                    {
                        RemoveVariable(VariableIndex);
                        return FReply::Handled();
                    })
                    [
                        SNew(SImage)
                        .Image(FAppStyle::Get().GetBrush("Icons.Delete"))
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildSelectedVariableEditor() const
{
    const int32 SelectedIndex = GetSelectedVariableIndex();
    TSharedPtr<IPropertyHandle> VariableHandle = GetVariableHandle(SelectedIndex);
    if (!VariableHandle.IsValid())
    {
        return BuildEmptyState(
            LOCTEXT("NoVariableSelectedTitle", "Select A Variable"),
            LOCTEXT("NoVariableSelectedDescription", "Pick a variable on the left to edit its name, description, and default value."));
    }

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(0.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor(EStyleColor::Header))
                .Padding(FMargin(12.0f, 10.0f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("SelectedVariableTitle", "Variable Details"))
                            .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(GetVariableDisplayName(SelectedIndex))
                            .TextStyle(FAppStyle::Get(), "SmallText")
                            .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(SButton)
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .OnClicked_Lambda([this, SelectedIndex]()
                        {
                            DuplicateVariable(SelectedIndex);
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("DuplicateVariableButton", "Duplicate"))
                            .TextStyle(FAppStyle::Get(), "SmallText")
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SButton)
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .OnClicked_Lambda([this, SelectedIndex]()
                        {
                            RemoveVariable(SelectedIndex);
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("RemoveVariableButton", "Remove"))
                            .TextStyle(FAppStyle::Get(), "SmallText")
                        ]
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .VAlign(VAlign_Fill)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
                    [
                        BuildInspectorSummary(SelectedIndex)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
                    [
                        BuildPropertyEditorRow(VariableHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, Name)))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(10.0f, 8.0f, 10.0f, 0.0f))
                    [
                        BuildPropertyEditorRow(VariableHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, Description)))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(10.0f, 8.0f, 10.0f, 10.0f))
                    [
                        BuildPropertyEditorRow(
                            VariableHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, DefaultValue)),
                            true)
                    ]
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SNullWidget::NullWidget
                ]
            ]
        ];
}

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildInspectorSummary(int32 VariableIndex) const
{
    const UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData();
    if (EditorData == nullptr || !EditorData->Variables.IsValidIndex(VariableIndex))
    {
        return SNullWidget::NullWidget;
    }

    const FDreamFlowVariableDefinition& VariableDefinition = EditorData->Variables[VariableIndex];
    const FLinearColor AccentColor = DreamFlowVariablesDetails::GetValueTypeColor(VariableDefinition.DefaultValue.Type);

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .BorderBackgroundColor(FSlateColor(EStyleColor::Panel))
        .Padding(FMargin(12.0f, 10.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(AccentColor)
                    .Padding(FMargin(8.0f, 2.0f))
                    [
                        SNew(STextBlock)
                        .Text(DreamFlowVariablesDetails::GetValueTypeLabel(VariableDefinition.DefaultValue.Type))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FLinearColor::White)
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::Format(
                        LOCTEXT("InspectorDefaultValue", "Default: {0}"),
                        FText::FromString(VariableDefinition.DefaultValue.Describe())))
                    .TextStyle(FAppStyle::Get(), "SmallText")
                    .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 8.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(GetVariableSummary(VariableIndex))
                .TextStyle(FAppStyle::Get(), "SmallText")
                .WrapTextAt(520.0f)
            ]
        ];
}

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildPropertyEditorRow(const TSharedPtr<IPropertyHandle>& PropertyHandle, bool bUseCustomValueWidget) const
{
    if (!PropertyHandle.IsValid())
    {
        return SNullWidget::NullWidget;
    }

    const TSharedPtr<const IDetailsView> DetailsViewPinned = DetailsView.Pin();
    TSharedRef<SWidget> ValueWidget = bUseCustomValueWidget && DetailsViewPinned.IsValid()
        ? PropertyHandle->CreatePropertyValueWidgetWithCustomization(DetailsViewPinned.Get())
        : PropertyHandle->CreatePropertyValueWidget();

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .BorderBackgroundColor(FSlateColor(EStyleColor::Panel))
        .Padding(FMargin(12.0f, 10.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                PropertyHandle->CreatePropertyNameWidget()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 8.0f, 0.0f, 0.0f)
            [
                ValueWidget
            ]
        ];
}

TSharedRef<SWidget> FDreamFlowVariablesEditorDataDetails::BuildEmptyState(const FText& Title, const FText& Description) const
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
        .Padding(FMargin(18.0f, 20.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(Title)
                .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
                .Justification(ETextJustify::Center)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Center)
            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(Description)
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                .Justification(ETextJustify::Center)
                .WrapTextAt(360.0f)
            ]
        ];
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

    if (UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData())
    {
        EditorData->SelectedVariableIndex = AddResult.GetIndex();
    }

    if (PropertyUtilities.IsValid())
    {
        PropertyUtilities->ForceRefresh();
    }
}

void FDreamFlowVariablesEditorDataDetails::SelectVariable(int32 VariableIndex) const
{
    if (UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData())
    {
        EditorData->SelectedVariableIndex = VariableIndex;
    }

    if (PropertyUtilities.IsValid())
    {
        PropertyUtilities->ForceRefresh();
    }
}

void FDreamFlowVariablesEditorDataDetails::DuplicateVariable(int32 VariableIndex) const
{
    TSharedPtr<IPropertyHandleArray> VariablesArrayHandle = GetVariablesArrayHandle();
    if (!VariablesArrayHandle.IsValid() || VariablesArrayHandle->DuplicateItem(VariableIndex) != FPropertyAccess::Success)
    {
        return;
    }

    const int32 DuplicatedIndex = VariableIndex + 1;
    TSharedPtr<IPropertyHandle> DuplicatedHandle = VariablesArrayHandle->GetElement(DuplicatedIndex);
    if (DuplicatedHandle.IsValid())
    {
        TSharedPtr<IPropertyHandle> NameHandle = DuplicatedHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, Name));
        FName ExistingName = NAME_None;
        if (NameHandle.IsValid() && NameHandle->GetValue(ExistingName) == FPropertyAccess::Success)
        {
            FString CandidateName = ExistingName.IsNone()
                ? TEXT("CopiedVariable")
                : FString::Printf(TEXT("%s_Copy"), *ExistingName.ToString());

            TSet<FName> ExistingNames;
            uint32 ElementCount = 0;
            VariablesArrayHandle->GetNumElements(ElementCount);
            for (uint32 Index = 0; Index < ElementCount; ++Index)
            {
                if (static_cast<int32>(Index) == DuplicatedIndex)
                {
                    continue;
                }

                TSharedPtr<IPropertyHandle> ExistingElementHandle = VariablesArrayHandle->GetElement(Index);
                TSharedPtr<IPropertyHandle> ExistingNameHandle = ExistingElementHandle.IsValid()
                    ? ExistingElementHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDreamFlowVariableDefinition, Name))
                    : nullptr;

                FName ExistingElementName = NAME_None;
                if (ExistingNameHandle.IsValid() && ExistingNameHandle->GetValue(ExistingElementName) == FPropertyAccess::Success && !ExistingElementName.IsNone())
                {
                    ExistingNames.Add(ExistingElementName);
                }
            }

            FString UniqueName = CandidateName;
            int32 Suffix = 1;
            while (ExistingNames.Contains(FName(*UniqueName)))
            {
                UniqueName = FString::Printf(TEXT("%s%d"), *CandidateName, Suffix++);
            }

            NameHandle->SetValue(FName(*UniqueName));
        }
    }

    if (UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData())
    {
        EditorData->SelectedVariableIndex = DuplicatedIndex;
    }

    if (PropertyUtilities.IsValid())
    {
        PropertyUtilities->ForceRefresh();
    }
}

void FDreamFlowVariablesEditorDataDetails::RemoveVariable(int32 VariableIndex) const
{
    TSharedPtr<IPropertyHandleArray> VariablesArrayHandle = GetVariablesArrayHandle();
    if (!VariablesArrayHandle.IsValid() || VariablesArrayHandle->DeleteItem(VariableIndex) != FPropertyAccess::Success)
    {
        return;
    }

    if (UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData())
    {
        uint32 RemainingCount = 0;
        VariablesArrayHandle->GetNumElements(RemainingCount);
        if (RemainingCount == 0)
        {
            EditorData->SelectedVariableIndex = INDEX_NONE;
        }
        else
        {
            EditorData->SelectedVariableIndex = FMath::Min(VariableIndex, static_cast<int32>(RemainingCount) - 1);
        }
    }

    if (PropertyUtilities.IsValid())
    {
        PropertyUtilities->ForceRefresh();
    }
}

void FDreamFlowVariablesEditorDataDetails::HandleSearchTextChanged(const FText& InSearchText)
{
    if (UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData())
    {
        EditorData->SearchText = InSearchText.ToString();
    }

    RefreshFilteredVariableWidgets();
}

FText FDreamFlowVariablesEditorDataDetails::GetSearchText() const
{
    return FText::FromString(GetStoredSearchText());
}

FText FDreamFlowVariablesEditorDataDetails::GetVariablesListCountText() const
{
    const TArray<int32> FilteredIndices = GetFilteredVariableIndices();
    return FText::Format(
        GetStoredSearchText().IsEmpty()
            ? LOCTEXT("VariablesListCount", "{0} variables")
            : LOCTEXT("VariablesFilteredCount", "{0} shown"),
        FText::AsNumber(FilteredIndices.Num()));
}

void FDreamFlowVariablesEditorDataDetails::RefreshFilteredVariableWidgets() const
{
    if (VariablesListContainer.IsValid())
    {
        VariablesListContainer->SetContent(BuildVariablesList());
    }

    if (VariablesListCountTextBlock.IsValid())
    {
        VariablesListCountTextBlock->SetText(GetVariablesListCountText());
    }
}

FString FDreamFlowVariablesEditorDataDetails::GetStoredSearchText() const
{
    const UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData();
    return EditorData != nullptr ? EditorData->SearchText : FString();
}

int32 FDreamFlowVariablesEditorDataDetails::GetSelectedVariableIndex() const
{
    const UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData();
    if (EditorData == nullptr)
    {
        return INDEX_NONE;
    }

    if (EditorData->Variables.IsValidIndex(EditorData->SelectedVariableIndex))
    {
        return EditorData->SelectedVariableIndex;
    }

    return EditorData->Variables.Num() > 0 ? 0 : INDEX_NONE;
}

TArray<int32> FDreamFlowVariablesEditorDataDetails::GetFilteredVariableIndices() const
{
    TArray<int32> Result;

    const UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData();
    if (EditorData == nullptr)
    {
        return Result;
    }

    const FString TrimmedSearch = EditorData->SearchText.TrimStartAndEnd();
    for (int32 Index = 0; Index < EditorData->Variables.Num(); ++Index)
    {
        const FDreamFlowVariableDefinition& VariableDefinition = EditorData->Variables[Index];
        if (TrimmedSearch.IsEmpty())
        {
            Result.Add(Index);
            continue;
        }

        const FString SearchableText = FString::Printf(
            TEXT("%s %s %s %s"),
            *VariableDefinition.Name.ToString(),
            *DreamFlowVariablesDetails::GetValueTypeLabel(VariableDefinition.DefaultValue.Type).ToString(),
            *VariableDefinition.DefaultValue.Describe(),
            *VariableDefinition.Description.ToString());

        if (SearchableText.Contains(TrimmedSearch, ESearchCase::IgnoreCase))
        {
            Result.Add(Index);
        }
    }

    return Result;
}

TSharedPtr<IPropertyHandleArray> FDreamFlowVariablesEditorDataDetails::GetVariablesArrayHandle() const
{
    return VariablesHandle.IsValid() ? VariablesHandle->AsArray() : nullptr;
}

TSharedPtr<IPropertyHandle> FDreamFlowVariablesEditorDataDetails::GetVariableHandle(int32 VariableIndex) const
{
    TSharedPtr<IPropertyHandleArray> VariablesArrayHandle = GetVariablesArrayHandle();
    if (!VariablesArrayHandle.IsValid() || VariableIndex == INDEX_NONE)
    {
        return nullptr;
    }

    return VariablesArrayHandle->GetElement(VariableIndex);
}

UDreamFlowVariablesEditorData* FDreamFlowVariablesEditorDataDetails::GetVariablesEditorData() const
{
    return VariablesEditorData.Get();
}

FText FDreamFlowVariablesEditorDataDetails::GetVariableDisplayName(int32 VariableIndex) const
{
    const UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData();
    if (EditorData == nullptr || !EditorData->Variables.IsValidIndex(VariableIndex))
    {
        return FText::FromString(TEXT("Variable"));
    }

    const FName Name = EditorData->Variables[VariableIndex].Name;
    return Name.IsNone() ? LOCTEXT("UnnamedVariable", "Unnamed Variable") : FText::FromName(Name);
}

FText FDreamFlowVariablesEditorDataDetails::GetVariableSummary(int32 VariableIndex) const
{
    const UDreamFlowVariablesEditorData* EditorData = GetVariablesEditorData();
    if (EditorData == nullptr || !EditorData->Variables.IsValidIndex(VariableIndex))
    {
        return FText::GetEmpty();
    }

    const FDreamFlowVariableDefinition& VariableDefinition = EditorData->Variables[VariableIndex];
    const FText DefaultValueLabel = FText::FromString(VariableDefinition.DefaultValue.Describe());

    if (!VariableDefinition.Description.IsEmpty())
    {
        return FText::Format(
            LOCTEXT("VariableSummaryWithDescription", "{0}  •  Default: {1}\n{2}"),
            DreamFlowVariablesDetails::GetValueTypeLabel(VariableDefinition.DefaultValue.Type),
            DefaultValueLabel,
            VariableDefinition.Description);
    }

    return FText::Format(
        LOCTEXT("VariableSummary", "{0}  •  Default: {1}"),
        DreamFlowVariablesDetails::GetValueTypeLabel(VariableDefinition.DefaultValue.Type),
        DefaultValueLabel);
}

#undef LOCTEXT_NAMESPACE
