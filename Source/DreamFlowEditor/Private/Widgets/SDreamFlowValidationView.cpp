#include "Widgets/SDreamFlowValidationView.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
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
        .Padding(12.0f)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Validation")))
                .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 10.0f)
            [
                SAssignNew(SummaryTextBlock, STextBlock)
                .Text(this, &SDreamFlowValidationView::GetSummaryText)
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FLinearColor(0.79f, 0.83f, 0.88f, 0.95f))
                .WrapTextAt(320.0f)
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
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
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("No validation issues were found.")))
            .TextStyle(FAppStyle::Get(), "NormalText")
            .ColorAndOpacity(FLinearColor(0.72f, 0.88f, 0.75f, 0.96f))
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
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(GetSeverityColor(Message.Severity))
                .Padding(1.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(FLinearColor(0.07f, 0.10f, 0.13f, 1.0f))
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
                                SNew(STextBlock)
                                .Text(GetSeverityText(Message.Severity))
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                .ColorAndOpacity(GetSeverityColor(Message.Severity))
                            ]
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(!Message.NodeTitle.IsEmpty() ? Message.NodeTitle : FText::FromString(TEXT("Graph")))
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                .ColorAndOpacity(FLinearColor::White)
                            ]
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(Message.Message)
                            .TextStyle(FAppStyle::Get(), "SmallText")
                            .WrapTextAt(340.0f)
                            .ColorAndOpacity(FLinearColor(0.80f, 0.84f, 0.89f, 0.97f))
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
        return FLinearColor(0.21f, 0.67f, 0.89f, 1.0f);

    case EDreamFlowValidationSeverity::Warning:
        return FLinearColor(0.94f, 0.66f, 0.22f, 1.0f);

    case EDreamFlowValidationSeverity::Error:
    default:
        return FLinearColor(0.92f, 0.24f, 0.29f, 1.0f);
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
        FText::FromString(TEXT("{0} errors, {1} warnings, {2} infos")),
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
