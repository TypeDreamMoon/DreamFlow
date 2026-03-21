#include "Widgets/SDreamFlowDebuggerView.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEditorModule.h"
#include "DreamFlowNode.h"
#include "DreamFlowVariableTypes.h"
#include "Execution/DreamFlowDebuggerSubsystem.h"
#include "Execution/DreamFlowExecutor.h"
#include "Engine/Engine.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
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

        case EDreamFlowExecutorDebugState::Waiting:
            return FText::FromString(TEXT("Waiting"));

        case EDreamFlowExecutorDebugState::Paused:
            return FText::FromString(TEXT("Paused"));

        case EDreamFlowExecutorDebugState::Finished:
            return FText::FromString(TEXT("Finished"));

        case EDreamFlowExecutorDebugState::Idle:
        default:
            return FText::FromString(TEXT("Idle"));
        }
    }

    static FText GetValueTypeLabel(const EDreamFlowValueType Type)
    {
        switch (Type)
        {
        case EDreamFlowValueType::Bool:
            return FText::FromString(TEXT("Bool"));

        case EDreamFlowValueType::Int:
            return FText::FromString(TEXT("Int"));

        case EDreamFlowValueType::Float:
            return FText::FromString(TEXT("Float"));

        case EDreamFlowValueType::Name:
            return FText::FromString(TEXT("Name"));

        case EDreamFlowValueType::String:
            return FText::FromString(TEXT("String"));

        case EDreamFlowValueType::Text:
            return FText::FromString(TEXT("Text"));

        case EDreamFlowValueType::GameplayTag:
            return FText::FromString(TEXT("GameplayTag"));

        case EDreamFlowValueType::Object:
            return FText::FromString(TEXT("Object"));

        default:
            return FText::FromString(TEXT("Unknown"));
        }
    }

    static FSlateColor GetStateAccentColor(const EDreamFlowExecutorDebugState DebugState)
    {
        switch (DebugState)
        {
        case EDreamFlowExecutorDebugState::Running:
            return FSlateColor(EStyleColor::AccentGreen);

        case EDreamFlowExecutorDebugState::Waiting:
            return FSlateColor(EStyleColor::AccentBlue);

        case EDreamFlowExecutorDebugState::Paused:
            return FSlateColor(EStyleColor::Primary);

        case EDreamFlowExecutorDebugState::Finished:
            return FSlateColor(EStyleColor::ForegroundHeader);

        case EDreamFlowExecutorDebugState::Idle:
        default:
            return FSlateColor(EStyleColor::Warning);
        }
    }

    static TSharedRef<SWidget> BuildChip(const FText& Text, const FSlateColor& ForegroundColor, const FLinearColor& BackgroundColor)
    {
        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
            .BorderBackgroundColor(BackgroundColor)
            .Padding(FMargin(7.0f, 3.0f))
            [
                SNew(STextBlock)
                .Text(Text)
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(ForegroundColor)
            ];
    }

    static bool AreValuesEqual(const FDreamFlowValue& LeftValue, const FDreamFlowValue& RightValue)
    {
        if (LeftValue.Type != RightValue.Type)
        {
            return false;
        }

        switch (LeftValue.Type)
        {
        case EDreamFlowValueType::Bool:
            return LeftValue.BoolValue == RightValue.BoolValue;

        case EDreamFlowValueType::Int:
            return LeftValue.IntValue == RightValue.IntValue;

        case EDreamFlowValueType::Float:
            return FMath::IsNearlyEqual(LeftValue.FloatValue, RightValue.FloatValue);

        case EDreamFlowValueType::Name:
            return LeftValue.NameValue == RightValue.NameValue;

        case EDreamFlowValueType::String:
            return LeftValue.StringValue == RightValue.StringValue;

        case EDreamFlowValueType::Text:
            return LeftValue.TextValue.EqualTo(RightValue.TextValue);

        case EDreamFlowValueType::GameplayTag:
            return LeftValue.GameplayTagValue == RightValue.GameplayTagValue;

        case EDreamFlowValueType::Object:
            return LeftValue.ObjectValue == RightValue.ObjectValue;

        default:
            return false;
        }
    }

    static FString BuildSnapshotText(const UDreamFlowExecutor* Executor)
    {
        if (Executor == nullptr)
        {
            return FString();
        }

        FString Snapshot;
        Snapshot += FString::Printf(TEXT("Executor: %s\n"), *GetNameSafe(Executor));
        Snapshot += FString::Printf(TEXT("State: %s\n"), *GetDebugStateLabel(Executor->GetDebugState()).ToString());
        Snapshot += FString::Printf(TEXT("Flow Asset: %s\n"), *GetNameSafe(Executor->GetFlowAsset()));
        Snapshot += FString::Printf(TEXT("Current Node: %s\n"), *GetNameSafe(Executor->GetCurrentNode()));
        Snapshot += FString::Printf(TEXT("Execution Context: %s\n"), *GetNameSafe(Executor->GetExecutionContext()));
        Snapshot += FString::Printf(TEXT("Visited Nodes: %d\n"), Executor->GetVisitedNodes().Num());
        Snapshot += FString::Printf(TEXT("Available Children: %d\n"), Executor->GetAvailableChildren().Num());
        Snapshot += FString::Printf(TEXT("Pause On Breakpoints: %s\n"), Executor->GetPauseOnBreakpoints() ? TEXT("Enabled") : TEXT("Disabled"));

        if (const UDreamFlowAsset* Asset = Executor->GetFlowAsset())
        {
            Snapshot += TEXT("\nRuntime Variables:\n");
            if (Asset->Variables.Num() == 0)
            {
                Snapshot += TEXT("- <none>\n");
            }
            else
            {
                for (const FDreamFlowVariableDefinition& VariableDefinition : Asset->Variables)
                {
                    FDreamFlowValue CurrentValue = VariableDefinition.DefaultValue;
                    const bool bHasRuntimeValue = Executor->GetVariableValue(VariableDefinition.Name, CurrentValue);
                    const bool bIsModified = bHasRuntimeValue && !AreValuesEqual(CurrentValue, VariableDefinition.DefaultValue);

                    Snapshot += FString::Printf(
                        TEXT("- %s [%s] = %s"),
                        *VariableDefinition.Name.ToString(),
                        *GetValueTypeLabel(VariableDefinition.DefaultValue.Type).ToString(),
                        *CurrentValue.Describe());

                    if (!bHasRuntimeValue)
                    {
                        Snapshot += TEXT(" (default)");
                    }
                    else if (bIsModified)
                    {
                        Snapshot += FString::Printf(TEXT(" (default: %s)"), *VariableDefinition.DefaultValue.Describe());
                    }

                    Snapshot += TEXT("\n");
                }
            }
        }

        return Snapshot;
    }

    static void SyncPIEToExecutorState(const UDreamFlowExecutor* Executor)
    {
        if (!FDreamFlowEditorModule::IsAvailable())
        {
            return;
        }

        FDreamFlowEditorModule& EditorModule = FDreamFlowEditorModule::Get();
        EditorModule.SyncPlaySessionToDebuggerState(Executor != nullptr && Executor->IsPaused());
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
                        .Text(FText::FromString(TEXT("Debugger")))
                        .TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
                        .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                    [
                        SAssignNew(SummaryTextBlock, STextBlock)
                        .Text(this, &SDreamFlowDebuggerView::GetSummaryText)
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                        .WrapTextAt(320.0f)
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 10.0f, 12.0f, 0.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor(EStyleColor::Panel))
                .Padding(FMargin(10.0f, 8.0f))
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
                    .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Focus Node")))
                        .OnClicked(this, &SDreamFlowDebuggerView::HandleFocusNodeClicked)
                        .IsEnabled(this, &SDreamFlowDebuggerView::CanFocusNode)
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Copy Snapshot")))
                        .OnClicked(this, &SDreamFlowDebuggerView::HandleCopySnapshotClicked)
                        .IsEnabled(this, &SDreamFlowDebuggerView::CanCopySnapshot)
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 10.0f, 12.0f, 0.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor(EStyleColor::Header))
                .Padding(FMargin(8.0f, 6.0f))
                [
                    SNew(STextBlock)
                    .Text(this, &SDreamFlowDebuggerView::GetSelectedNodeText)
                    .TextStyle(FAppStyle::Get(), "SmallText")
                    .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                    .WrapTextAt(320.0f)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 10.0f, 12.0f, 0.0f)
            [
                SAssignNew(SelectedInspectorContainer, SVerticalBox)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 12.0f, 12.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Active Sessions")))
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(12.0f, 0.0f, 12.0f, 12.0f)
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

        RebuildSelectedExecutorDetails();
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

    RebuildSelectedExecutorDetails();

    if (CachedExecutors.Num() == 0)
    {
        ExecutorContainer->AddSlot()
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
            .Padding(FMargin(12.0f, 10.0f))
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("No active DreamFlow executors for this asset.")))
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                .WrapTextAt(320.0f)
            ]
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

void SDreamFlowDebuggerView::RebuildSelectedExecutorDetails()
{
    if (!SelectedInspectorContainer.IsValid())
    {
        return;
    }

    SelectedInspectorContainer->ClearChildren();
    SelectedInspectorContainer->AddSlot()
    .AutoHeight()
    [
        BuildSelectedExecutorInspector()
    ];
}

TSharedRef<SWidget> SDreamFlowDebuggerView::BuildExecutorCard(UDreamFlowExecutor* Executor) const
{
    const TWeakObjectPtr<UDreamFlowExecutor> WeakExecutor = Executor;
    const FText ContextLabel = Executor->GetExecutionContext() != nullptr
        ? Executor->GetExecutionContext()->GetClass()->GetDisplayNameText()
        : FText::FromString(TEXT("No Context"));
    const FText VisitLabel = FText::Format(
        FText::FromString(TEXT("{0} visited")),
        FText::AsNumber(Executor->GetVisitedNodes().Num()));
    const FSlateColor StateColor = DreamFlowDebuggerView::GetStateAccentColor(Executor->GetDebugState());

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
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(StateColor)
                    .Padding(FMargin(3.0f, 0.0f))
                ]

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
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
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            [
                                SNew(STextBlock)
                                .Text(this, &SDreamFlowDebuggerView::GetExecutorDisplayNameText, WeakExecutor)
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(StateColor.GetSpecifiedColor().CopyWithNewOpacity(0.16f))
                                .Padding(FMargin(6.0f, 2.0f))
                                [
                                    SNew(STextBlock)
                                    .Text(this, &SDreamFlowDebuggerView::GetExecutorStateText, WeakExecutor)
                                    .TextStyle(FAppStyle::Get(), "SmallText")
                                    .ColorAndOpacity(StateColor)
                                ]
                            ]
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(this, &SDreamFlowDebuggerView::GetExecutorCurrentNodeText, WeakExecutor)
                            .TextStyle(FAppStyle::Get(), "SmallText")
                            .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                            .WrapTextAt(260.0f)
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SNew(STextBlock)
                                .Text(ContextLabel)
                                .TextStyle(FAppStyle::Get(), "SmallText")
                                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(VisitLabel)
                                .TextStyle(FAppStyle::Get(), "SmallText")
                                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                            ]
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SDreamFlowDebuggerView::BuildSelectedExecutorInspector() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    if (Executor == nullptr)
    {
        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(10.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Select an active executor to inspect runtime variables, context, and execution state.")))
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                .WrapTextAt(320.0f)
            ];
    }

    const UDreamFlowAsset* Asset = Executor->GetFlowAsset();
    const UDreamFlowNode* CurrentNode = Executor->GetCurrentNode();
    const UObject* ExecutionContext = Executor->GetExecutionContext();
    const FText ExecutionContextText = ExecutionContext != nullptr
        ? FText::Format(
            FText::FromString(TEXT("{0} ({1})")),
            ExecutionContext->GetClass()->GetDisplayNameText(),
            FText::FromString(GetNameSafe(ExecutionContext)))
        : FText::FromString(TEXT("None"));

    bool bIsCurrentBreakpoint = false;
    int32 ModifiedVariableCount = 0;
    if (GEngine != nullptr && Asset != nullptr && CurrentNode != nullptr)
    {
        if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
        {
            DebuggerSubsystem->IsNodeCurrentExecutionLocation(Asset, CurrentNode->NodeGuid, bIsCurrentBreakpoint);
        }
    }

    TSharedRef<SVerticalBox> VariableRows = SNew(SVerticalBox);
    if (Asset == nullptr || Asset->Variables.Num() == 0)
    {
        VariableRows->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("This flow asset does not define any environment variables.")))
            .TextStyle(FAppStyle::Get(), "SmallText")
            .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
        ];
    }
    else
    {
        for (const FDreamFlowVariableDefinition& VariableDefinition : Asset->Variables)
        {
            FDreamFlowValue CurrentValue = VariableDefinition.DefaultValue;
            const bool bHasRuntimeValue = Executor->GetVariableValue(VariableDefinition.Name, CurrentValue);
            ModifiedVariableCount += bHasRuntimeValue && !DreamFlowDebuggerView::AreValuesEqual(CurrentValue, VariableDefinition.DefaultValue) ? 1 : 0;

            VariableRows->AddSlot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                BuildVariableRow(VariableDefinition, CurrentValue, bHasRuntimeValue)
            ];
        }
    }

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(10.0f)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Selected Executor")))
                    .TextStyle(FAppStyle::Get(), "SmallText")
                    .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(DreamFlowDebuggerView::GetStateAccentColor(Executor->GetDebugState()).GetSpecifiedColor().CopyWithNewOpacity(0.16f))
                    .Padding(FMargin(6.0f, 2.0f))
                    [
                        SNew(STextBlock)
                        .Text(DreamFlowDebuggerView::GetDebugStateLabel(Executor->GetDebugState()))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(DreamFlowDebuggerView::GetStateAccentColor(Executor->GetDebugState()))
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 10.0f, 0.0f, 0.0f)
            [
                SNew(SGridPanel)
                .FillColumn(0, 1.0f)
                .FillColumn(1, 1.0f)

                + SGridPanel::Slot(0, 0)
                .Padding(0.0f, 0.0f, 6.0f, 8.0f)
                [
                    BuildInfoRow(FText::FromString(TEXT("Name")), FText::FromString(GetNameSafe(Executor)))
                ]

                + SGridPanel::Slot(1, 0)
                .Padding(6.0f, 0.0f, 0.0f, 8.0f)
                [
                    BuildInfoRow(FText::FromString(TEXT("Flow")), FText::FromString(GetNameSafe(Asset)))
                ]

                + SGridPanel::Slot(0, 1)
                .Padding(0.0f, 0.0f, 6.0f, 8.0f)
                [
                    BuildInfoRow(
                        FText::FromString(TEXT("Current Node")),
                        CurrentNode != nullptr ? CurrentNode->GetNodeDisplayName() : FText::FromString(TEXT("None")))
                ]

                + SGridPanel::Slot(1, 1)
                .Padding(6.0f, 0.0f, 0.0f, 8.0f)
                [
                    BuildInfoRow(FText::FromString(TEXT("Execution Context")), ExecutionContextText)
                ]

                + SGridPanel::Slot(0, 2)
                .Padding(0.0f, 0.0f, 6.0f, 8.0f)
                [
                    BuildInfoRow(FText::FromString(TEXT("Visited Nodes")), FText::AsNumber(Executor->GetVisitedNodes().Num()))
                ]

                + SGridPanel::Slot(1, 2)
                .Padding(6.0f, 0.0f, 0.0f, 8.0f)
                [
                    BuildInfoRow(FText::FromString(TEXT("Available Children")), FText::AsNumber(Executor->GetAvailableChildren().Num()))
                ]

                + SGridPanel::Slot(0, 3)
                .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                [
                    BuildInfoRow(
                        FText::FromString(TEXT("Pause On Breakpoints")),
                        Executor->GetPauseOnBreakpoints() ? FText::FromString(TEXT("Enabled")) : FText::FromString(TEXT("Disabled")))
                ]

                + SGridPanel::Slot(1, 3)
                .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                [
                    BuildInfoRow(
                        FText::FromString(TEXT("Breakpoint Focus")),
                        bIsCurrentBreakpoint ? FText::FromString(TEXT("Hit Breakpoint")) : FText::FromString(TEXT("No Breakpoint")))
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 10.0f, 0.0f, 6.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::Format(
                        FText::FromString(TEXT("Runtime Variables ({0})")),
                        FText::AsNumber(Asset != nullptr ? Asset->Variables.Num() : 0)))
                    .TextStyle(FAppStyle::Get(), "SmallText")
                    .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(FText::Format(
                        FText::FromString(TEXT("{0} modified")),
                        FText::AsNumber(ModifiedVariableCount)))
                    .TextStyle(FAppStyle::Get(), "SmallText")
                    .ColorAndOpacity(ModifiedVariableCount > 0 ? FSlateColor(EStyleColor::Primary) : FSlateColor(EStyleColor::ForegroundHeader))
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor(EStyleColor::Header))
                .Padding(FMargin(10.0f, 6.0f))
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .FillWidth(0.24f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Variable")))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(0.12f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Type")))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(0.28f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Value")))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(0.24f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Default")))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(0.12f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("State")))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBox)
                .MaxDesiredHeight(300.0f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        VariableRows
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SDreamFlowDebuggerView::BuildInfoRow(const FText& Label, const FText& Value) const
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
        .Padding(FMargin(10.0f, 8.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(Label)
                .TextStyle(FAppStyle::Get(), "SmallText")
                .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(Value)
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                .AutoWrapText(true)
            ]
        ];
}

TSharedRef<SWidget> SDreamFlowDebuggerView::BuildVariableRow(const FDreamFlowVariableDefinition& VariableDefinition, const FDreamFlowValue& CurrentValue, bool bHasRuntimeValue) const
{
    const bool bIsModified = bHasRuntimeValue && !DreamFlowDebuggerView::AreValuesEqual(CurrentValue, VariableDefinition.DefaultValue);
    const FText StatusText = !bHasRuntimeValue
        ? FText::FromString(TEXT("Default"))
        : (bIsModified ? FText::FromString(TEXT("Modified")) : FText::FromString(TEXT("Runtime")));

    const FSlateColor StatusColor = !bHasRuntimeValue
        ? FSlateColor(EStyleColor::ForegroundHeader)
        : (bIsModified ? FSlateColor(EStyleColor::Primary) : FSlateColor(EStyleColor::AccentGreen));

    const FText NameText = VariableDefinition.Name.IsNone()
        ? FText::FromString(TEXT("<unnamed>"))
        : FText::FromName(VariableDefinition.Name);

    const bool bShowDefaultLine = !bHasRuntimeValue || bIsModified;
    const FLinearColor AccentColor = StatusColor.GetSpecifiedColor();
    const FLinearColor StatusBackground = AccentColor.CopyWithNewOpacity(bIsModified ? 0.18f : 0.12f);

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(1.0f)
        .ToolTipText(VariableDefinition.Description.IsEmpty() ? NameText : VariableDefinition.Description)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(AccentColor)
                .Padding(FMargin(2.0f, 0.0f))
            ]

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor(EStyleColor::Recessed))
                .Padding(FMargin(10.0f, 8.0f))
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .FillWidth(0.24f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(NameText)
                        .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                        .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                        .AutoWrapText(true)
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(0.12f)
                    .Padding(8.0f, 0.0f, 8.0f, 0.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(DreamFlowDebuggerView::GetValueTypeLabel(VariableDefinition.DefaultValue.Type))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(0.28f)
                    .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(CurrentValue.Describe()))
                        .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                        .AutoWrapText(true)
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(0.24f)
                    .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(VariableDefinition.DefaultValue.Describe()))
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .ColorAndOpacity(bShowDefaultLine ? FSlateColor(EStyleColor::ForegroundHeader) : FSlateColor(EStyleColor::ForegroundHover))
                        .AutoWrapText(true)
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(0.12f)
                    .VAlign(VAlign_Center)
                    [
                        DreamFlowDebuggerView::BuildChip(StatusText, StatusColor, StatusBackground)
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
        if (Executor->ContinueExecution())
        {
            DreamFlowDebuggerView::SyncPIEToExecutorState(Executor);
        }
    }

    return FReply::Handled();
}

FReply SDreamFlowDebuggerView::HandleStepClicked() const
{
    if (UDreamFlowExecutor* Executor = GetSelectedExecutor())
    {
        if (Executor->StepExecution())
        {
            DreamFlowDebuggerView::SyncPIEToExecutorState(Executor);
        }
    }

    return FReply::Handled();
}

FReply SDreamFlowDebuggerView::HandlePauseClicked() const
{
    if (UDreamFlowExecutor* Executor = GetSelectedExecutor())
    {
        if (Executor->PauseExecution())
        {
            DreamFlowDebuggerView::SyncPIEToExecutorState(Executor);
        }
    }

    return FReply::Handled();
}

FReply SDreamFlowDebuggerView::HandleStopClicked() const
{
    if (UDreamFlowExecutor* Executor = GetSelectedExecutor())
    {
        Executor->FinishFlow();
        DreamFlowDebuggerView::SyncPIEToExecutorState(Executor);
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

FReply SDreamFlowDebuggerView::HandleCopySnapshotClicked() const
{
    if (const UDreamFlowExecutor* Executor = GetSelectedExecutor())
    {
        const FString Snapshot = DreamFlowDebuggerView::BuildSnapshotText(Executor);
        FPlatformApplicationMisc::ClipboardCopy(*Snapshot);
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

bool SDreamFlowDebuggerView::CanCopySnapshot() const
{
    return GetSelectedExecutor() != nullptr;
}

FText SDreamFlowDebuggerView::GetSummaryText() const
{
    const UDreamFlowExecutor* Executor = GetSelectedExecutor();
    if (Executor == nullptr)
    {
        return FText::Format(
            FText::FromString(TEXT("Run the flow in PIE or game to inspect active executors, breakpoints, and runtime variables. Active executors: {0}")),
            FText::AsNumber(CachedExecutors.Num()));
    }

    return FText::Format(
        FText::FromString(TEXT("Inspect runtime state for {0}. Active executors: {1}  Selected state: {2}")),
        Executor->GetFlowAsset() != nullptr ? FText::FromString(Executor->GetFlowAsset()->GetName()) : FText::FromString(TEXT("this flow")),
        FText::AsNumber(CachedExecutors.Num()),
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
        ? FSlateColor(EStyleColor::Panel)
        : FSlateColor(EStyleColor::Recessed);
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
