#include "Widgets/SDreamFlowNodePalette.h"

#include "DragDrop/DreamFlowNodeClassDragDropOp.h"
#include "DreamFlowAsset.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowNode.h"
#include "Algo/Sort.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace DreamFlowNodePalette
{
    static FLinearColor MakeAccentColor(const FLinearColor& Tint)
    {
        return Tint.CopyWithNewOpacity(0.92f);
    }

    static const UDreamFlowNode* GetDefaultNode(const TSubclassOf<UDreamFlowNode>& NodeClass)
    {
        const UClass* LoadedClass = *NodeClass;
        return LoadedClass != nullptr ? Cast<UDreamFlowNode>(LoadedClass->GetDefaultObject()) : nullptr;
    }

    static const FSlateBrush* ResolveNodeIconBrush(const UDreamFlowNode* DefaultNode, const UClass* LoadedClass, TSharedPtr<FSlateBrush>& InOutCustomBrush)
    {
        InOutCustomBrush.Reset();

        if (DefaultNode != nullptr)
        {
            if (UTexture2D* NodeIconTexture = DefaultNode->GetNodeIconTexture())
            {
                InOutCustomBrush = MakeShared<FSlateBrush>();
                InOutCustomBrush->SetResourceObject(NodeIconTexture);
                InOutCustomBrush->ImageSize = FVector2D(16.0f, 16.0f);
                InOutCustomBrush->DrawAs = ESlateBrushDrawType::Image;
                return InOutCustomBrush.Get();
            }

            const FName StyleName = DefaultNode->GetNodeIconStyleName();
            if (!StyleName.IsNone())
            {
                return FAppStyle::GetBrush(StyleName);
            }
        }

        return LoadedClass != nullptr ? FSlateIconFinder::FindIconBrushForClass(LoadedClass) : nullptr;
    }

    class SPaletteEntryWidget : public SCompoundWidget
    {
    public:
        DECLARE_DELEGATE_RetVal_OneParam(FReply, FOnEntryClicked, TSubclassOf<UDreamFlowNode>);
        DECLARE_DELEGATE_RetVal_ThreeParams(FReply, FOnEntryDragDetected, const FGeometry&, const FPointerEvent&, TSubclassOf<UDreamFlowNode>);

        SLATE_BEGIN_ARGS(SPaletteEntryWidget) {}
            SLATE_ARGUMENT(TSubclassOf<UDreamFlowNode>, NodeClass)
            SLATE_EVENT(FOnEntryClicked, OnEntryClicked)
            SLATE_EVENT(FOnEntryDragDetected, OnEntryDragDetected)
            SLATE_DEFAULT_SLOT(FArguments, Content)
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs)
        {
            NodeClass = InArgs._NodeClass;
            OnEntryClicked = InArgs._OnEntryClicked;
            OnEntryDragDetected = InArgs._OnEntryDragDetected;
            bDragTriggered = false;

            ChildSlot
            [
                InArgs._Content.Widget
            ];
        }

        virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
        {
            if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
            {
                bDragTriggered = false;
                return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
            }

            return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
        }

        virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
        {
            (void)MyGeometry;

            if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
            {
                const bool bShouldInvokeClick = !bDragTriggered;
                bDragTriggered = false;

                if (bShouldInvokeClick && OnEntryClicked.IsBound())
                {
                    return OnEntryClicked.Execute(NodeClass);
                }

                return FReply::Handled();
            }

            return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
        }

        virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
        {
            bDragTriggered = true;

            return OnEntryDragDetected.IsBound()
                ? OnEntryDragDetected.Execute(MyGeometry, MouseEvent, NodeClass)
                : SCompoundWidget::OnDragDetected(MyGeometry, MouseEvent);
        }

    private:
        TSubclassOf<UDreamFlowNode> NodeClass;
        FOnEntryClicked OnEntryClicked;
        FOnEntryDragDetected OnEntryDragDetected;
        bool bDragTriggered = false;
    };
}

void SDreamFlowNodePalette::Construct(const FArguments& InArgs)
{
    FlowAsset = InArgs._FlowAsset;
    OnNodeClassPicked = InArgs._OnNodeClassPicked;
    OnNodeClassDropped = InArgs._OnNodeClassDropped;

    ChildSlot
    [
        SNew(SBorder)
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
                .Padding(FMargin(12.0f, 12.0f))
                [
                    SNew(SVerticalBox)

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Node Palette")))
                        .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
                        .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]()
                        {
                            return FText::Format(
                                FText::FromString(TEXT("{0} node types available for {1}.")),
                                FText::AsNumber(AllEntries.Num()),
                                FlowAsset.IsValid()
                                    ? FlowAsset->GetClass()->GetDisplayNameText()
                                    : FText::FromString(TEXT("this flow")));
                        })
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                        .WrapTextAt(240.0f)
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]()
                        {
                            return FlowAsset.IsValid()
                                ? FText::Format(
                                    FText::FromString(TEXT("Editing {0}")),
                                    FText::FromString(FlowAsset->GetName()))
                                : FText::FromString(TEXT("Pick a node to add it to the graph"));
                        })
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::Primary))
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 10.0f, 12.0f, 0.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .Padding(2.0f)
                [
                    SNew(SSearchBox)
                    .HintText(FText::FromString(TEXT("Search node types, categories, or class names")))
                    .OnTextChanged(this, &SDreamFlowNodePalette::HandleSearchTextChanged)
                ]
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(12.0f, 10.0f, 12.0f, 12.0f)
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
        const UDreamFlowNode* DefaultNode = DreamFlowNodePalette::GetDefaultNode(NodeClass);
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
        Entry.IconBrush = DreamFlowNodePalette::ResolveNodeIconBrush(DefaultNode, LoadedClass, Entry.CustomIconBrush);
        Entry.SearchText = FString::Printf(
            TEXT("%s %s %s %s %s"),
            *Entry.DisplayName.ToString(),
            *Entry.Category.ToString(),
            *Entry.AccentLabel.ToString(),
            *DefaultNode->Description.ToString(),
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
            .Padding(0.0f, MatchCount > 0 ? 14.0f : 0.0f, 0.0f, 2.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor(EStyleColor::Header))
                .Padding(FMargin(10.0f, 6.0f))
                [
                    SNew(STextBlock)
                    .Text(Entry.Category)
                    .TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
                    .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                ]
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
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
            .Padding(FMargin(12.0f, 10.0f))
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("No node classes match the current filter.")))
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                .WrapTextAt(240.0f)
            ]
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

FReply SDreamFlowNodePalette::HandleEntryDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TSubclassOf<UDreamFlowNode> NodeClass) const
{
    (void)MyGeometry;

    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || NodeClass == nullptr)
    {
        return FReply::Unhandled();
    }

    const UDreamFlowNode* DefaultNode = DreamFlowNodePalette::GetDefaultNode(NodeClass);
    if (DefaultNode == nullptr)
    {
        return FReply::Unhandled();
    }

    return FReply::Handled().BeginDragDrop(FDreamFlowNodeClassDragDropOp::New(
        NodeClass,
        DefaultNode->GetNodeDisplayName(),
        DefaultNode->GetNodeCategory(),
        DefaultNode->GetNodeTint(),
        FDreamFlowNodeClassDragDropOp::FOnNodeClassDropped::CreateLambda([OnNodeClassDropped = OnNodeClassDropped](TSubclassOf<UDreamFlowNode> DraggedNodeClass, const FVector2f& GraphPosition)
        {
            if (OnNodeClassDropped.IsBound())
            {
                OnNodeClassDropped.Execute(DraggedNodeClass, GraphPosition);
            }
        })));
}

TSharedRef<SWidget> SDreamFlowNodePalette::BuildEntryWidget(const FPaletteEntry& Entry) const
{
    const UDreamFlowNode* DefaultNode = DreamFlowNodePalette::GetDefaultNode(Entry.NodeClass);
    const FText DescriptionText = DefaultNode != nullptr ? DefaultNode->Description : FText::GetEmpty();
    const FText ClassLabel = Entry.NodeClass != nullptr
        ? Entry.NodeClass->GetDisplayNameText()
        : FText::FromString(TEXT("Node"));

    return SNew(DreamFlowNodePalette::SPaletteEntryWidget)
        .NodeClass(Entry.NodeClass)
        .OnEntryClicked(DreamFlowNodePalette::SPaletteEntryWidget::FOnEntryClicked::CreateSP(this, &SDreamFlowNodePalette::HandleEntryClicked))
        .OnEntryDragDetected(DreamFlowNodePalette::SPaletteEntryWidget::FOnEntryDragDetected::CreateSP(this, &SDreamFlowNodePalette::HandleEntryDragDetected))
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .BorderBackgroundColor(FSlateColor(EStyleColor::InputOutline))
            .Padding(1.0f)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(DreamFlowNodePalette::MakeAccentColor(Entry.Tint))
                    .Padding(FMargin(3.0f, 0.0f))
                ]

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
                    .Padding(FMargin(11.0f, 10.0f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SHorizontalBox)

                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Top)
                            .Padding(0.0f, 1.0f, 8.0f, 0.0f)
                            [
                                SNew(SImage)
                                .Visibility(Entry.IconBrush != nullptr ? EVisibility::Visible : EVisibility::Collapsed)
                                .Image(Entry.IconBrush)
                                .DesiredSizeOverride(FVector2D(16.0f, 16.0f))
                                .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                            ]

                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            [
                                SNew(STextBlock)
                                .Text(Entry.DisplayName)
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                                .WrapTextAt(190.0f)
                            ]

                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(FSlateColor(EStyleColor::Panel))
                                .Padding(FMargin(6.0f, 2.0f))
                                [
                                    SNew(STextBlock)
                                    .Text(ClassLabel)
                                    .TextStyle(FAppStyle::Get(), "SmallText")
                                    .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                                ]
                            ]
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(DescriptionText.IsEmpty() ? FText::FromString(TEXT("Reusable DreamFlow node. Click to add or drag into the graph.")) : DescriptionText)
                            .TextStyle(FAppStyle::Get(), "SmallText")
                            .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                            .WrapTextAt(232.0f)
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(FSlateColor(EStyleColor::Header))
                                .Padding(FMargin(6.0f, 2.0f))
                                [
                                    SNew(STextBlock)
                                    .Text(Entry.Category)
                                    .TextStyle(FAppStyle::Get(), "SmallText")
                                    .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(SBorder)
                                .Visibility(!Entry.AccentLabel.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed)
                                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(FSlateColor(EStyleColor::Input))
                                .Padding(FMargin(6.0f, 2.0f))
                                [
                                    SNew(STextBlock)
                                    .Text(Entry.AccentLabel)
                                    .TextStyle(FAppStyle::Get(), "SmallText")
                                    .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ];
}
