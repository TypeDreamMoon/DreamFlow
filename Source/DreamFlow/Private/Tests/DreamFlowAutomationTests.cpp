#include "DreamFlowAsset.h"
#include "DreamFlowAsyncNode.h"
#include "DreamFlowBlueprintLibrary.h"
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

    struct FDreamFlowDelayTestGraph
    {
        UDreamFlowAsset* Asset = nullptr;
        UDreamFlowEntryNode* EntryNode = nullptr;
        UDreamFlowDelayNode* DelayNode = nullptr;
        UDreamFlowNode* CompletedNode = nullptr;
    };

    struct FDreamFlowFinishTestGraph
    {
        UDreamFlowAsset* Asset = nullptr;
        UDreamFlowEntryNode* EntryNode = nullptr;
        UDreamFlowFinishFlowNode* FinishNode = nullptr;
        UDreamFlowNode* IgnoredNode = nullptr;
    };

    static FDreamFlowValue MakeBoolValue(const bool bValue)
    {
        FDreamFlowValue Value;
        Value.Type = EDreamFlowValueType::Bool;
        Value.BoolValue = bValue;
        return Value;
    }

    static FDreamFlowValue MakeFloatValue(const float InValue)
    {
        FDreamFlowValue Value;
        Value.Type = EDreamFlowValueType::Float;
        Value.FloatValue = InValue;
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

    static FDreamFlowFinishTestGraph BuildFinishFlowAsset()
    {
        FDreamFlowFinishTestGraph Graph;
        Graph.Asset = NewObject<UDreamFlowAsset>(GetTransientPackage(), NAME_None, RF_Transient);

        Graph.EntryNode = NewObject<UDreamFlowEntryNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.FinishNode = NewObject<UDreamFlowFinishFlowNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.IgnoredNode = NewObject<UDreamFlowNode>(Graph.Asset, NAME_None, RF_Transient);

        Graph.IgnoredNode->Title = FText::FromString(TEXT("Ignored After Finish"));

        Graph.EntryNode->SetOutputLinks({ MakeOutputLink(TEXT("Out"), Graph.FinishNode) });
        Graph.FinishNode->SetOutputLinks({ MakeOutputLink(TEXT("Out"), Graph.IgnoredNode) });

        Graph.Asset->ReplaceNodes({
            Graph.EntryNode,
            Graph.FinishNode,
            Graph.IgnoredNode
        });
        Graph.Asset->SetEntryNodeInternal(Graph.EntryNode);

        return Graph;
    }

    static FDreamFlowDelayTestGraph BuildDelayFlowAsset()
    {
        FDreamFlowDelayTestGraph Graph;
        Graph.Asset = NewObject<UDreamFlowAsset>(GetTransientPackage(), NAME_None, RF_Transient);

        FDreamFlowVariableDefinition DurationVariable;
        DurationVariable.Name = TEXT("DelayDuration");
        DurationVariable.Description = FText::FromString(TEXT("Controls the delay duration for the async delay node test."));
        DurationVariable.DefaultValue = MakeFloatValue(0.0f);
        Graph.Asset->Variables.Add(DurationVariable);

        Graph.EntryNode = NewObject<UDreamFlowEntryNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.DelayNode = NewObject<UDreamFlowDelayNode>(Graph.Asset, NAME_None, RF_Transient);
        Graph.CompletedNode = NewObject<UDreamFlowNode>(Graph.Asset, NAME_None, RF_Transient);

        Graph.CompletedNode->Title = FText::FromString(TEXT("After Delay"));
        Graph.DelayNode->DurationBinding.SourceType = EDreamFlowValueSourceType::FlowVariable;
        Graph.DelayNode->DurationBinding.VariableName = DurationVariable.Name;

        Graph.EntryNode->SetOutputLinks({ MakeOutputLink(TEXT("Out"), Graph.DelayNode) });
        Graph.DelayNode->SetOutputLinks({ MakeOutputLink(TEXT("Completed"), Graph.CompletedNode) });

        Graph.Asset->ReplaceNodes({
            Graph.EntryNode,
            Graph.DelayNode,
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
    TestTrue(TEXT("The executor should report that it is waiting for manual advance."), Executor->IsWaitingForAdvance());
    TestFalse(TEXT("The manual branch node should not be treated as automatic."), Executor->IsCurrentNodeAutomatic());
    TestEqual(TEXT("Two output pins should be available on the manual branch node."), Executor->GetAvailableOutputPins().Num(), 2);
    TestFalse(TEXT("Advance should stay blocked when the manual node exposes multiple outputs."), Executor->Advance());
    TestTrue(TEXT("MoveToOutputPin should allow choosing the true branch explicitly."), Executor->MoveToOutputPin(TEXT("True")));
    TestEqual(TEXT("Advancing through the true output pin should enter the true node."), Executor->GetCurrentNode(), Graph.TrueNode);

    UDreamFlowExecutor* NodeDrivenExecutor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    NodeDrivenExecutor->Initialize(Graph.Asset, nullptr);
    TestTrue(TEXT("A second executor should also start successfully."), NodeDrivenExecutor->StartFlow());
    TestTrue(TEXT("Node helper flow continuation should advance through the requested output pin."), Graph.BranchNode->ContinueFlowFromOutputPin(NodeDrivenExecutor, TEXT("False")));
    TestEqual(TEXT("Node helper output continuation should enter the false node."), NodeDrivenExecutor->GetCurrentNode(), Graph.FalseNode);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowFinishNodeStopsExecutionTest,
    "DreamFlow.Core.Execution.FinishFlowNodeStopsExecution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowFinishNodeStopsExecutionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowFinishTestGraph Graph = BuildFinishFlowAsset();
    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    Executor->Initialize(Graph.Asset, nullptr);

    TestTrue(TEXT("Finish flow test graph should start successfully."), Executor->StartFlow());
    TestFalse(TEXT("Finish Flow node should stop execution immediately."), Executor->IsRunning());
    TestEqual(TEXT("Executor should report the finished debug state after the finish node runs."), Executor->GetDebugState(), EDreamFlowExecutorDebugState::Finished);
    TestNull(TEXT("Current node should be cleared after FinishFlow executes."), Executor->GetCurrentNode());

    const TArray<UDreamFlowNode*> VisitedNodes = Executor->GetVisitedNodes();
    TestEqual(TEXT("Finish flow test should only visit the entry node and finish node."), VisitedNodes.Num(), 2);
    TestTrue(TEXT("Visited nodes should include the finish node."), VisitedNodes.Contains(Graph.FinishNode));
    TestFalse(TEXT("Nodes after Finish Flow should never execute."), VisitedNodes.Contains(Graph.IgnoredNode));

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
    TestEqual(TEXT("Component should expose the assigned flow asset."), ExecutorComponent->GetFlowAsset(), Graph.Asset);
    TestEqual(TEXT("Manual entry start should leave the executor on the entry node."), ExecutorComponent->GetCurrentNode(), static_cast<UDreamFlowNode*>(Graph.EntryNode));
    TestTrue(TEXT("Component should report that the flow is running after StartFlow."), ExecutorComponent->IsRunning());
    TestEqual(TEXT("Component debug state should mirror the running executor."), ExecutorComponent->GetDebugState(), EDreamFlowExecutorDebugState::Running);
    TestEqual(TEXT("Component should expose entry-node children while waiting for manual continuation."), ExecutorComponent->GetAvailableChildren().Num(), 1);
    TestEqual(TEXT("Component should expose the visited entry node."), ExecutorComponent->GetVisitedNodes().Num(), 1);

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
    FDreamFlowBlueprintLibraryHelpersTest,
    "DreamFlow.Core.Blueprint.LibraryHelpers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowBlueprintLibraryHelpersTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowBranchTestGraph Graph = BuildBranchingFlowAsset(false);
    UDreamFlowExecutor* Executor = UDreamFlowBlueprintLibrary::CreateFlowExecutor(GetTransientPackage(), Graph.Asset, nullptr, UDreamFlowExecutor::StaticClass());
    TestNotNull(TEXT("Blueprint library should be able to create an initialized executor."), Executor);
    TestEqual(TEXT("Created executor should point at the requested flow asset."), Executor != nullptr ? Executor->GetFlowAsset() : nullptr, Graph.Asset);

    FDreamFlowVariableDefinition VariableDefinition;
    TestTrue(TEXT("Blueprint library should find declared flow variables."), UDreamFlowBlueprintLibrary::FindFlowVariableDefinition(Graph.Asset, TEXT("CanContinue"), VariableDefinition));
    TestEqual(TEXT("The found variable definition should match the declared name."), VariableDefinition.Name, FName(TEXT("CanContinue")));

    TestTrue(TEXT("Blueprint library should report node compatibility against the owning asset."), UDreamFlowBlueprintLibrary::NodeSupportsFlowAsset(Graph.BranchNode, Graph.Asset));
    TestEqual(TEXT("Blueprint library should expose node output links."), UDreamFlowBlueprintLibrary::GetNodeOutputLinks(Graph.BranchNode).Num(), 2);

    const FDreamFlowValue IntValue = UDreamFlowBlueprintLibrary::MakeIntFlowValue(7);
    FDreamFlowValue FloatValue;
    TestTrue(TEXT("Blueprint library should convert low-level flow values."), UDreamFlowBlueprintLibrary::ConvertFlowValue(IntValue, EDreamFlowValueType::Float, FloatValue));
    TestEqual(TEXT("Converted flow values should preserve the numeric value."), FloatValue.FloatValue, 7.0f);
    TestEqual(TEXT("Blueprint library should describe low-level values."), UDreamFlowBlueprintLibrary::DescribeFlowValue(IntValue), FString(TEXT("7")));

    bool bComparisonResult = false;
    TestTrue(TEXT("Blueprint library should compare low-level flow values."), UDreamFlowBlueprintLibrary::CompareFlowValues(IntValue, UDreamFlowBlueprintLibrary::MakeIntFlowValue(7), EDreamFlowComparisonOperation::Equal, bComparisonResult));
    TestTrue(TEXT("The comparison result should report equality."), bComparisonResult);

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
    FDreamFlowDelayBindingTest,
    "DreamFlow.Core.Execution.DelayBinding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowDelayBindingTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowDelayTestGraph Graph = BuildDelayFlowAsset();
    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    Executor->Initialize(Graph.Asset, nullptr);

    TestTrue(TEXT("Delay test flow should start successfully."), Executor->StartFlow());
    TestFalse(TEXT("A zero-duration delay binding should complete immediately without leaving the executor in waiting mode."), Executor->IsWaitingForAsyncNode());
    TestEqual(TEXT("Delay binding should allow the flow to continue to the completed node immediately."), Executor->GetCurrentNode(), Graph.CompletedNode);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowBindingBlueprintHelpersTest,
    "DreamFlow.Core.Display.BindingBlueprintHelpers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowBindingBlueprintHelpersTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowValue LiteralValue = MakeBoolValue(true);

    const FDreamFlowValueBinding LiteralBinding = UDreamFlowBlueprintLibrary::MakeLiteralFlowBinding(LiteralValue);
    TestTrue(TEXT("Literal binding helper should mark the binding as literal."), UDreamFlowBlueprintLibrary::IsLiteralFlowBinding(LiteralBinding));
    TestFalse(TEXT("Literal binding helper should not mark the binding as variable-driven."), UDreamFlowBlueprintLibrary::IsVariableFlowBinding(LiteralBinding));
    TestEqual(TEXT("Literal binding helper should preserve the literal value."), UDreamFlowBlueprintLibrary::GetFlowBindingLiteralValue(LiteralBinding).BoolValue, true);

    const FDreamFlowValueBinding VariableBinding = UDreamFlowBlueprintLibrary::MakeVariableFlowBinding(TEXT("QuestState"));
    TestTrue(TEXT("Variable binding helper should mark the binding as variable-driven."), UDreamFlowBlueprintLibrary::IsVariableFlowBinding(VariableBinding));
    TestEqual(TEXT("Variable binding helper should preserve the variable name."), UDreamFlowBlueprintLibrary::GetFlowBindingVariableName(VariableBinding), FName(TEXT("QuestState")));

    FDreamFlowValueBinding UpdatedBinding = UDreamFlowBlueprintLibrary::SetFlowBindingVariableName(VariableBinding, TEXT("StoryState"));
    TestEqual(TEXT("Binding variable setter should update the variable name."), UpdatedBinding.VariableName, FName(TEXT("StoryState")));

    UpdatedBinding = UDreamFlowBlueprintLibrary::SetFlowBindingAsLiteral(UpdatedBinding, UDreamFlowBlueprintLibrary::MakeIntFlowValue(12));
    TestTrue(TEXT("Binding mode setter should switch the binding back to literal mode."), UDreamFlowBlueprintLibrary::IsLiteralFlowBinding(UpdatedBinding));
    TestEqual(TEXT("Binding literal setter should update the literal payload."), UpdatedBinding.LiteralValue.IntValue, 12);
    TestTrue(TEXT("Binding source type getter should reflect the current mode."), UDreamFlowBlueprintLibrary::GetFlowBindingSourceType(UpdatedBinding) == EDreamFlowValueSourceType::Literal);

    UpdatedBinding = UDreamFlowBlueprintLibrary::SetFlowBindingAsVariable(UpdatedBinding, TEXT("FinalState"));
    TestTrue(TEXT("Binding mode setter should switch the binding to variable mode."), UDreamFlowBlueprintLibrary::IsVariableFlowBinding(UpdatedBinding));
    TestEqual(TEXT("Binding mode setter should update the variable name."), UpdatedBinding.VariableName, FName(TEXT("FinalState")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowBindingRuntimeReadWriteTest,
    "DreamFlow.Core.Execution.BindingRuntimeReadWrite",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowBindingRuntimeReadWriteTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowBranchTestGraph Graph = BuildBranchingFlowAsset(false);
    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(GetTransientPackage(), NAME_None, RF_Transient);
    Executor->Initialize(Graph.Asset, nullptr);

    const FDreamFlowValueBinding VariableBinding = UDreamFlowBlueprintLibrary::MakeVariableFlowBinding(TEXT("CanContinue"));
    const FDreamFlowValueBinding LiteralBinding = UDreamFlowBlueprintLibrary::MakeLiteralFlowBinding(MakeBoolValue(false));

    FName BoundVariableName = NAME_None;
    bool bResolvedBoolValue = true;

    TestTrue(TEXT("Variable bindings should expose their writable target variable name."), Executor->GetBindingVariableName(VariableBinding, BoundVariableName));
    TestEqual(TEXT("Binding target variable name should match the selected flow variable."), BoundVariableName, FName(TEXT("CanContinue")));
    TestTrue(TEXT("Variable-backed bindings should report that they are writable."), Executor->CanWriteBindingValue(VariableBinding));
    TestFalse(TEXT("Literal bindings should report that they are not writable."), Executor->CanWriteBindingValue(LiteralBinding));

    TestTrue(TEXT("Bindings should resolve the current runtime variable value."), Executor->GetBindingBoolValue(VariableBinding, bResolvedBoolValue));
    TestFalse(TEXT("The bound variable should start at its default value before mutation."), bResolvedBoolValue);

    TestTrue(TEXT("Binding writes should update the underlying runtime variable."), Executor->SetBindingBoolValue(VariableBinding, true));
    TestTrue(TEXT("Typed binding reads should return the updated value after a write."), Executor->GetBindingBoolValue(VariableBinding, bResolvedBoolValue));
    TestTrue(TEXT("The bound variable should reflect the new value after writing through the binding."), bResolvedBoolValue);

    FDreamFlowValue RawBindingValue;
    TestTrue(TEXT("Low-level blueprint helpers should resolve binding values through an executor."), UDreamFlowBlueprintLibrary::GetExecutorBindingValue(Executor, VariableBinding, RawBindingValue));
    TestTrue(TEXT("Low-level resolved binding values should expose the updated payload."), RawBindingValue.BoolValue);
    TestTrue(TEXT("Blueprint helpers should report variable-backed bindings as writable."), UDreamFlowBlueprintLibrary::CanExecutorWriteBinding(Executor, VariableBinding));
    TestFalse(TEXT("Blueprint helpers should still reject literal bindings as writable."), UDreamFlowBlueprintLibrary::CanExecutorWriteBinding(Executor, LiteralBinding));
    TestTrue(TEXT("Blueprint helpers should support writing runtime values through a binding."), UDreamFlowBlueprintLibrary::SetExecutorBindingBoolValue(Executor, VariableBinding, false));
    TestTrue(TEXT("Binding reads should continue to work after helper-driven writes."), Executor->GetBindingBoolValue(VariableBinding, bResolvedBoolValue));
    TestFalse(TEXT("Helper-driven writes should update the runtime variable value."), bResolvedBoolValue);

    TestFalse(TEXT("Attempting to write through a literal binding should fail safely."), Executor->SetBindingBoolValue(LiteralBinding, true));
    TestTrue(TEXT("Literal bindings should remain readable as literal values."), UDreamFlowBlueprintLibrary::GetExecutorBindingBoolValue(Executor, LiteralBinding, bResolvedBoolValue));
    TestFalse(TEXT("Literal binding reads should continue to return their literal payload."), bResolvedBoolValue);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDreamFlowCrossAccessTest,
    "DreamFlow.Core.Execution.CrossAccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDreamFlowCrossAccessTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FDreamFlowBranchTestGraph Graph = BuildBranchingFlowAsset(false);

    UDreamFlowExecutor* Executor = Graph.Asset->CreateExecutor(nullptr, UDreamFlowExecutor::StaticClass());
    TestNotNull(TEXT("Flow assets should be able to create executors directly."), Executor);
    TestEqual(TEXT("Created executor should point back to its flow asset."), Executor != nullptr ? Executor->GetFlowAsset() : nullptr, Graph.Asset);
    TestEqual(TEXT("Executors should expose the flow entry node directly."), Executor != nullptr ? Executor->GetEntryNode() : nullptr, static_cast<UDreamFlowNode*>(Graph.EntryNode));
    TestTrue(TEXT("Executors should report that asset-owned nodes belong to the flow."), Executor != nullptr && Executor->OwnsNode(Graph.BranchNode));
    TestEqual(TEXT("Executors should be able to resolve nodes by GUID through the flow asset."), Executor != nullptr ? Executor->FindNodeByGuid(Graph.BranchNode->NodeGuid) : nullptr, static_cast<UDreamFlowNode*>(Graph.BranchNode));
    TestEqual(TEXT("Executors should expose the full node list through the owning asset."), Executor != nullptr ? Executor->GetNodes().Num() : 0, Graph.Asset->GetNodesCopy().Num());

    TestEqual(TEXT("Nodes should expose their owning flow asset."), Graph.BranchNode->GetOwningFlowAsset(), Graph.Asset);
    TestTrue(TEXT("Flow assets should report ownership for nodes in their graph."), Graph.Asset->OwnsNode(Graph.BranchNode));

    TestEqual(TEXT("No active executors should be registered before the flow starts."), Graph.Asset->GetActiveExecutors().Num(), 0);
    TestTrue(TEXT("The created executor should be able to start the flow."), Executor != nullptr && Executor->StartFlow());

    TArray<UDreamFlowExecutor*> AssetExecutors = Graph.Asset->GetActiveExecutors();
    TestTrue(TEXT("Flow assets should be able to find active executors for themselves."), AssetExecutors.Contains(Executor));

    TArray<UDreamFlowExecutor*> NodeAssetExecutors = Graph.EntryNode->GetActiveExecutors();
    TestTrue(TEXT("Nodes should be able to query active executors for their owning flow asset."), NodeAssetExecutors.Contains(Executor));

    TArray<UDreamFlowExecutor*> EntryExecutors = Graph.Asset->GetExecutorsOnNode(Graph.EntryNode);
    TestTrue(TEXT("Flow assets should be able to query executors currently stopped on a node."), EntryExecutors.Contains(Executor));

    TArray<UDreamFlowExecutor*> DirectNodeExecutors = Graph.EntryNode->GetExecutorsOnThisNode();
    TestTrue(TEXT("Nodes should be able to query executors currently stopped on themselves."), DirectNodeExecutors.Contains(Executor));

    TestEqual(TEXT("The manual-start flow should remain on the entry node after StartFlow."), Executor->GetCurrentNode(), static_cast<UDreamFlowNode*>(Graph.EntryNode));

    return true;
}

#endif
