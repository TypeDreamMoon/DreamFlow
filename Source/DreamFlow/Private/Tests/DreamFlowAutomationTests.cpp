#include "DreamFlowAsset.h"
#include "DreamFlowAsyncNode.h"
#include "DreamFlowCoreNodes.h"
#include "DreamFlowEntryNode.h"
#include "DreamFlowSettings.h"
#include "DreamFlowVariableTypes.h"
#include "Execution/DreamFlowExecutor.h"
#include "Execution/DreamFlowExecutorComponent.h"
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

    struct FDreamFlowAsyncTestGraph
    {
        UDreamFlowAsset* Asset = nullptr;
        UDreamFlowEntryNode* EntryNode = nullptr;
        UDreamFlowAsyncNode* AsyncNode = nullptr;
        UDreamFlowNode* CompletedNode = nullptr;
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

    static FDreamFlowBranchTestGraph BuildBranchingFlowAsset(const bool bAutoExecuteEntryNodeOnStart = true)
    {
        FDreamFlowBranchTestGraph Graph;
        Graph.Asset = NewObject<UDreamFlowAsset>(GetTransientPackage(), NAME_None, RF_Transient);
        Graph.Asset->bAutoExecuteEntryNodeOnStart = bAutoExecuteEntryNodeOnStart;

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

    static FDreamFlowAsyncTestGraph BuildAsyncFlowAsset()
    {
        FDreamFlowAsyncTestGraph Graph;
        Graph.Asset = NewObject<UDreamFlowAsset>(GetTransientPackage(), NAME_None, RF_Transient);

        Graph.EntryNode = NewObject<UDreamFlowEntryNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.AsyncNode = NewObject<UDreamFlowAsyncNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.CompletedNode = NewObject<UDreamFlowNode>(Graph.Asset, NAME_None, RF_Transient);

        Graph.AsyncNode->Title = FText::FromString(TEXT("Manual Async"));
        Graph.CompletedNode->Title = FText::FromString(TEXT("After Async"));

        Graph.EntryNode->SetOutputLinks({ MakeOutputLink(TEXT("Out"), Graph.AsyncNode) });
        Graph.AsyncNode->SetOutputLinks({ MakeOutputLink(TEXT("Completed"), Graph.CompletedNode) });

        Graph.Asset->ReplaceNodes({
            Graph.EntryNode,
            Graph.AsyncNode,
            Graph.CompletedNode
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
    FDreamFlowManualEntryStartTest,
    "DreamFlow.Core.Execution.ManualEntryStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowManualEntryStartTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowBranchTestGraph Graph = BuildBranchingFlowAsset(false);
    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);

    Executor->Initialize(Graph.Asset, nullptr);

    TestTrue(TEXT("StartFlow should still succeed when auto execution from the entry node is disabled."), Executor->StartFlow());
    TestEqual(TEXT("Executor should remain on the entry node until advanced manually."), Executor->GetCurrentNode(), static_cast<UDreamFlowNode*>(Graph.EntryNode));

    bool bRuntimeCondition = true;
    TestTrue(TEXT("Default variable values should still exist before manual advance."), Executor->GetVariableBoolValue(TEXT("CanContinue"), bRuntimeCondition));
    TestFalse(TEXT("The first gameplay node should not have executed yet."), bRuntimeCondition);
    TestEqual(TEXT("Only the entry node should have been visited after StartFlow."), Executor->GetVisitedNodes().Num(), 1);

    TestTrue(TEXT("Advance should execute the first gameplay node chain after the manual start mode enters the flow."), Executor->Advance());
    TestEqual(TEXT("Executor should eventually land on the true branch node after the manual advance."), Executor->GetCurrentNode(), Graph.TrueNode);
    TestTrue(TEXT("Manual advance should still update runtime variables."), Executor->GetVariableBoolValue(TEXT("CanContinue"), bRuntimeCondition));
    TestTrue(TEXT("The runtime condition should become true after the first gameplay node executes."), bRuntimeCondition);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowManualMultiOutputStepTest,
    "DreamFlow.Core.Execution.ManualMultiOutputStep",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowManualMultiOutputStepTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowBranchTestGraph Graph = BuildBranchingFlowAsset();
    Graph.BranchNode->TransitionMode = EDreamFlowNodeTransitionMode::Manual;

    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    Executor->Initialize(Graph.Asset, nullptr);

    TestTrue(TEXT("The flow should start successfully for a manual multi-output node."), Executor->StartFlow());
    TestEqual(TEXT("Execution should stop on the manual branch node instead of auto-transitioning."), Executor->GetCurrentNode(), static_cast<UDreamFlowNode*>(Graph.BranchNode));
    TestTrue(TEXT("The executor should report that it is waiting for a manual step."), Executor->IsWaitingForManualStep());
    TestFalse(TEXT("The manual branch node should not be treated as automatic."), Executor->IsCurrentNodeAutomatic());
    TestEqual(TEXT("Two output pins should be available on the manual branch node."), Executor->GetAvailableOutputPins().Num(), 2);
    TestFalse(TEXT("Default Step should stay blocked when the manual node exposes multiple outputs."), Executor->Step());
    TestTrue(TEXT("StepToOutputPin should allow choosing the true branch explicitly."), Executor->StepToOutputPin(TEXT("True")));
    TestEqual(TEXT("Stepping through the true output pin should enter the true node."), Executor->GetCurrentNode(), Graph.TrueNode);

    UDreamFlowExecutor* NodeDrivenExecutor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    NodeDrivenExecutor->Initialize(Graph.Asset, nullptr);
    TestTrue(TEXT("A second executor should also start successfully."), NodeDrivenExecutor->StartFlow());
    TestTrue(TEXT("Node helper flow continuation should step through the requested output pin."), Graph.BranchNode->ContinueFlowFromOutputPin(NodeDrivenExecutor, TEXT("False")));
    TestEqual(TEXT("Node helper output stepping should enter the false node."), NodeDrivenExecutor->GetCurrentNode(), Graph.FalseNode);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowComponentVariableMutationKeepsRuntimeStateTest,
    "DreamFlow.Core.Execution.ComponentVariableMutationKeepsRuntimeState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowComponentVariableMutationKeepsRuntimeStateTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowBranchTestGraph Graph = BuildBranchingFlowAsset(false);
    UDreamFlowExecutorComponent* ExecutorComponent = NewObject<UDreamFlowExecutorComponent>(GetTransientPackage(), NAME_None, RF_Transient);
    ExecutorComponent->FlowAsset = Graph.Asset;

    TestTrue(TEXT("Component-backed flow should start successfully."), ExecutorComponent->StartFlow());
    TestEqual(TEXT("Manual entry start should leave the executor on the entry node."), ExecutorComponent->GetCurrentNode(), static_cast<UDreamFlowNode*>(Graph.EntryNode));

    TestTrue(TEXT("Writing a variable at runtime should succeed without recreating the executor."), ExecutorComponent->SetVariableBoolValue(TEXT("CanContinue"), true));
    TestNotNull(TEXT("A runtime executor should still exist after mutating variables."), ExecutorComponent->GetExecutor());
    TestTrue(TEXT("Runtime variable mutation should keep the flow running."), ExecutorComponent->GetExecutor() != nullptr && ExecutorComponent->GetExecutor()->IsRunning());
    TestEqual(TEXT("Mutating variables should not discard the current node."), ExecutorComponent->GetCurrentNode(), static_cast<UDreamFlowNode*>(Graph.EntryNode));

    bool bConditionValue = false;
    TestTrue(TEXT("Updated runtime variables should remain readable after the write."), ExecutorComponent->GetVariableBoolValue(TEXT("CanContinue"), bConditionValue));
    TestTrue(TEXT("The runtime variable should keep the new value."), bConditionValue);

    TestTrue(TEXT("The flow should still be able to advance after mutating variables."), ExecutorComponent->Advance());
    TestEqual(TEXT("Advancing after the variable mutation should continue through the graph normally."), ExecutorComponent->GetCurrentNode(), Graph.TrueNode);

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
    FDreamFlowAsyncManualCompletionTest,
    "DreamFlow.Core.Execution.AsyncManualCompletion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowAsyncManualCompletionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowAsyncTestGraph Graph = BuildAsyncFlowAsset();
    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    Executor->Initialize(Graph.Asset, nullptr);

    TestTrue(TEXT("Async test flow should start successfully."), Executor->StartFlow());
    TestTrue(TEXT("Executor should remain running while an async node is pending."), Executor->IsRunning());
    TestTrue(TEXT("Executor should report that it is waiting for an async node."), Executor->IsWaitingForAsyncNode());
    TestEqual(TEXT("Waiting state should be exposed through the debug state."), Executor->GetDebugState(), EDreamFlowExecutorDebugState::Waiting);
    TestEqual(TEXT("Current node should be the pending async node."), Executor->GetCurrentNode(), static_cast<UDreamFlowNode*>(Graph.AsyncNode));
    TestEqual(TEXT("Pending async node should match the current async node."), Executor->GetPendingAsyncNode(), static_cast<UDreamFlowNode*>(Graph.AsyncNode));
    TestEqual(TEXT("Async nodes should hide manual child selection until they complete."), Executor->GetAvailableChildren().Num(), 0);
    TestFalse(TEXT("Advance should be blocked while the executor is waiting for async completion."), Executor->Advance());

    TestTrue(TEXT("Completing the async node should resume the flow."), Executor->CompleteAsyncNode());
    TestFalse(TEXT("Executor should no longer wait for async completion after completion."), Executor->IsWaitingForAsyncNode());
    TestEqual(TEXT("Flow should continue through the async node's completed output."), Executor->GetCurrentNode(), Graph.CompletedNode);

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
