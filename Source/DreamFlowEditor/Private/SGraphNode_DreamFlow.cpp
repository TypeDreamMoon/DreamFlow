#include "SGraphNode_DreamFlow.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEditorToolkit.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowNodeDisplayTypes.h"
#include "DreamFlowNode.h"
#include "Execution/DreamFlowDebuggerSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "DetailLayoutBuilder.h"
#include "IDetailTreeNode.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "IPropertyRowGenerator.h"
#include "Modules/ModuleManager.h"
#include "PropertyHandle.h"
#include "PropertyEditorModule.h"
#include "Styling/SlateIconFinder.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "KismetPins/SGraphPinExec.h"
#include "SGraphPin.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace DreamFlowNodeWidget
{
    static int32 GetInlinePropertyPriority(const UDreamFlowNode* RuntimeNode, const FName PropertyName)
    {
        if (RuntimeNode == nullptr)
        {
            return MAX_int32;
        }

        if (const FProperty* Property = RuntimeNode->GetClass()->FindPropertyByName(PropertyName))
        {
            const FString PriorityString = Property->GetMetaData(TEXT("DreamFlowInlinePriority"));
            if (!PriorityString.IsEmpty())
            {
                return FCString::Atoi(*PriorityString);
            }
        }

        return MAX_int32;
    }

    static const FSlateBrush* ResolveNodeIconBrush(const UDreamFlowNode* RuntimeNode, TSharedPtr<FSlateBrush>& InOutCustomBrush)
    {
        InOutCustomBrush.Reset();

        if (RuntimeNode == nullptr)
        {
            return nullptr;
        }

        if (UTexture2D* NodeIconTexture = RuntimeNode->GetNodeIconTexture())
        {
            InOutCustomBrush = MakeShared<FSlateBrush>();
            InOutCustomBrush->SetResourceObject(NodeIconTexture);
            InOutCustomBrush->ImageSize = FVector2D(18.0f, 18.0f);
            InOutCustomBrush->DrawAs = ESlateBrushDrawType::Image;
            return InOutCustomBrush.Get();
        }

        const FName StyleName = RuntimeNode->GetNodeIconStyleName();
        if (!StyleName.IsNone())
        {
            if (const FSlateBrush* Brush = FAppStyle::GetBrush(StyleName))
            {
                return Brush;
            }
        }

        return FSlateIconFinder::FindIconBrushForClass(RuntimeNode->GetClass());
    }

    static bool IsExecutionFocused(const UDreamFlowEdGraphNode* FlowNode, bool& bOutIsBreakpointHit)
    {
        bOutIsBreakpointHit = false;

        const UDreamFlowNode* RuntimeNode = FlowNode != nullptr ? FlowNode->GetRuntimeNode() : nullptr;
        const UDreamFlowAsset* FlowAsset = FlowNode != nullptr ? FDreamFlowEditorUtils::GetFlowAssetFromGraph(FlowNode->GetGraph()) : nullptr;
        if (RuntimeNode == nullptr || FlowAsset == nullptr || !RuntimeNode->NodeGuid.IsValid() || GEngine == nullptr)
        {
            return false;
        }

        const UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>();
        return DebuggerSubsystem != nullptr && DebuggerSubsystem->IsNodeCurrentExecutionLocation(FlowAsset, RuntimeNode->NodeGuid, bOutIsBreakpointHit);
    }

    static EVisibility GetExecutionBadgeVisibility(const UDreamFlowEdGraphNode* FlowNode)
    {
        bool bIsBreakpointHit = false;
        return IsExecutionFocused(FlowNode, bIsBreakpointHit) ? EVisibility::Visible : EVisibility::Collapsed;
    }

    static FText GetExecutionBadgeLabel(const UDreamFlowEdGraphNode* FlowNode)
    {
        bool bIsBreakpointHit = false;
        if (!IsExecutionFocused(FlowNode, bIsBreakpointHit))
        {
            return FText::GetEmpty();
        }

        return bIsBreakpointHit
            ? FText::FromString(TEXT("Paused"))
            : FText::FromString(TEXT("Active"));
    }

    static FSlateColor GetExecutionBadgeColor(const UDreamFlowEdGraphNode* FlowNode)
    {
        bool bIsBreakpointHit = false;
        const bool bIsFocused = IsExecutionFocused(FlowNode, bIsBreakpointHit);
        if (!bIsFocused)
        {
            return FSlateColor(EStyleColor::Primary);
        }

        return bIsBreakpointHit
            ? FSlateColor(EStyleColor::Error)
            : FSlateColor(EStyleColor::Primary);
    }

    static int32 GetValidationPriority(const EDreamFlowValidationSeverity Severity)
    {
        switch (Severity)
        {
        case EDreamFlowValidationSeverity::Error:
            return 3;

        case EDreamFlowValidationSeverity::Warning:
            return 2;

        case EDreamFlowValidationSeverity::Info:
        default:
            return 1;
        }
    }

    static EDreamFlowValidationSeverity GetHighestValidationSeverity(const TArray<FDreamFlowValidationMessage>& Messages)
    {
        EDreamFlowValidationSeverity HighestSeverity = EDreamFlowValidationSeverity::Info;
        int32 HighestPriority = 0;

        for (const FDreamFlowValidationMessage& Message : Messages)
        {
            const int32 Priority = GetValidationPriority(Message.Severity);
            if (Priority > HighestPriority)
            {
                HighestPriority = Priority;
                HighestSeverity = Message.Severity;
            }
        }

        return HighestSeverity;
    }

    static FSlateColor GetValidationColor(const EDreamFlowValidationSeverity Severity)
    {
        switch (Severity)
        {
        case EDreamFlowValidationSeverity::Error:
            return FSlateColor(EStyleColor::Error);

        case EDreamFlowValidationSeverity::Warning:
            return FSlateColor(EStyleColor::Warning);

        case EDreamFlowValidationSeverity::Info:
        default:
            return FSlateColor(EStyleColor::Primary);
        }
    }

    static FText GetValidationLabel(const EDreamFlowValidationSeverity Severity)
    {
        switch (Severity)
        {
        case EDreamFlowValidationSeverity::Error:
            return FText::FromString(TEXT("Error"));

        case EDreamFlowValidationSeverity::Warning:
            return FText::FromString(TEXT("Warning"));

        case EDreamFlowValidationSeverity::Info:
        default:
            return FText::FromString(TEXT("Info"));
        }
    }
}

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
    NodeIconBrush.Reset();
    RefreshInlinePropertyRows();

    this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

    TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

    GetOrAddSlot(ENodeZone::Center)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Center)
        [
            SNew(SBorder)
            .BorderImage(this, &SGraphNode_DreamFlow::GetOuterNodeBrush)
            .BorderBackgroundColor(this, &SGraphNode_DreamFlow::GetNodeOutlineColor)
            .ToolTipText(this, &SGraphNode_DreamFlow::GetValidationToolTipText)
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
                    [
                        SNew(SBorder)
                        .BorderImage(this, &SGraphNode_DreamFlow::GetInnerNodeBrush)
                        .BorderBackgroundColor(FSlateColor(EStyleColor::Header))
                        .Padding(FMargin(0.0f))
                        [
                            SNew(SVerticalBox)

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(FMargin(12.0f, 10.0f, 12.0f, 0.0f))
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
                            .Padding(FMargin(10.0f, 8.0f, 10.0f, 10.0f))
                            [
                                SNew(SHorizontalBox)

                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Top)
                                .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                                [
                                    SAssignNew(LeftNodeBox, SVerticalBox)
                                ]

                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
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
                                            .Visibility(this, &SGraphNode_DreamFlow::GetTransitionModeVisibility)
                                            .BorderImage(this, &SGraphNode_DreamFlow::GetBadgeBrush)
                                            .BorderBackgroundColor_Lambda([this]()
                                            {
                                                const UDreamFlowNode* RuntimeNode = GetFlowNode() != nullptr ? GetFlowNode()->GetRuntimeNode() : nullptr;
                                                return RuntimeNode != nullptr && RuntimeNode->GetTransitionMode() == EDreamFlowNodeTransitionMode::Automatic
                                                    ? FStyleColors::Primary
                                                    : FStyleColors::Recessed;
                                            })
                                            .Padding(FMargin(8.0f, 3.0f))
                                            [
                                                SNew(STextBlock)
                                                .Text(this, &SGraphNode_DreamFlow::GetTransitionModeLabel)
                                                .TextStyle(FAppStyle::Get(), "SmallText")
                                                .ColorAndOpacity(FSlateColor(EStyleColor::White))
                                            ]
                                        ]
                                    ]

                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                                    [
                                        SNew(SHorizontalBox)
                                        + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .VAlign(VAlign_Top)
                                        .Padding(0.0f, 2.0f, 8.0f, 0.0f)
                                        [
                                            SNew(SImage)
                                            .Visibility(this, &SGraphNode_DreamFlow::GetIconVisibility)
                                            .Image(this, &SGraphNode_DreamFlow::GetNodeIconBrush)
                                            .DesiredSizeOverride(FVector2D(18.0f, 18.0f))
                                            .ColorAndOpacity(FSlateColor(EStyleColor::Foreground))
                                        ]
                                        + SHorizontalBox::Slot()
                                        .FillWidth(1.0f)
                                        [
                                            SNew(SVerticalBox)
                                            + SVerticalBox::Slot()
                                            .AutoHeight()
                                            [
                                                SNew(STextBlock)
                                                .Text(this, &SGraphNode_DreamFlow::GetNodeClassLabel)
                                                .TextStyle(FAppStyle::Get(), "SmallText")
                                                .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetClassTextColor)
                                            ]
                                            + SVerticalBox::Slot()
                                            .AutoHeight()
                                            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                                            [
                                                SNew(STextBlock)
                                                .Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
                                                .TextStyle(FAppStyle::Get(), "Graph.Node.NodeTitle")
                                                .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetTitleTextColor)
                                                .WrapTextAt(220.0f)
                                            ]
                                        ]
                                        + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .VAlign(VAlign_Center)
                                        [
                                            SNew(SHorizontalBox)
                                            + SHorizontalBox::Slot()
                                            .AutoWidth()
                                            [
                                                SNew(SBorder)
                                                .Visibility_Lambda([this]()
                                                {
                                                    return DreamFlowNodeWidget::GetExecutionBadgeVisibility(GetFlowNode());
                                                })
                                                .BorderImage(this, &SGraphNode_DreamFlow::GetBadgeBrush)
                                                .BorderBackgroundColor_Lambda([this]()
                                                {
                                                    return DreamFlowNodeWidget::GetExecutionBadgeColor(GetFlowNode());
                                                })
                                                .Padding(FMargin(8.0f, 3.0f))
                                                [
                                                    SNew(STextBlock)
                                                    .Text_Lambda([this]()
                                                    {
                                                        return DreamFlowNodeWidget::GetExecutionBadgeLabel(GetFlowNode());
                                                    })
                                                    .TextStyle(FAppStyle::Get(), "SmallText")
                                                    .ColorAndOpacity(FSlateColor(EStyleColor::White))
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
                                            + SHorizontalBox::Slot()
                                            .AutoWidth()
                                            .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                                            [
                                                SNew(SBorder)
                                                .Visibility(this, &SGraphNode_DreamFlow::GetValidationBadgeVisibility)
                                                .BorderImage(this, &SGraphNode_DreamFlow::GetBadgeBrush)
                                                .BorderBackgroundColor(this, &SGraphNode_DreamFlow::GetValidationBadgeColor)
                                                .Padding(FMargin(8.0f, 3.0f))
                                                .ToolTipText(this, &SGraphNode_DreamFlow::GetValidationToolTipText)
                                                [
                                                    SNew(STextBlock)
                                                    .Text(this, &SGraphNode_DreamFlow::GetValidationBadgeText)
                                                    .TextStyle(FAppStyle::Get(), "SmallText")
                                                    .ColorAndOpacity(FSlateColor(EStyleColor::White))
                                                ]
                                            ]
                                        ]
                                    ]

                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                                    [
                                        SNew(STextBlock)
                                        .Text(this, &SGraphNode_DreamFlow::GetNodeDescription)
                                        .TextStyle(FAppStyle::Get(), "SmallText")
                                        .WrapTextAt(240.0f)
                                        .ColorAndOpacity(this, &SGraphNode_DreamFlow::GetMutedTextColor)
                                        .Visibility(this, &SGraphNode_DreamFlow::GetDescriptionVisibility)
                                    ]
                                ]

                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Top)
                                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                                [
                                    SAssignNew(RightNodeBox, SVerticalBox)
                                ]
                            ]
                        ]
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(10.0f, 0.0f, 10.0f, 10.0f))
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            BuildInlineEditorArea()
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                        [
                            BuildPreviewArea()
                        ]
                    ]
                ]
            ]
        ];

    CreatePinWidgets();
}

TSharedPtr<SGraphPin> SGraphNode_DreamFlow::CreatePinWidget(UEdGraphPin* Pin) const
{
    return SNew(SGraphPinExec, Pin);
}

void SGraphNode_DreamFlow::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
    PinToAdd->SetOwner(SharedThis(this));
    PinToAdd->SetShowLabel(true);

    if (PinToAdd->GetDirection() == EGPD_Input)
    {
        LeftNodeBox->AddSlot()
            .AutoHeight()
            .VAlign(VAlign_Top)
            .Padding(FMargin(0.0f, 0.0f, 4.0f, 4.0f))
            [
                PinToAdd
            ];
        InputPins.Add(PinToAdd);
    }
    else
    {
        RightNodeBox->AddSlot()
            .AutoHeight()
            .VAlign(VAlign_Top)
            .Padding(FMargin(4.0f, 0.0f, 0.0f, 4.0f))
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

void SGraphNode_DreamFlow::GetOverlayBrushes(bool bSelected, const FVector2f& WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
    SGraphNode::GetOverlayBrushes(bSelected, WidgetSize, Brushes);

    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode != nullptr ? FlowNode->GetRuntimeNode() : nullptr;
    const UDreamFlowAsset* FlowAsset = FlowNode != nullptr ? FDreamFlowEditorUtils::GetFlowAssetFromGraph(FlowNode->GetGraph()) : nullptr;
    if (RuntimeNode == nullptr || FlowAsset == nullptr || !RuntimeNode->NodeGuid.IsValid() || GEngine == nullptr)
    {
        return;
    }

    bool bIsBreakpointHit = false;
    const UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>();
    if (DebuggerSubsystem == nullptr || !DebuggerSubsystem->IsNodeCurrentExecutionLocation(FlowAsset, RuntimeNode->NodeGuid, bIsBreakpointHit))
    {
        return;
    }

    FOverlayBrushInfo InstructionPointerOverlay;
    InstructionPointerOverlay.Brush = FAppStyle::GetBrush(
        bIsBreakpointHit
            ? TEXT("Kismet.DebuggerOverlay.InstructionPointerBreakpoint")
            : TEXT("Kismet.DebuggerOverlay.InstructionPointer"));

    if (InstructionPointerOverlay.Brush != nullptr)
    {
        const float Overlap = 10.0f;
        InstructionPointerOverlay.OverlayOffset.X = (WidgetSize.X / 2.0f) - (InstructionPointerOverlay.Brush->ImageSize.X / 2.0f);
        InstructionPointerOverlay.OverlayOffset.Y = Overlap - InstructionPointerOverlay.Brush->ImageSize.Y;
    }

    InstructionPointerOverlay.AnimationEnvelope = FVector2f(0.0f, 10.0f);
    Brushes.Add(InstructionPointerOverlay);
}

void SGraphNode_DreamFlow::RefreshInlinePropertyRows()
{
    InlinePropertyRowGenerator.Reset();

    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    UDreamFlowNode* RuntimeNode = FlowNode != nullptr ? FlowNode->GetRuntimeNode() : nullptr;
    if (RuntimeNode == nullptr)
    {
        return;
    }

    const TArray<FName> InlinePropertyNames = RuntimeNode->GetInlineEditablePropertyNames();
    if (InlinePropertyNames.Num() == 0)
    {
        return;
    }

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FPropertyRowGeneratorArgs GeneratorArgs;
    GeneratorArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
    InlinePropertyRowGenerator = PropertyEditorModule.CreatePropertyRowGenerator(GeneratorArgs);

    TSet<FString> AllowedPropertyPaths;
    for (const FName PropertyName : InlinePropertyNames)
    {
        if (!PropertyName.IsNone())
        {
            AllowedPropertyPaths.Add(PropertyName.ToString());
        }
    }

    InlinePropertyRowGenerator->SetPropertyGenerationAllowListPaths(AllowedPropertyPaths);
    InlinePropertyRowGenerator->SetObjects({ RuntimeNode });
}

void SGraphNode_DreamFlow::CollectDetailNodesRecursive(const TArray<TSharedRef<IDetailTreeNode>>& Nodes, TMap<FName, TSharedRef<IDetailTreeNode>>& OutNodes) const
{
    for (const TSharedRef<IDetailTreeNode>& Node : Nodes)
    {
        if (const TSharedPtr<IPropertyHandle> PropertyHandle = Node->CreatePropertyHandle())
        {
            if (const FProperty* Property = PropertyHandle->GetProperty())
            {
                OutNodes.FindOrAdd(Property->GetFName(), Node);
            }
        }

        TArray<TSharedRef<IDetailTreeNode>> ChildNodes;
        Node->GetChildren(ChildNodes, false);
        if (ChildNodes.Num() > 0)
        {
            CollectDetailNodesRecursive(ChildNodes, OutNodes);
        }
    }
}

TArray<TSharedRef<IDetailTreeNode>> SGraphNode_DreamFlow::GetInlinePropertyNodes() const
{
    TArray<TSharedRef<IDetailTreeNode>> Result;

    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode != nullptr ? FlowNode->GetRuntimeNode() : nullptr;
    if (RuntimeNode == nullptr || !InlinePropertyRowGenerator.IsValid())
    {
        return Result;
    }

    TMap<FName, TSharedRef<IDetailTreeNode>> NodesByName;
    CollectDetailNodesRecursive(InlinePropertyRowGenerator->GetRootTreeNodes(), NodesByName);

    TArray<FName> PropertyNames = RuntimeNode->GetInlineEditablePropertyNames();
    PropertyNames.RemoveAll([](const FName PropertyName)
    {
        return PropertyName.IsNone();
    });

    PropertyNames.Sort([RuntimeNode](const FName& A, const FName& B)
    {
        const int32 PriorityA = DreamFlowNodeWidget::GetInlinePropertyPriority(RuntimeNode, A);
        const int32 PriorityB = DreamFlowNodeWidget::GetInlinePropertyPriority(RuntimeNode, B);
        return PriorityA == PriorityB ? A.LexicalLess(B) : PriorityA < PriorityB;
    });

    for (const FName PropertyName : PropertyNames)
    {
        if (const TSharedRef<IDetailTreeNode>* FoundNode = NodesByName.Find(PropertyName))
        {
            Result.Add(*FoundNode);
        }
    }

    return Result;
}

TSharedRef<SWidget> SGraphNode_DreamFlow::BuildInlineEditorArea()
{
    const TArray<TSharedRef<IDetailTreeNode>> InlineNodes = GetInlinePropertyNodes();
    TSharedRef<SVerticalBox> InlineEditorBox = SNew(SVerticalBox);

    for (const TSharedRef<IDetailTreeNode>& DetailNode : InlineNodes)
    {
        InlineEditorBox->AddSlot()
            .AutoHeight()
            .Padding(FMargin(0.0f, 0.0f, 0.0f, 6.0f))
            [
                BuildInlinePropertyWidget(DetailNode)
            ];
    }

    return SNew(SBox)
        .Visibility(this, &SGraphNode_DreamFlow::GetInlineEditorVisibility)
        [
            InlineEditorBox
        ];
}

TSharedRef<SWidget> SGraphNode_DreamFlow::BuildInlinePropertyWidget(const TSharedRef<IDetailTreeNode>& DetailNode) const
{
    const FNodeWidgets NodeWidgets = DetailNode->CreateNodeWidgets();

    if (NodeWidgets.WholeRowWidget.IsValid())
    {
        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FSlateColor(EStyleColor::Panel))
            .Padding(FMargin(8.0f, 6.0f))
            [
                NodeWidgets.WholeRowWidget.ToSharedRef()
            ];
    }

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FSlateColor(EStyleColor::Panel))
        .Padding(FMargin(8.0f, 5.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(0.42f)
            .VAlign(VAlign_Center)
            [
                NodeWidgets.NameWidget.IsValid()
                    ? NodeWidgets.NameWidget.ToSharedRef()
                    : SNullWidget::NullWidget
            ]
            + SHorizontalBox::Slot()
            .FillWidth(0.58f)
            .Padding(8.0f, 0.0f, 0.0f, 0.0f)
            .VAlign(VAlign_Center)
            [
                NodeWidgets.ValueWidget.IsValid()
                    ? NodeWidgets.ValueWidget.ToSharedRef()
                    : SNullWidget::NullWidget
            ]
        ];
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
        .BorderBackgroundColor(FStyleColors::Header)
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

TArray<FDreamFlowValidationMessage> SGraphNode_DreamFlow::GetValidationMessages() const
{
    TArray<FDreamFlowValidationMessage> Messages;

    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode != nullptr ? FlowNode->GetRuntimeNode() : nullptr;
    if (FlowNode == nullptr || RuntimeNode == nullptr || !RuntimeNode->NodeGuid.IsValid())
    {
        return Messages;
    }

    FDreamFlowEditorToolkit::GetValidationMessagesForGraphNode(FlowNode->GetGraph(), RuntimeNode->NodeGuid, Messages);
    return Messages;
}

FText SGraphNode_DreamFlow::GetValidationBadgeText() const
{
    const TArray<FDreamFlowValidationMessage> Messages = GetValidationMessages();
    if (Messages.Num() == 0)
    {
        return FText::GetEmpty();
    }

    const EDreamFlowValidationSeverity HighestSeverity = DreamFlowNodeWidget::GetHighestValidationSeverity(Messages);
    return Messages.Num() == 1
        ? DreamFlowNodeWidget::GetValidationLabel(HighestSeverity)
        : FText::Format(FText::FromString(TEXT("{0} Issues")), FText::AsNumber(Messages.Num()));
}

FText SGraphNode_DreamFlow::GetValidationToolTipText() const
{
    const TArray<FDreamFlowValidationMessage> Messages = GetValidationMessages();
    if (Messages.Num() == 0)
    {
        return FText::GetEmpty();
    }

    FString ToolTipText;
    for (int32 Index = 0; Index < Messages.Num(); ++Index)
    {
        const FDreamFlowValidationMessage& Message = Messages[Index];
        if (!ToolTipText.IsEmpty())
        {
            ToolTipText += TEXT("\n");
        }

        ToolTipText += FString::Printf(
            TEXT("[%s] %s"),
            *DreamFlowNodeWidget::GetValidationLabel(Message.Severity).ToString(),
            *Message.Message.ToString());
    }

    return FText::FromString(ToolTipText);
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

const FSlateBrush* SGraphNode_DreamFlow::GetNodeIconBrush() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode != nullptr ? FlowNode->GetRuntimeNode() : nullptr;
    return DreamFlowNodeWidget::ResolveNodeIconBrush(RuntimeNode, NodeIconBrush);
}

FSlateColor SGraphNode_DreamFlow::GetNodeOutlineColor() const
{
    bool bIsBreakpointHit = false;
    if (DreamFlowNodeWidget::IsExecutionFocused(GetFlowNode(), bIsBreakpointHit))
    {
        return bIsBreakpointHit
            ? FSlateColor(EStyleColor::Error)
            : FSlateColor(EStyleColor::Primary);
    }

    if (IsSelectedExclusively())
    {
        return FSlateColor(EStyleColor::Primary);
    }

    const TArray<FDreamFlowValidationMessage> ValidationMessages = GetValidationMessages();
    if (ValidationMessages.Num() > 0)
    {
        return DreamFlowNodeWidget::GetValidationColor(DreamFlowNodeWidget::GetHighestValidationSeverity(ValidationMessages));
    }

    if (IsHovered())
    {
        return FSlateColor(EStyleColor::Hover);
    }

    return FSlateColor(EStyleColor::InputOutline);
}

FSlateColor SGraphNode_DreamFlow::GetNodeSurfaceColor() const
{
    bool bIsBreakpointHit = false;
    if (DreamFlowNodeWidget::IsExecutionFocused(GetFlowNode(), bIsBreakpointHit))
    {
        return bIsBreakpointHit
            ? FSlateColor(EStyleColor::Panel)
            : FSlateColor(EStyleColor::Recessed);
    }

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

FSlateColor SGraphNode_DreamFlow::GetValidationBadgeColor() const
{
    const TArray<FDreamFlowValidationMessage> ValidationMessages = GetValidationMessages();
    return ValidationMessages.Num() > 0
        ? DreamFlowNodeWidget::GetValidationColor(DreamFlowNodeWidget::GetHighestValidationSeverity(ValidationMessages))
        : FSlateColor(EStyleColor::Primary);
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

FText SGraphNode_DreamFlow::GetTransitionModeLabel() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    return RuntimeNode ? RuntimeNode->GetTransitionModeLabel() : FText::GetEmpty();
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

    const int32 OutputPinCount = RuntimeNode->GetOutputPins().Num();
    return FText::Format(
        FText::FromString(TEXT("Pins: {0} output{1}  |  Parent rule: {2}")),
        FText::AsNumber(OutputPinCount),
        OutputPinCount == 1 ? FText::GetEmpty() : FText::FromString(TEXT("s")),
        RuntimeNode->SupportsMultipleParents() ? FText::FromString(TEXT("multiple")) : FText::FromString(TEXT("single")));
}

FText SGraphNode_DreamFlow::GetNodeDescription() const
{
    const UDreamFlowEdGraphNode* FlowNode = GetFlowNode();
    const UDreamFlowNode* RuntimeNode = FlowNode ? FlowNode->GetRuntimeNode() : nullptr;
    return RuntimeNode ? RuntimeNode->Description : FText::GetEmpty();
}

EVisibility SGraphNode_DreamFlow::GetIconVisibility() const
{
    return GetNodeIconBrush() != nullptr
        ? EVisibility::Visible
        : EVisibility::Collapsed;
}

EVisibility SGraphNode_DreamFlow::GetInlineEditorVisibility() const
{
    return GetInlinePropertyNodes().Num() > 0
        ? EVisibility::Visible
        : EVisibility::Collapsed;
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

EVisibility SGraphNode_DreamFlow::GetTransitionModeVisibility() const
{
    return GetTransitionModeLabel().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SGraphNode_DreamFlow::GetDescriptionVisibility() const
{
    return GetNodeDescription().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SGraphNode_DreamFlow::GetValidationBadgeVisibility() const
{
    return GetValidationMessages().Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

UDreamFlowEdGraphNode* SGraphNode_DreamFlow::GetFlowNode() const
{
    return Cast<UDreamFlowEdGraphNode>(GraphNode);
}
