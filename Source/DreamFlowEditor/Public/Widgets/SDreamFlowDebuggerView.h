#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UDreamFlowAsset;
class UDreamFlowExecutor;

class DREAMFLOWEDITOR_API SDreamFlowDebuggerView : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnNodeGuidActivated, const FGuid&);

    SLATE_BEGIN_ARGS(SDreamFlowDebuggerView) {}
        SLATE_ARGUMENT(UDreamFlowAsset*, FlowAsset)
        SLATE_EVENT(FOnNodeGuidActivated, OnNodeGuidActivated)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    void CleanupCachedExecutors();
    void RefreshExecutors();
    void RebuildExecutorList();
    TSharedRef<SWidget> BuildExecutorCard(UDreamFlowExecutor* Executor) const;
    void SelectExecutor(UDreamFlowExecutor* Executor);
    FReply HandleSelectExecutor(TWeakObjectPtr<UDreamFlowExecutor> Executor) const;
    FReply HandleContinueClicked() const;
    FReply HandleStepClicked() const;
    FReply HandlePauseClicked() const;
    FReply HandleStopClicked() const;
    FReply HandleFocusNodeClicked() const;
    bool CanContinue() const;
    bool CanStep() const;
    bool CanPause() const;
    bool CanStop() const;
    bool CanFocusNode() const;
    FText GetSummaryText() const;
    FText GetExecutorDisplayNameText(TWeakObjectPtr<UDreamFlowExecutor> Executor) const;
    FText GetExecutorStateText(TWeakObjectPtr<UDreamFlowExecutor> Executor) const;
    FText GetExecutorCurrentNodeText(TWeakObjectPtr<UDreamFlowExecutor> Executor) const;
    FText GetSelectedNodeText() const;
    FSlateColor GetExecutorCardColor(TWeakObjectPtr<UDreamFlowExecutor> Executor) const;
    FSlateColor GetExecutorBorderColor(TWeakObjectPtr<UDreamFlowExecutor> Executor) const;
    bool HasExecutorSnapshotChanged(const TArray<UDreamFlowExecutor*>& NewExecutors) const;
    UDreamFlowExecutor* GetSelectedExecutor() const;

private:
    TWeakObjectPtr<UDreamFlowAsset> FlowAsset;
    TWeakObjectPtr<UDreamFlowExecutor> SelectedExecutor;
    TArray<TWeakObjectPtr<UDreamFlowExecutor>> CachedExecutors;
    FOnNodeGuidActivated OnNodeGuidActivated;
    TSharedPtr<class SScrollBox> ExecutorContainer;
    TSharedPtr<class STextBlock> SummaryTextBlock;
};
