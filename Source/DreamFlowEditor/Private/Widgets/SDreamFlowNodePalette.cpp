#include "Widgets/SDreamFlowNodePalette.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowNode.h"
#include "Algo/Sort.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace DreamFlowNodePalette
{
    static FLinearColor MakeMutedBodyColor(const FLinearColor& Tint)
    {
        return FLinearColor(
            FMath::Clamp(Tint.R * 0.16f + 0.08f, 0.0f, 1.0f),
            FMath::Clamp(Tint.G * 0.16f + 0.09f, 0.0f, 1.0f),
            FMath::Clamp(Tint.B * 0.16f + 0.11f, 0.0f, 1.0f),
            1.0f);
    }
}

void SDreamFlowNodePalette::Construct(const FArguments& InArgs)
{
    FlowAsset = InArgs._FlowAsset;
    OnNodeClassPicked = InArgs._OnNodeClassPicked;

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(12.0f)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Node Palette")))
                .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 10.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Drag your design language from a reusable set of node types.")))
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FLinearColor(0.77f, 0.81f, 0.86f, 0.92f))
                .WrapTextAt(240.0f)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SSearchBox)
                .HintText(FText::FromString(TEXT("Search nodes")))
                .OnTextChanged(this, &SDreamFlowNodePalette::HandleSearchTextChanged)
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0.0f, 10.0f, 0.0f, 0.0f)
            [
                SAssignNew(EntryContainer, SScrollBox)
            ]
        ]
    ];

    RefreshNodeClasses();
}

void SDreamFlowNodePalette::RefreshNodeClasses()
{
    AllEntries.Reset();

    for (const TSubclassOf<UDreamFlowNode>& NodeClass : FDreamFlowEditorUtils::GetLoadedCreatableNodeClasses(FlowAsset.Get()))
    {
        const UClass* LoadedClass = *NodeClass;
        const UDreamFlowNode* DefaultNode = LoadedClass ? Cast<UDreamFlowNode>(LoadedClass->GetDefaultObject()) : nullptr;
        if (DefaultNode == nullptr)
        {
            continue;
        }

        FPaletteEntry& Entry = AllEntries.AddDefaulted_GetRef();
        Entry.NodeClass = NodeClass;
        Entry.DisplayName = DefaultNode->GetNodeDisplayName();
        Entry.Category = DefaultNode->GetNodeCategory();
        Entry.AccentLabel = DefaultNode->GetNodeAccentLabel();
        Entry.Tint = DefaultNode->GetNodeTint();
        Entry.SearchText = FString::Printf(
            TEXT("%s %s %s"),
            *Entry.DisplayName.ToString(),
            *Entry.Category.ToString(),
            *LoadedClass->GetName());
    }

    Algo::Sort(AllEntries, [](const FPaletteEntry& A, const FPaletteEntry& B)
    {
        const FString CategoryA = A.Category.ToString();
        const FString CategoryB = B.Category.ToString();
        if (CategoryA != CategoryB)
        {
            return CategoryA < CategoryB;
        }

        return A.DisplayName.ToString() < B.DisplayName.ToString();
    });

    RebuildPalette();
}

void SDreamFlowNodePalette::RebuildPalette()
{
    if (!EntryContainer.IsValid())
    {
        return;
    }

    EntryContainer->ClearChildren();

    FString CurrentCategory;
    int32 MatchCount = 0;
    const FString SearchLower = SearchText.ToLower();

    for (const FPaletteEntry& Entry : AllEntries)
    {
        if (!SearchLower.IsEmpty() && !Entry.SearchText.ToLower().Contains(SearchLower))
        {
            continue;
        }

        const FString EntryCategory = Entry.Category.ToString();
        if (CurrentCategory != EntryCategory)
        {
            CurrentCategory = EntryCategory;
            EntryContainer->AddSlot()
            [
                SNew(STextBlock)
                .Text(Entry.Category)
                .TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
                .ColorAndOpacity(FLinearColor(0.92f, 0.95f, 0.98f, 1.0f))
            ];
        }

        EntryContainer->AddSlot()
        .Padding(0.0f, 6.0f, 0.0f, 0.0f)
        [
            BuildEntryWidget(Entry)
        ];

        ++MatchCount;
    }

    if (MatchCount == 0)
    {
        EntryContainer->AddSlot()
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("No node classes match the current filter.")))
            .TextStyle(FAppStyle::Get(), "SmallText")
            .ColorAndOpacity(FLinearColor(0.76f, 0.80f, 0.85f, 0.85f))
        ];
    }
}

void SDreamFlowNodePalette::HandleSearchTextChanged(const FText& InSearchText)
{
    SearchText = InSearchText.ToString();
    RebuildPalette();
}

FReply SDreamFlowNodePalette::HandleEntryClicked(TSubclassOf<UDreamFlowNode> NodeClass) const
{
    if (OnNodeClassPicked.IsBound())
    {
        OnNodeClassPicked.Execute(NodeClass);
    }

    return FReply::Handled();
}

TSharedRef<SWidget> SDreamFlowNodePalette::BuildEntryWidget(const FPaletteEntry& Entry) const
{
    const FLinearColor BodyColor = DreamFlowNodePalette::MakeMutedBodyColor(Entry.Tint);

    return SNew(SButton)
        .ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
        .OnClicked(this, &SDreamFlowNodePalette::HandleEntryClicked, Entry.NodeClass)
        .ContentPadding(0.0f)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(Entry.Tint)
            .Padding(1.0f)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(BodyColor)
                .Padding(FMargin(10.0f, 9.0f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(Entry.DisplayName)
                        .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                        .ColorAndOpacity(FLinearColor::White)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 3.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text(!Entry.AccentLabel.IsEmpty() ? Entry.AccentLabel : Entry.Category)
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.92f, 0.94f))
                    ]
                ]
            ]
        ];
}
