#include "SGraphNode_DreamFlow.h"

#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEditorToolkit.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowNodeDisplayTypes.h"
#include "DreamFlowNode.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "Styling/StyleColors.h"
#include "Styling/AppStyle.h"
#include "SGraphPin.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SGraphNode_DreamFlow::Construct(const FArguments& InArgs, UDreamFlowEdGraphNode* InNode)
{
    (void)InArgs;
    GraphNode = InNode;
    UpdateGraphNode();
}

void SGraphNode_DreamFlow::UpdateGraphNode()
{
    InputPins.Empty();
    OutputPins.Empty();
    LeftNodeBox.Reset();
    RightNodeBox.Reset();
    PreviewImageBrushes.Reset();

    this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

    TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

    GetOrAddSlot(ENodeZone::Center)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Center)
        [
            SNew(SBorder)
            .BorderImage(this, &SGraphNode_DreamFlow::GetOuterNodeBrush)
            .BorderBackgroundColor(this, &SGraphNode_DreamFlow::GetNodeOutlineColor)
            .Padding(1.0f)
            [
                SNew(SBorder)
                .BorderImage(this, &SGraphNode_DreamFlow::GetInnerNodeBrush)
                .BorderBackgroundColor(this, &SGraphNode_DreamFlow::GetNodeSurfaceColor)
                .Padding(FMargin(0.0f))
                [
                    SNew(SVerticalBox)

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(12.0f, 12.0f, 12.0f, 0.0f))
                    [
                        SNew(SBox)
                        .HeightOverride(4.0f)
                        [
                            SNew(SBorder)
                            .BorderImage(this, &SGraphNode_DreamFlow::GetMarkerBrush)
                            .BorderBackgroundColor(this, &SGraphNode_DreamFlow::GetMarkerColor)
                            .Padding(0.0f)
                        ]
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(12.0f, 10.0f, 12.0f, 0.0f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
                            .TextStyle(FAppStyle::Get(), "Graph.Node.NodeTitle")
                            .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetTitleTextColor)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(this, &SGraphNode_DreamFlow::GetNodeClassLabel)
                            .TextStyle(FAppStyle::Get(), "SmallText")
                            .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetClassTextColor)
                        ]
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(12.0f, 8.0f, 12.0f, 0.0f))
                    [
                        SNew(SSeparator)
                        .Orientation(Orient_Horizontal)
                        .Thickness(1.0f)
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(10.0f, 8.0f, 10.0f, 10.0f))
                    [
                        SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SAssignNew(LeftNodeBox, SVerticalBox)
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(FMargin(10.0f, 0.0f))
                        .VAlign(VAlign_Center)
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
                                    .BorderImage(this, &SGraphNode_DreamFlow::GetBadgeBrush)
                                    .BorderBackgroundColor(FStyleColors::Recessed)
                                    .Padding(FMargin(8.0f, 3.0f))
                                    [
                                        SNew(STextBlock)
                                        .Text(this, &SGraphNode_DreamFlow::GetNodeCategoryLabel)
                                        .TextStyle(FAppStyle::Get(), "SmallText")
                                        .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetBadgeTextColor)
                                    ]
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                                [
                                    SNew(SBorder)
                                    .Visibility(this, &SGraphNode_DreamFlow::GetAccentVisibility)
                                    .BorderImage(this, &SGraphNode_DreamFlow::GetBadgeBrush)
                                    .BorderBackgroundColor(FStyleColors::Input)
                                    .Padding(FMargin(8.0f, 3.0f))
                                    [
                                        SNew(STextBlock)
                                        .Text(this, &SGraphNode_DreamFlow::GetNodeAccentLabel)
                                        .TextStyle(FAppStyle::Get(), "SmallText")
                                        .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetBadgeTextColor)
                                    ]
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                                [
                                    SNew(SBorder)
                                    .Visibility(this, &SGraphNode_DreamFlow::GetBreakpointVisibility)
                                    .BorderImage(this, &SGraphNode_DreamFlow::GetBadgeBrush)
                                    .BorderBackgroundColor(FStyleColors::Error)
                                    .Padding(FMargin(8.0f, 3.0f))
                                    [
                                        SNew(STextBlock)
                                        .Text(FText::FromString(TEXT("Breakpoint")))
                                        .TextStyle(FAppStyle::Get(), "SmallText")
                                        .ColorAndOpacity(FSlateColor(EStyleColor::White))
                                    ]
                                ]
                            ]

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(this, &SGraphNode_DreamFlow::GetNodeDescription)
                                .TextStyle(FAppStyle::Get(), "NormalText")
                                .WrapTextAt(260.0f)
                                .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetTitleTextColor)
                                .Visibility(this, &SGraphNode_DreamFlow::GetDescriptionVisibility)
                            ]

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                            [
                                BuildPreviewArea()
                            ]

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(this, &SGraphNode_DreamFlow::GetNodeConnectionLabel)
                                .TextStyle(FAppStyle::Get(), "SmallText")
                                .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetMutedTextColor)
                            ]
                        ]

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SAssignNew(RightNodeBox, SVerticalBox)
                        ]
                    ]
                ]
            ]
        ];

    CreatePinWidgets();
}

void SGraphNode_DreamFlow::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
    PinToAdd->SetOwner(SharedThis(this));

    if (PinToAdd->GetDirection() == EGPD_Input)
    {
        LeftNodeBox->AddSlot()
            .AutoHeight()
            .VAlign(VAlign_Center)
            .Padding(FMargin(0.0f, 3.0f, 10.0f, 3.0f))
            [
                PinToAdd
            ];
        InputPins.Add(PinToAdd);
    }
    else
    {
        RightNodeBox->AddSlot()
            .AutoHeight()
            .VAlign(VAlign_Center)
            .Padding(FMargin(10.0f, 3.0f, 0.0f, 3.0f))
            [
                PinToAdd
            ];
        OutputPins.Add(PinToAdd);
    }
}

void SGraphNode_DreamFlow::MoveTo(const FVector2f& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
{
    SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);

    if (const UDreamFlowEdGraphNode* FlowNode = GetFlowNode())
    {
        FDreamFlowEditorUtils::SynchronizeAssetFromGraph(FDreamFlowEditorUtils::GetFlowAssetFromGraph(FlowNode->GetGraph()));
    }
}

FReply SGraphNode_DreamFlow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    const FReply Reply = SGraphNode::OnMouseButtonDown(MyGeometry, MouseEvent);

    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (const UDreamFlowEdGraphNode* FlowNode = GetFlowNode())
        {
            FDreamFlowEditorToolkit::OpenNodeEditorForGraph(FlowNode->GetGraph(), FlowNode->GetRuntimeNode());
        }
    }

    return Reply;
}

TSharedRef<SWidget> SGraphNode_DreamFlow::BuildPreviewArea()
{
    TSharedRef<SVerticalBox> PreviewBox = SNew(SVerticalBox);

    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    const TArray<FDreamFlowNodeDisplayItem> Items = RuntimeNode ? RuntimeNode->GetNodeDisplayItems() : TArray<FDreamFlowNodeDisplayItem>();

    for (const FDreamFlowNodeDisplayItem& Item : Items)
    {
        PreviewBox->AddSlot()
            .AutoHeight()
            .Padding(FMargin(0.0f, 0.0f, 0.0f, 6.0f))
            [
                BuildDisplayItemWidget(Item)
            ];
    }

    return SNew(SBorder)
        .Visibility(this, &SGraphNode_DreamFlow::GetPreviewVisibility)
        .BorderImage(this, &SGraphNode_DreamFlow::GetBadgeBrush)
        .BorderBackgroundColor(FStyleColors::Dropdown)
        .Padding(FMargin(8.0f, 8.0f))
        [
            PreviewBox
        ];
}

TSharedRef<SWidget> SGraphNode_DreamFlow::BuildDisplayItemWidget(const FDreamFlowNodeDisplayItem& Item)
{
    const FText LabelText = Item.Label.IsEmpty() ? FText::GetEmpty() : Item.Label;

    if (Item.Type == EDreamFlowNodeDisplayItemType::Color)
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SColorBlock)
                .Color(Item.ColorValue)
                .Size(FVector2D(18.0f, 18.0f))
                .CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(!LabelText.IsEmpty() ? LabelText : FText::FromString(Item.ColorValue.ToString()))
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetBadgeTextColor)
            ];
    }

    if (Item.Type == EDreamFlowNodeDisplayItemType::Image)
    {
        UTexture2D* Texture = Item.Image.LoadSynchronous();
        if (Texture != nullptr)
        {
            TSharedPtr<FSlateBrush> ImageBrush = MakeShared<FSlateBrush>();
            ImageBrush->SetResourceObject(Texture);
            ImageBrush->ImageSize = FVector2D(
                FMath::Max(24.0f, Item.ImageSize.X),
                FMath::Max(24.0f, Item.ImageSize.Y));
            ImageBrush->DrawAs = ESlateBrushDrawType::Image;
            PreviewImageBrushes.Add(ImageBrush);

            return SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, LabelText.IsEmpty() ? 0.0f : 6.0f)
                [
                    SNew(SImage)
                    .Image(ImageBrush.Get())
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Visibility(LabelText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                    .Text(LabelText)
                    .TextStyle(FAppStyle::Get(), "SmallText")
                    .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetBadgeTextColor)
                ];
        }

        return SNew(STextBlock)
            .Text(!LabelText.IsEmpty() ? LabelText : FText::FromString(TEXT("Image Preview")))
            .TextStyle(FAppStyle::Get(), "SmallText")
            .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetMutedTextColor);
    }

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Visibility(LabelText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
            .Text(LabelText)
            .TextStyle(FAppStyle::Get(), "SmallText")
            .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetClassTextColor)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, LabelText.IsEmpty() ? 0.0f : 2.0f, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(Item.TextValue)
            .TextStyle(FAppStyle::Get(), "NormalText")
            .WrapTextAt(240.0f)
            .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetBadgeTextColor)
        ];
}

const FSlateBrush* SGraphNode_DreamFlow::GetOuterNodeBrush() const
{
    static const FSlateRoundedBoxBrush OuterNodeBrush(FStyleColors::White, 12.0f);
    return &OuterNodeBrush;
}

const FSlateBrush* SGraphNode_DreamFlow::GetInnerNodeBrush() const
{
    static const FSlateRoundedBoxBrush InnerNodeBrush(FStyleColors::White, 11.0f);
    return &InnerNodeBrush;
}

const FSlateBrush* SGraphNode_DreamFlow::GetMarkerBrush() const
{
    static const FSlateRoundedBoxBrush MarkerBrush(FStyleColors::White, 3.0f);
    return &MarkerBrush;
}

const FSlateBrush* SGraphNode_DreamFlow::GetBadgeBrush() const
{
    static const FSlateRoundedBoxBrush BadgeBrush(FStyleColors::White, 6.0f);
    return &BadgeBrush;
}

FSlateColor SGraphNode_DreamFlow::GetNodeOutlineColor() const
{
    if (IsSelectedExclusively())
    {
        return FSlateColor(EStyleColor::Primary);
    }

    if (IsHovered())
    {
        return FSlateColor(EStyleColor::Hover);
    }

    return FSlateColor(EStyleColor::InputOutline);
}

FSlateColor SGraphNode_DreamFlow::GetNodeSurfaceColor() const
{
    return FSlateColor(EStyleColor::Panel);
}

FSlateColor SGraphNode_DreamFlow::GetMarkerColor() const
{
    const FLinearColor MarkerColor = GetFlowNode() ? GetFlowNode()->GetNodeTitleColor() : FLinearColor(0.17f, 0.43f, 0.57f, 1.0f);
    return MarkerColor.CopyWithNewOpacity(0.92f);
}

FSlateColor SGraphNode_DreamFlow::GetTitleTextColor() const
{
    return FSlateColor(EStyleColor::Foreground);
}

FSlateColor SGraphNode_DreamFlow::GetClassTextColor() const
{
    return FSlateColor(EStyleColor::ForegroundHeader);
}

FSlateColor SGraphNode_DreamFlow::GetBadgeTextColor() const
{
    return FSlateColor(EStyleColor::Foreground);
}

FSlateColor SGraphNode_DreamFlow::GetMutedTextColor() const
{
    return FSlateColor(EStyleColor::ForegroundHeader);
}

FText SGraphNode_DreamFlow::GetNodeClassLabel() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    return RuntimeNode ? RuntimeNode->GetClass()->GetDisplayNameText() : FText::GetEmpty();
}

FText SGraphNode_DreamFlow::GetNodeCategoryLabel() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    return RuntimeNode ? RuntimeNode->GetNodeCategory() : FText::FromString(TEXT("Core"));
}

FText SGraphNode_DreamFlow::GetNodeAccentLabel() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    return RuntimeNode ? RuntimeNode->GetNodeAccentLabel() : FText::GetEmpty();
}

FText SGraphNode_DreamFlow::GetNodeConnectionLabel() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    if (RuntimeNode == nullptr)
    {
        return FText::GetEmpty();
    }

    if (RuntimeNode->IsTerminalNode())
    {
        return FText::FromString(TEXT("Terminal node"));
    }

    const FString ParentMode = RuntimeNode->SupportsMultipleParents() ? TEXT("many") : TEXT("single");
    const FString ChildMode = RuntimeNode->SupportsMultipleChildren() ? TEXT("many") : TEXT("single");
    return FText::FromString(FString::Printf(TEXT("Incoming: %s  |  Outgoing: %s"), *ParentMode, *ChildMode));
}

FText SGraphNode_DreamFlow::GetNodeDescription() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    return RuntimeNode ? RuntimeNode->Description : FText::GetEmpty();
}

EVisibility SGraphNode_DreamFlow::GetPreviewVisibility() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    return (RuntimeNode != nullptr && RuntimeNode->GetNodeDisplayItems().Num() > 0) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SGraphNode_DreamFlow::GetBreakpointVisibility() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    return (FlowNode != nullptr && FlowNode->HasBreakpoint()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SGraphNode_DreamFlow::GetAccentVisibility() const
{
    return GetNodeAccentLabel().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SGraphNode_DreamFlow::GetDescriptionVisibility() const
{
    return GetNodeDescription().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

UDreamFlowEdGraphNode* SGraphNode_DreamFlow::GetFlowNode() const
{
    return Cast<UDreamFlowEdGraphNode>(GraphNode);
}
