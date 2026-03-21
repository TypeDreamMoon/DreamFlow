#pragma once

#include "CoreMinimal.h"
#include "DreamFlowTypes.h"
#include "Widgets/SCompoundWidget.h"

class DREAMFLOWEDITOR_API SDreamFlowValidationView : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnMessageActivated, const FGuid&);
    DECLARE_DELEGATE(FOnValidateRequested);

    SLATE_BEGIN_ARGS(SDreamFlowValidationView) {}
        SLATE_EVENT(FOnMessageActivated, OnMessageActivated)
        SLATE_EVENT(FOnValidateRequested, OnValidateRequested)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void SetValidationState(const TArray<FDreamFlowValidationMessage>& InMessages, bool bInHasValidationRun, bool bInValidationDirty);

private:
    void RebuildMessages();
    FReply HandleMessageClicked(FGuid NodeGuid) const;
    FReply HandleValidateClicked() const;
    FSlateColor GetSeverityColor(EDreamFlowValidationSeverity Severity) const;
    FText GetSummaryText() const;
    FText GetSeverityText(EDreamFlowValidationSeverity Severity) const;
    FText GetValidateButtonText() const;
    FText GetStatusText() const;
    EVisibility GetStatusVisibility() const;
    FSlateColor GetStatusColor() const;

private:
    FOnMessageActivated OnMessageActivated;
    FOnValidateRequested OnValidateRequested;
    TArray<FDreamFlowValidationMessage> Messages;
    bool bHasValidationRun = false;
    bool bValidationDirty = true;
    TSharedPtr<class STextBlock> SummaryTextBlock;
    TSharedPtr<class STextBlock> StatusTextBlock;
    TSharedPtr<class SScrollBox> MessageContainer;
};
