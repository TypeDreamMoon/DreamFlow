#include "Widgets/SDreamFlowDebuggerView.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "Execution/DreamFlowDebuggerSubsystem.h"
#include "Execution/DreamFlowExecutor.h"
#include "Engine/Engine.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace DreamFlowDebuggerView
{
    static FText GetDebugStateLabel(EDreamFlowExecutorDebugState DebugState)
    {
        switch (DebugState)
        {
        case EDreamFlowExecutorDebugState::Running:
            return FText::FromString(TEXT("Running"));

        case EDreamFlowExecutorDebugState::Paused:
            return FText::FromString(TEXT("Paused"));

        case EDreamFlowExecutorDebugState::Finished:
            return FText::FromString(TEXT("Finished"));

        case EDreamFlowExecutorDebugState::Idle:
        default:
            return FText::FromString(TEXT("Idle"));
        }
    }
}

void SDreamFlowDebuggerView::Construct(const FArguments& InArgs)
{
    FlowAsset = InArgs._FlowAsset;
    OnNodeGuidActivated = InArgs._OnNodeGuidActivated;

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
                .Text(FText::FromString(TEXT("Debugger")))
                .TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 10.0f)
            [
                SAssignNew(SummaryTextBlock, STextBlock)
                .Text(this, &SDreamFlowDebuggerView::GetSummaryText)
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                .WrapTextAt(320.0f)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Continue")))
                    .OnClicked(this, &SDreamFlowDebuggerView::HandleContinueClicked)
                    .IsEnabled(this, &SDreamFlowDebuggerView::CanContinue)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Step")))
                    .OnClicked(this, &SDreamFlowDebuggerView::HandleStepClicked)
                    .IsEnabled(this, &SDreamFlowDebuggerView::CanStep)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Pause")))
                    .OnClicked(this, &SDreamFlowDebuggerView::HandlePauseClicked)
                    .IsEnabled(this, &SDreamFlowDebuggerView::CanPause)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Stop")))
                    .OnClicked(this, &SDreamFlowDebuggerView::HandleStopClicked)
                    .IsEnabled(this, &SDreamFlowDebuggerView::CanStop)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Focus Node")))
                    .OnClicked(this, &SDreamFlowDebuggerView::HandleFocusNodeClicked)
                    .IsEnabled(this, &SDreamFlowDebuggerView::CanFocusNode)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 10.0f, 0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(this, &SDreamFlowDebuggerView::GetSelectedNodeText)
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                .WrapTextAt(320.0f)
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SAssignNew(ExecutorContainer, SScrollBox)
            ]
        ]
    ];

    RefreshExecutors();
}

void SDreamFlowDebuggerView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    RefreshExecutors();
}

void SDreamFlowDebuggerView::CleanupCachedExecutors()
{
    CachedExecutors.RemoveAll([](const TWeakObjectPtr<UDreamFlowExecutor>& WeakExecutor)
    {
        return !WeakExecutor.IsValid();
    });

    if (!SelectedExecutor.IsValid())
    {
        SelectedExecutor.Reset();
    }
}

void SDreamFlowDebuggerView::RefreshExecutors()
{
    const int32 PreviousCachedCount = CachedExecutors.Num();
    const bool bHadSelectedExecutor = SelectedExecutor.IsValid();
    CleanupCachedExecutors();
    const bool bLocalCleanupChanged = PreviousCachedCount != CachedExecutors.Num() || bHadSelectedExecutor != SelectedExecutor.IsValid();

    TArray<UDreamFlowExecutor*> NewExecutors;
    if (GEngine != nullptr)
    {
        if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
        {
            NewExecutors = DebuggerSubsystem->GetExecutorsForAsset(FlowAsset.Get());
        }
    }

    bool bSelectionChanged = false;
    if (SelectedExecutor.IsValid() && !NewExecutors.Contains(SelectedExecutor.Get()))
    {
        SelectedExecutor = NewExecutors.Num() > 0 ? NewExecutors[0] : nullptr;
        bSelectionChanged = true;
    }
    else if (!SelectedExecutor.IsValid() && NewExecutors.Num() > 0)
    {
        SelectedExecutor = NewExecutors[0];
        bSelectionChanged = true;
    }

    if (!HasExecutorSnapshotChanged(NewExecutors) && !bSelectionChanged && !bLocalCleanupChanged)
    {
        if (SummaryTextBlock.IsValid())
        {
            SummaryTextBlock->SetText(GetSummaryText());
        }
        return;
    }

    CachedExecutors.Reset();
    for (UDreamFlowExecutor* Executor : NewExecutors)
    {
        CachedExecutors.Add(Executor);
    }

    if (!SelectedExecutor.IsValid() && CachedExecutors.Num() > 0)
    {
        SelectedExecutor = CachedExecutors[0];
    }

    RebuildExecutorList();
}

void SDreamFlowDebuggerView::RebuildExecutorList()
{
    if (!ExecutorContainer.IsValid())
    {
        return;
    }

    ExecutorContainer->ClearChildren();

    if (SummaryTextBlock.IsValid())
    {
        SummaryTextBlock->SetText(GetSummaryText());
    }

    if (CachedExecutors.Num() == 0)
    {
        ExecutorContainer->AddSlot()
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("No active DreamFlow executors for this asset.")))
            .TextStyle(FAppStyle::Get(), "SmallText")
            .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
        ];
        return;
    }

    for (const TWeakObjectPtr<UDreamFlowExecutor>& WeakExecutor : CachedExecutors)
    {
        if (UDreamFlowExecutor* Executor = WeakExecutor.Get())
        {
            ExecutorContainer->AddSlot()
            .Padding(0.0f, 0.0f, 0.0f, 8.0f)
            [
                BuildExecutorCard(Executor)
            ];
        }
    }
}

TSharedRef<SWidget> SDreamFlowDebuggerView::BuildExecutorCard(UDreamFlowExecutor* Executor) const
{
    const TWeakObjectPtr<UDreamFlowExecutor> WeakExecutor = Executor;

    return SNew(SButton)
        .ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
        .ContentPadding(0.0f)
        .OnClicked(this, &SDreamFlowDebuggerView::HandleSelectExecutor, WeakExecutor)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .BorderBackgroundColor(this, &SDreamFlowDebuggerView::GetExecutorBorderColor, WeakExecutor)
            .Padding(1.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(this, &SDreamFlowDebuggerView::GetExecutorCardColor, WeakExecutor)
                .Padding(FMargin(10.0f, 8.0f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(this, &SDreamFlowDebuggerView::GetExecutorDisplayNameText, WeakExecutor)
                        .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                        .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 3.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text(this, &SDreamFlowDebuggerView::GetExecutorStateText, WeakExecutor)
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 3.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text(this, &SDreamFlowDebuggerView::GetExecutorCurrentNodeText, WeakExecutor)
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                        .WrapTextAt(260.0f)
                    ]
                ]
            ]
        ];
}

void SDreamFlowDebuggerView::SelectExecutor(UDreamFlowExecutor* Executor)
{
    SelectedExecutor = Executor;
    RebuildExecutorList();
}

FReply SDreamFlowDebuggerView::HandleContinueClicked() const
{
    if (UDreamFlowExecutor* Executor = GetSelectedExecutor())
    {
        Executor->ContinueExecution();
    }

    return FReply::Handled();
}

FReply SDreamFlowDebuggerView::HandleStepClicked() const
{
    if (UDreamFlowExecutor* Executor = GetSelectedExecutor())
    {
        Executor->StepExecution();
    }

    return FReply::Handled();
}

FReply SDreamFlowDebuggerView::HandlePauseClicked() const
{
    if (UDreamFlowExecutor* Executor = GetSelectedExecutor())
    {
        Executor->PauseExecution();
    }

    return FReply::Handled();
}

FReply SDreamFlowDebuggerView::HandleStopClicked() const
{
    if (UDreamFlowExecutor* Executor = GetSelectedExecutor())
    {
        Executor->FinishFlow();
    }

    return FReply::Handled();
}

FReply SDreamFlowDebuggerView::HandleFocusNodeClicked() const
{
    if (OnNodeGuidActivated.IsBound())
    {
        if (const UDreamFlowExecutor* Executor = GetSelectedExecutor())
        {
            if (const UDreamFlowNode* Node = Executor->GetCurrentNode())
            {
                OnNodeGuidActivated.Execute(Node->NodeGuid);
            }
        }
    }

    return FReply::Handled();
}

bool SDreamFlowDebuggerView::CanContinue() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    return Executor != nullptr && Executor->IsPaused();
}

bool SDreamFlowDebuggerView::CanStep() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    return Executor != nullptr && Executor->IsRunning();
}

bool SDreamFlowDebuggerView::CanPause() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    return Executor != nullptr && Executor->IsRunning() && !Executor->IsPaused();
}

bool SDreamFlowDebuggerView::CanStop() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    return Executor != nullptr && Executor->IsRunning();
}

bool SDreamFlowDebuggerView::CanFocusNode() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    return Executor != nullptr && Executor->GetCurrentNode() != nullptr;
}

FText SDreamFlowDebuggerView::GetSummaryText() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    if (Executor == nullptr)
    {
        return FText::FromString(TEXT("Run the flow in PIE or game to inspect active executors and breakpoints."));
    }

    return FText::Format(
        FText::FromString(TEXT("Selected executor state: {0}")),
        DreamFlowDebuggerView::GetDebugStateLabel(Executor->GetDebugState()));
}

FReply SDreamFlowDebuggerView::HandleSelectExecutor(TWeakObjectPtr<UDreamFlowExecutor> Executor) const
{
    const_cast<SDreamFlowDebuggerView*>(this)->SelectExecutor(Executor.Get());
    return FReply::Handled();
}

FText SDreamFlowDebuggerView::GetExecutorDisplayNameText(TWeakObjectPtr<UDreamFlowExecutor> Executor) const
{
    return FText::FromString(GetNameSafe(Executor.Get()));
}

FText SDreamFlowDebuggerView::GetExecutorStateText(TWeakObjectPtr<UDreamFlowExecutor> Executor) const
{
    if (!Executor.IsValid())
    {
        return FText::GetEmpty();
    }

    return DreamFlowDebuggerView::GetDebugStateLabel(Executor->GetDebugState());
}

FText SDreamFlowDebuggerView::GetExecutorCurrentNodeText(TWeakObjectPtr<UDreamFlowExecutor> Executor) const
{
    const UDreamFlowNode* CurrentNode = Executor.IsValid() ? Executor->GetCurrentNode() : nullptr;
    return CurrentNode != nullptr
        ? FText::Format(FText::FromString(TEXT("Current: {0}")), CurrentNode->GetNodeDisplayName())
        : FText::FromString(TEXT("Current: None"));
}

FText SDreamFlowDebuggerView::GetSelectedNodeText() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    const UDreamFlowNode* CurrentNode = Executor != nullptr ? Executor->GetCurrentNode() : nullptr;

    return CurrentNode != nullptr
        ? FText::Format(FText::FromString(TEXT("Focused node: {0}")), CurrentNode->GetNodeDisplayName())
        : FText::FromString(TEXT("Focused node: None"));
}

FSlateColor SDreamFlowDebuggerView::GetExecutorCardColor(TWeakObjectPtr<UDreamFlowExecutor> Executor) const
{
    return SelectedExecutor.Get() == Executor.Get()
        ? FSlateColor(EStyleColor::Primary)
        : FSlateColor(EStyleColor::Panel);
}

FSlateColor SDreamFlowDebuggerView::GetExecutorBorderColor(TWeakObjectPtr<UDreamFlowExecutor> Executor) const
{
    return SelectedExecutor.Get() == Executor.Get()
        ? FSlateColor(EStyleColor::PrimaryHover)
        : FSlateColor(EStyleColor::InputOutline);
}

bool SDreamFlowDebuggerView::HasExecutorSnapshotChanged(const TArray<UDreamFlowExecutor*>& NewExecutors) const
{
    if (NewExecutors.Num() != CachedExecutors.Num())
    {
        return true;
    }

    for (int32 Index = 0; Index < NewExecutors.Num(); ++Index)
    {
        if (CachedExecutors[Index].Get() != NewExecutors[Index])
        {
            return true;
        }
    }

    return false;
}

UDreamFlowExecutor* SDreamFlowDebuggerView::GetSelectedExecutor() const
{
    return SelectedExecutor.IsValid() ? SelectedExecutor.Get() : nullptr;
}
