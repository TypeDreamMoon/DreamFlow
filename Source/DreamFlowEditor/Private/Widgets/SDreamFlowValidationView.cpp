#include "Widgets/SDreamFlowValidationView.h"

#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SDreamFlowValidationView::Construct(const FArguments& InArgs)
{
    OnMessageActivated = InArgs._OnMessageActivated;

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
                        .Text(FText::FromString(TEXT("Validation")))
                        .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
                        .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                    [
                        SAssignNew(SummaryTextBlock, STextBlock)
                        .Text(this, &SDreamFlowValidationView::GetSummaryText)
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                        .WrapTextAt(320.0f)
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(12.0f, 10.0f, 12.0f, 12.0f)
            [
                SAssignNew(MessageContainer, SScrollBox)
            ]
        ]
    ];

    RebuildMessages();
}

void SDreamFlowValidationView::SetMessages(const TArray<FDreamFlowValidationMessage>& InMessages)
{
    Messages = InMessages;
    RebuildMessages();
}

void SDreamFlowValidationView::RebuildMessages()
{
    if (!MessageContainer.IsValid())
    {
        return;
    }

    MessageContainer->ClearChildren();

    if (SummaryTextBlock.IsValid())
    {
        SummaryTextBlock->SetText(GetSummaryText());
    }

    if (Messages.Num() == 0)
    {
        MessageContainer->AddSlot()
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
            .Padding(FMargin(12.0f, 10.0f))
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("No validation issues were found. This flow is ready for editing and runtime testing.")))
                .TextStyle(FAppStyle::Get(), "NormalText")
                .ColorAndOpacity(FSlateColor(EStyleColor::Success))
                .WrapTextAt(320.0f)
            ]
        ];
        return;
    }

    for (const FDreamFlowValidationMessage& Message : Messages)
    {
        MessageContainer->AddSlot()
        .Padding(0.0f, 0.0f, 0.0f, 8.0f)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
            .OnClicked(this, &SDreamFlowValidationView::HandleMessageClicked, Message.NodeGuid)
            .ContentPadding(0.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .BorderBackgroundColor(GetSeverityColor(Message.Severity))
                .Padding(1.0f)
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(GetSeverityColor(Message.Severity))
                        .Padding(FMargin(3.0f, 0.0f))
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
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
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(SBorder)
                                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                                    .BorderBackgroundColor(GetSeverityColor(Message.Severity).GetSpecifiedColor().CopyWithNewOpacity(0.16f))
                                    .Padding(FMargin(6.0f, 2.0f))
                                    [
                                        SNew(STextBlock)
                                        .Text(GetSeverityText(Message.Severity))
                                        .TextStyle(FAppStyle::Get(), "SmallText")
                                        .ColorAndOpacity(GetSeverityColor(Message.Severity))
                                    ]
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                                [
                                    SNew(STextBlock)
                                    .Text(!Message.NodeTitle.IsEmpty() ? Message.NodeTitle : FText::FromString(TEXT("Graph")))
                                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                    .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                                ]
                            ]

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(Message.Message)
                                .TextStyle(FAppStyle::Get(), "SmallText")
                                .WrapTextAt(340.0f)
                                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                            ]
                        ]
                    ]
                ]
            ]
        ];
    }
}

FReply SDreamFlowValidationView::HandleMessageClicked(FGuid NodeGuid) const
{
    if (NodeGuid.IsValid() && OnMessageActivated.IsBound())
    {
        OnMessageActivated.Execute(NodeGuid);
    }

    return FReply::Handled();
}

FSlateColor SDreamFlowValidationView::GetSeverityColor(EDreamFlowValidationSeverity Severity) const
{
    switch (Severity)
    {
    case EDreamFlowValidationSeverity::Info:
        return FSlateColor(EStyleColor::AccentBlue);

    case EDreamFlowValidationSeverity::Warning:
        return FSlateColor(EStyleColor::Warning);

    case EDreamFlowValidationSeverity::Error:
    default:
        return FSlateColor(EStyleColor::Error);
    }
}

FText SDreamFlowValidationView::GetSummaryText() const
{
    int32 ErrorCount = 0;
    int32 WarningCount = 0;
    int32 InfoCount = 0;

    for (const FDreamFlowValidationMessage& Message : Messages)
    {
        switch (Message.Severity)
        {
        case EDreamFlowValidationSeverity::Info:
            ++InfoCount;
            break;

        case EDreamFlowValidationSeverity::Warning:
            ++WarningCount;
            break;

        case EDreamFlowValidationSeverity::Error:
        default:
            ++ErrorCount;
            break;
        }
    }

    return FText::Format(
        FText::FromString(TEXT("{0} errors, {1} warnings, {2} infos. Click an item to focus the owning node.")),
        ErrorCount,
        WarningCount,
        InfoCount);
}

FText SDreamFlowValidationView::GetSeverityText(EDreamFlowValidationSeverity Severity) const
{
    switch (Severity)
    {
    case EDreamFlowValidationSeverity::Info:
        return FText::FromString(TEXT("Info"));

    case EDreamFlowValidationSeverity::Warning:
        return FText::FromString(TEXT("Warning"));

    case EDreamFlowValidationSeverity::Error:
    default:
        return FText::FromString(TEXT("Error"));
    }
}
