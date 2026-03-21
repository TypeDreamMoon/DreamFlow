#include "DreamFlowAsset.h"
#include "DreamFlowCoreNodes.h"
#include "DreamFlowEntryNode.h"
#include "DreamFlowSettings.h"
#include "DreamFlowVariableTypes.h"
#include "Execution/DreamFlowExecutor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    struct FDreamFlowBranchTestGraph
    {
        UDreamFlowAsset* Asset = nullptr;
        UDreamFlowEntryNode* EntryNode = nullptr;
        UDreamFlowSetVariableNode* SetVariableNode = nullptr;
        UDreamFlowBranchNode* BranchNode = nullptr;
        UDreamFlowNode* TrueNode = nullptr;
        UDreamFlowNode* FalseNode = nullptr;
    };

    static FDreamFlowValue MakeBoolValue(const bool bValue)
    {
        FDreamFlowValue Value;
        Value.Type = EDreamFlowValueType::Bool;
        Value.BoolValue = bValue;
        return Value;
    }

    static FDreamFlowNodeOutputLink MakeOutputLink(const FName PinName, UDreamFlowNode* ChildNode)
    {
        FDreamFlowNodeOutputLink Link;
        Link.PinName = PinName;
        Link.Child = ChildNode;
        return Link;
    }

    static FDreamFlowBranchTestGraph BuildBranchingFlowAsset()
    {
        FDreamFlowBranchTestGraph Graph;
        Graph.Asset = NewObject<UDreamFlowAsset>(GetTransientPackage(), NAME_None, RF_Transient);

        FDreamFlowVariableDefinition ConditionVariable;
        ConditionVariable.Name = TEXT("CanContinue");
        ConditionVariable.Description = FText::FromString(TEXT("Used by the branching test flow."));
        ConditionVariable.DefaultValue = MakeBoolValue(false);
        Graph.Asset->Variables.Add(ConditionVariable);

        Graph.EntryNode = NewObject<UDreamFlowEntryNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.SetVariableNode = NewObject<UDreamFlowSetVariableNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.BranchNode = NewObject<UDreamFlowBranchNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.TrueNode = NewObject<UDreamFlowNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.FalseNode = NewObject<UDreamFlowNode>(Graph.Asset, NAME_None, RF_Transient);

        Graph.TrueNode->Title = FText::FromString(TEXT("True Path"));
        Graph.FalseNode->Title = FText::FromString(TEXT("False Path"));

        Graph.SetVariableNode->TargetVariable = ConditionVariable.Name;
        Graph.SetVariableNode->ValueBinding.SourceType = EDreamFlowValueSourceType::Literal;
        Graph.SetVariableNode->ValueBinding.LiteralValue = MakeBoolValue(true);

        Graph.BranchNode->ConditionBinding.SourceType = EDreamFlowValueSourceType::FlowVariable;
        Graph.BranchNode->ConditionBinding.VariableName = ConditionVariable.Name;

        Graph.EntryNode->SetOutputLinks({ MakeOutputLink(TEXT("Out"), Graph.SetVariableNode) });
        Graph.SetVariableNode->SetOutputLinks({ MakeOutputLink(TEXT("Out"), Graph.BranchNode) });
        Graph.BranchNode->SetOutputLinks({
            MakeOutputLink(TEXT("True"), Graph.TrueNode),
            MakeOutputLink(TEXT("False"), Graph.FalseNode)
        });

        Graph.Asset->ReplaceNodes({
            Graph.EntryNode,
            Graph.SetVariableNode,
            Graph.BranchNode,
            Graph.TrueNode,
            Graph.FalseNode
        });
        Graph.Asset->SetEntryNodeInternal(Graph.EntryNode);

        return Graph;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowAutomaticExecutionTest,
    "DreamFlow.Core.Execution.AutomaticBranching",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowAutomaticExecutionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowBranchTestGraph Graph = BuildBranchingFlowAsset();
    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);

    Executor->Initialize(Graph.Asset, nullptr);

    TestTrue(TEXT("StartFlow should succeed for a valid asset."), Executor->StartFlow());

    bool bRuntimeCondition = false;
    TestTrue(TEXT("Runtime condition variable should exist and resolve as true."), Executor->GetVariableBoolValue(TEXT("CanContinue"), bRuntimeCondition));
    TestTrue(TEXT("Set Variable node should update the runtime condition to true."), bRuntimeCondition);
    TestEqual(TEXT("Executor should land on the true branch node after automatic transitions."), Executor->GetCurrentNode(), Graph.TrueNode);

    const TArray<UDreamFlowNode*> VisitedNodes = Executor->GetVisitedNodes();
    TestEqual(TEXT("Executor should have visited four nodes."), VisitedNodes.Num(), 4);
    TestTrue(TEXT("Visited nodes should include the branch node."), VisitedNodes.Contains(Graph.BranchNode));
    TestTrue(TEXT("Visited nodes should include the true branch output node."), VisitedNodes.Contains(Graph.TrueNode));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowValidationMissingVariableTest,
    "DreamFlow.Core.Validation.MissingVariable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowValidationMissingVariableTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UDreamFlowAsset* Asset = NewObject<UDreamFlowAsset>(GetTransientPackage(), NAME_None, RF_Transient);
    UDreamFlowEntryNode* EntryNode = NewObject<UDreamFlowEntryNode>(Asset, NAME_None, RF_Transient);
    UDreamFlowSetVariableNode* SetVariableNode = NewObject<UDreamFlowSetVariableNode>(Asset, NAME_None, RF_Transient);
    UDreamFlowNode* EndNode = NewObject<UDreamFlowNode>(Asset, NAME_None, RF_Transient);

    SetVariableNode->TargetVariable = TEXT("MissingVariable");
    SetVariableNode->ValueBinding.SourceType = EDreamFlowValueSourceType::Literal;
    SetVariableNode->ValueBinding.LiteralValue = MakeBoolValue(true);

    EntryNode->SetOutputLinks({ MakeOutputLink(TEXT("Out"), SetVariableNode) });
    SetVariableNode->SetOutputLinks({ MakeOutputLink(TEXT("Out"), EndNode) });

    Asset->ReplaceNodes({ EntryNode, SetVariableNode, EndNode });
    Asset->SetEntryNodeInternal(EntryNode);

    TArray<FDreamFlowValidationMessage> ValidationMessages;
    Asset->ValidateFlow(ValidationMessages);

    bool bFoundMissingVariableError = false;
    for (const FDreamFlowValidationMessage& Message : ValidationMessages)
    {
        if (Message.Severity == EDreamFlowValidationSeverity::Error
            && Message.NodeGuid == SetVariableNode->NodeGuid
            && Message.Message.ToString().Contains(TEXT("missing flow variable"), ESearchCase::IgnoreCase))
        {
            bFoundMissingVariableError = true;
            break;
        }
    }

    TestTrue(TEXT("Validation should report an error when Set Variable targets an undefined flow variable."), bFoundMissingVariableError);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowReplicatedStateMirrorTest,
    "DreamFlow.Core.Network.ReplicatedStateMirror",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowReplicatedStateMirrorTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowBranchTestGraph Graph = BuildBranchingFlowAsset();
    UDreamFlowExecutor* AuthorityExecutor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    AuthorityExecutor->Initialize(Graph.Asset, nullptr);
    TestTrue(TEXT("Authority executor should start successfully."), AuthorityExecutor->StartFlow());

    FDreamFlowReplicatedExecutionState ReplicatedState;
    AuthorityExecutor->BuildReplicatedState(ReplicatedState);

    UDreamFlowExecutor* MirrorExecutor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    MirrorExecutor->InitializeReplicatedMirror(Graph.Asset, nullptr);
    MirrorExecutor->ApplyReplicatedState(ReplicatedState);

    bool bMirroredCondition = false;
    TestEqual(TEXT("Mirror executor should resolve the same current node as the authority executor."), MirrorExecutor->GetCurrentNode(), AuthorityExecutor->GetCurrentNode());
    TestEqual(TEXT("Mirror executor should have the same debug state as the authority executor."), MirrorExecutor->GetDebugState(), AuthorityExecutor->GetDebugState());
    TestTrue(TEXT("Mirror executor should inherit the runtime variable map."), MirrorExecutor->GetVariableBoolValue(TEXT("CanContinue"), bMirroredCondition));
    TestTrue(TEXT("Mirrored runtime condition should be true."), bMirroredCondition);
    TestEqual(TEXT("Visited node counts should match after replication snapshot application."), MirrorExecutor->GetVisitedNodes().Num(), AuthorityExecutor->GetVisitedNodes().Num());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowLogSettingsFilterTest,
    "DreamFlow.Core.Logging.SettingsFiltering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowLogSettingsFilterTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UDreamFlowSettings* Settings = NewObject<UDreamFlowSettings>(GetTransientPackage(), NAME_None, RF_Transient);
    Settings->bEnableLogging = true;
    Settings->LogVerbosity = EDreamFlowLogVerbositySetting::Warning;
    Settings->bLogExecution = false;
    Settings->bLogVariables = true;

    TestFalse(TEXT("Execution channel should not log when the channel is disabled."), Settings->ShouldLog(ELogVerbosity::Error, EDreamFlowLogChannel::Execution));
    TestTrue(TEXT("Variable channel should log warnings when warning verbosity is enabled."), Settings->ShouldLog(ELogVerbosity::Warning, EDreamFlowLogChannel::Variables));
    TestFalse(TEXT("Variable channel should not log plain Log messages when warning verbosity is selected."), Settings->ShouldLog(ELogVerbosity::Log, EDreamFlowLogChannel::Variables));
    TestFalse(TEXT("Variable channel should not log verbose messages when warning verbosity is selected."), Settings->ShouldLog(ELogVerbosity::Verbose, EDreamFlowLogChannel::Variables));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowBindingCompactDescriptionTest,
    "DreamFlow.Core.Display.BindingCompactDescription",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowBindingCompactDescriptionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FDreamFlowValue LiteralValue = MakeBoolValue(true);
    TestEqual(TEXT("Literal values should expose a short compact summary."), LiteralValue.DescribeCompact(), FString(TEXT("Type: Bool  Value: True")));

    FDreamFlowValueBinding VariableBinding;
    VariableBinding.SourceType = EDreamFlowValueSourceType::FlowVariable;
    VariableBinding.VariableName = TEXT("QuestState");
    TestEqual(TEXT("Variable bindings should expose a compact variable summary."), VariableBinding.DescribeCompact(), FString(TEXT("Type: Variable  Value: QuestState")));

    FDreamFlowValueBinding LiteralBinding;
    LiteralBinding.SourceType = EDreamFlowValueSourceType::Literal;
    LiteralBinding.LiteralValue = LiteralValue;
    TestEqual(TEXT("Literal bindings should reuse the compact value summary."), LiteralBinding.DescribeCompact(), FString(TEXT("Type: Bool  Value: True")));

    return true;
}

#endif
