#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "Widgets/SCompoundWidget.h"

class DREAMFLOWEDITOR_API SDreamFlowValidationView : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnMessageActivated, const FGuid&);

    SLATE_BEGIN_ARGS(SDreamFlowValidationView) {}
        SLATE_EVENT(FOnMessageActivated, OnMessageActivated)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void SetMessages(const TArray<FDreamFlowValidationMessage>& InMessages);

private:
    void RebuildMessages();
    FReply HandleMessageClicked(FGuid NodeGuid) const;
    FSlateColor GetSeverityColor(EDreamFlowValidationSeverity Severity) const;
    FText GetSummaryText() const;
    FText GetSeverityText(EDreamFlowValidationSeverity Severity) const;

private:
    FOnMessageActivated OnMessageActivated;
    TArray<FDreamFlowValidationMessage> Messages;
    TSharedPtr<class STextBlock> SummaryTextBlock;
    TSharedPtr<class SScrollBox> MessageContainer;
};
