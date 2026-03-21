#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UDreamFlowAsset;
class UDreamFlowExecutor;
class ITableRow;
class STableViewBase;
template <typename ItemType>
class SListView;

namespace DreamFlowDebuggerView
{
    struct FDebuggerInfoItem;
    struct FDebuggerVariableItem;
}

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
    void RebuildSelectedExecutorDetails();
    void RefreshSelectedExecutorItems();
    TSharedRef<SWidget> BuildExecutorCard(UDreamFlowExecutor* Executor) const;
    TSharedRef<SWidget> BuildSelectedExecutorInspector();
    FText GetSelectedExecutorStateText() const;
    FSlateColor GetSelectedExecutorStateColor() const;
    FSlateColor GetSelectedExecutorStateBackgroundColor() const;
    FText GetRuntimeVariablesTitleText() const;
    FText GetModifiedVariableSummaryText() const;
    int32 GetSelectedInspectorWidgetIndex() const;
    int32 GetVariableContentWidgetIndex() const;
    TSharedRef<SWidget> BuildInfoRow(const FText& Label, const FText& Value) const;
    TSharedRef<SWidget> BuildVariableRow(const struct FDreamFlowVariableDefinition& VariableDefinition, const struct FDreamFlowValue& CurrentValue, bool bHasRuntimeValue) const;
    TSharedRef<ITableRow> GenerateSelectedInfoRow(TSharedPtr<DreamFlowDebuggerView::FDebuggerInfoItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
    TSharedRef<ITableRow> GenerateSelectedVariableRow(TSharedPtr<DreamFlowDebuggerView::FDebuggerVariableItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
    void SelectExecutor(UDreamFlowExecutor* Executor);
    FReply HandleSelectExecutor(TWeakObjectPtr<UDreamFlowExecutor> Executor) const;
    FReply HandleContinueClicked() const;
    FReply HandleStepClicked() const;
    FReply HandlePauseClicked() const;
    FReply HandleStopClicked() const;
    FReply HandleFocusNodeClicked() const;
    FReply HandleCopySnapshotClicked() const;
    bool CanContinue() const;
    bool CanStep() const;
    bool CanPause() const;
    bool CanStop() const;
    bool CanFocusNode() const;
    bool CanCopySnapshot() const;
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
    TArray<TSharedPtr<DreamFlowDebuggerView::FDebuggerInfoItem>> SelectedInfoItems;
    TArray<TSharedPtr<DreamFlowDebuggerView::FDebuggerVariableItem>> SelectedVariableItems;
    FOnNodeGuidActivated OnNodeGuidActivated;
    TSharedPtr<class SBox> SelectedInspectorContainer;
    TSharedPtr<class SScrollBox> ExecutorContainer;
    TSharedPtr<SListView<TSharedPtr<DreamFlowDebuggerView::FDebuggerInfoItem>>> SelectedInfoListView;
    TSharedPtr<SListView<TSharedPtr<DreamFlowDebuggerView::FDebuggerVariableItem>>> SelectedVariableListView;
    TSharedPtr<class STextBlock> SummaryTextBlock;
    double NextRefreshTime = 0.0;
};
