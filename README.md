# DreamFlow

DreamFlow is the core Unreal flow graph plugin in the Dream plugin stack.

It provides a reusable runtime executor, a node-based editor, flow-scoped variables, Blueprint/C++ extensibility, debugging tools, and replication support for game-facing logic graphs.

The `DreamFlow` plugin is the foundation layer. Domain packs such as quest, dialogue, story, or encounter flows now live outside the core plugin.

## Modules

- `DreamFlow`: runtime graph asset, node, executor, variables, bindings, and networking support
- `DreamFlowEditor`: asset factory, asset editor toolkit, graph schema, debugger, validation panel, and custom Slate graph UI

## Core features

- `UDreamFlowAsset`: reusable flow graph asset
- `UDreamFlowNode`: extensible runtime node base class
- `UDreamFlowAsyncNode`: async-ready node base for Blueprint and C++ latent workflows
- `UDreamFlowEntryNode`: built-in entry node
- `UDreamFlowExecutor`: runtime flow runner with Blueprint events
- `UDreamFlowAsyncContext`: async completion handle passed into async nodes
- `UDreamFlowExecutorComponent`: Actor component wrapper with replication support
- Per-flow typed variables with defaults and runtime values
- Value binding support for literal, flow-variable, or execution-context-property driven node inputs
- Executor-scoped per-node runtime state keyed by `NodeGuid`
- Snapshot capture and restore support for runtime save/load style workflows
- Child-flow stack tracking and child executor access for nested flow execution
- Multi-output pins with named branches such as `True`, `False`, or `Completed`
- Inline node preview items for text, color, and image display
- Breakpoint-aware debugger tab for active executors
- Validation helpers and automation coverage for runtime/editor behavior
- Blueprint helper functions in `UDreamFlowBlueprintLibrary`
- Settings-driven logging through `UDreamFlowSettings` and `LogDreamFlow`

## What ships in the core plugin

The core plugin intentionally stays generic.

It includes:

- the `Dream Flow` asset type
- the standard DreamFlow editor
- generic flow execution/runtime systems
- generic core nodes such as `Branch`, `Compare`, `Set Variable`, `Delay`, and `Finish Flow`

It does not include domain-specific packs such as Quest or Dialogue anymore. Those have moved into separate plugins.

## Editor workflow

1. Enable the `DreamFlow` plugin.
2. In the Content Browser create a `Dream Flow` asset.
3. Open the asset to edit the graph.
4. Right click in the graph to add nodes.
5. Use `Choose Node Class...` to place Blueprint-derived DreamFlow node classes into the graph.
6. Use the left `Palette` tab to place common compatible node classes quickly.
7. Select a node to edit its runtime object in the right `Node Editor` tab.
8. Use the `Variables` tab to manage flow-scoped variables and defaults.
9. Use the `Validation` tab to inspect structural problems.
10. Use the `Debugger` tab during PIE/gameplay to inspect active executors, pause, continue, step, and stop.
11. Right click a node and choose `Add Breakpoint` to pause before that node executes.
12. Use the toolbar `Analyze References` button to inspect variable bindings, execution-context property bindings, sub-flow references, and node-class usage in the current asset.
13. The graph editor supports undo/redo, copy, cut, paste, duplicate, comments, and normal node selection workflows.

## Create Node Class

DreamFlow includes a fast way to create Blueprint node implementations.

You can create a new node class from either:

- Content Browser `Add New` -> `DreamFlow` -> `Create Node Class...`
- the DreamFlow editor toolbar button `Create Node Class` beside `Validate`

This opens a class picker for `UDreamFlowNode`-derived parent classes and creates a Blueprint asset in the current Content Browser path.

## Flow variables

Every `UDreamFlowAsset` owns a `Variables` array, so each flow can define its own environment-style data without relying on one global runtime object.

- Variable definitions are edited in the dedicated `Variables` tab.
- The variables tab supports type-first creation for `Bool`, `Int`, `Float`, `Name`, `String`, `Text`, `GameplayTag`, and `Object`.
- Each variable stores a `Name`, `Description`, and typed `DefaultValue`.
- When a `UDreamFlowExecutor` initializes, it copies those defaults into its runtime variable map.
- Each flow asset can choose whether `StartFlow` should immediately execute past the entry node through `bAutoExecuteEntryNodeOnStart`.

## Variable bindings

DreamFlow includes a lightweight binding struct, `FDreamFlowValueBinding`, for nodes that need data input.

- A binding can read from either a literal value, a named flow variable, or a dotted property path on the current execution context.
- The details UI exposes a source picker and variable dropdown for compatible flow variables.
- Execution-context property bindings are authored as paths such as `QuestComponent.CurrentStage` or `DialogueState.LastSpeaker`.
- Bindings can declare an expected value type, and the editor filters incompatible variables when possible.
- The executor resolves bindings at runtime and converts values to the target type when possible.
- Flow-variable and execution-context-property bindings are writable through the executor.
- Validation reports missing variables and incompatible literal values.

The built-in core logic nodes already use these bindings.

## Built-in core nodes

The base `DreamFlow` module ships with a reusable `Core` category for generic flow logic.

- `UDreamFlowBranchNode`: evaluates a bound boolean and routes to child `0` for true or child `1` for false
- `UDreamFlowCompareNode`: compares two bound values and routes to child `0` or `1`
- `UDreamFlowSetVariableNode`: writes a flow variable, then auto-continues to the first child
- `UDreamFlowRunSubFlowNode`: runs another flow asset as a child flow, waits for it, and can sync or remap variables in and out
- `UDreamFlowFinishFlowNode`: immediately ends the current flow execution
- `UDreamFlowDelayNode`: waits asynchronously for a duration, then continues through `Completed`
- `UDreamFlowWaitUntilNode`: waits until a bool binding becomes true
- `UDreamFlowWaitForBindingChangeNode`: waits until a binding resolves to a different value

These nodes target `UDreamFlowAsset`, so they are available in any DreamFlow-derived asset type unless a more specialized node narrows compatibility.

## Multi-output pins

Nodes can declare named output pins instead of being limited to one generic `Out`.

- Output pin definitions come from `UDreamFlowNode::GetOutputPins`
- The graph editor renders one pin per declared output
- Automatic transitions can resolve by output pin name instead of relying only on child index
- Built-in branching nodes use this to expose explicit `True` and `False` outputs

## Node appearance and previews

Every `UDreamFlowNode` can expose a compact display area inside the graph node card.

- Use the built-in `PreviewItems` array for simple previews
- Override `GetNodeDisplayItems` for fully data-driven custom previews from C++ or Blueprint
- Supported display item types are `Text`, `Color`, and `Image`
- Node cards, pins, and connection colors use the node's `NodeTint` so graph visuals stay consistent

This is useful for showing portraits, state colors, rewards, icons, summary text, or designer notes directly on the node.

## Large graph behavior

The editor is optimized to stay usable on bigger graphs.

- Node details are opened through a deferred update path instead of forcing synchronous details refreshes during selection changes
- Validation messages are cached by node GUID for cheaper graph painting
- For large graphs, inline node property editors are automatically disabled once the graph grows beyond the inline-editor threshold, and editing continues in the `Node Editor` side panel

This keeps large multi-node assets more responsive while preserving normal authoring workflows.

## Asset compatibility

Nodes can declare which flow asset type they support through `SupportedFlowAssetType`.

- If a node supports `UDreamFlowAsset`, it can be used in any DreamFlow asset subclass
- If a node supports a specialized asset subclass from another plugin, it will only appear in compatible editors
- Incompatible nodes are blocked by palette/context-menu creation and can also be reported by validation

Core DreamFlow nodes target `UDreamFlowAsset`.

## C++ extension

Derive from `UDreamFlowNode` and override any of the following:

- `GetNodeDisplayName`
- `GetNodeTint`
- `GetNodeCategory`
- `GetNodeAccentLabel`
- `GetNodeDisplayItems`
- `SupportsMultipleChildren`
- `SupportsMultipleParents`
- `IsTerminalNode`
- `CanEnterNode`
- `CanConnectTo`
- `ExecuteNode`
- `ExecuteNodeWithExecutor`
- `StartAsyncNode` when deriving from `UDreamFlowAsyncNode`
- `SupportsAutomaticTransition`
- `ResolveAutomaticTransitionChildIndex`
- `ResolveAutomaticTransitionOutputPin`
- `GetOutputPins`

For variable-aware nodes, you can also use:

- `UDreamFlowAsset::FindVariableDefinition`
- `UDreamFlowExecutor::GetVariableValue`
- `UDreamFlowExecutor::SetVariableValue`
- `UDreamFlowExecutor::MoveToOutputPin`
- `UDreamFlowExecutor::ResolveBindingValue`
- `UDreamFlowExecutor::ResolveBindingAsBool`

Example:

```cpp
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYGAME_API UMyTagCheckNode : public UDreamFlowNode
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FName Label;

    UMyTagCheckNode()
    {
        SupportedFlowAssetType = UDreamFlowAsset::StaticClass();
    }

    virtual FText GetNodeDisplayName_Implementation() const override
    {
        return FText::FromName(Label);
    }

    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override
    {
        TArray<FDreamFlowNodeDisplayItem> Items;

        FDreamFlowNodeDisplayItem TextItem;
        TextItem.Type = EDreamFlowNodeDisplayItemType::Text;
        TextItem.Label = FText::FromString(TEXT("Label"));
        TextItem.TextValue = FText::FromName(Label);
        Items.Add(TextItem);

        return Items;
    }
};
```

## Blueprint extension

`UDreamFlowNode` is `Blueprintable`, so you can create Blueprint classes derived from it and then instantiate them from the graph editor with `Choose Node Class...`.

For async Blueprint nodes, derive from `UDreamFlowAsyncNode` instead.

- Override `StartAsyncNode`
- Use the provided `AsyncContext` object to call `Complete()` or `CompleteWithOutputPin()`
- Keep runtime state outside the node asset itself; the `AsyncContext` and `Executor` carry execution state per run
- Use per-node `TransitionMode` to switch between `Automatic` and `Manual` output handling
- Blueprint node classes can also call `ContinueFlow(Executor)` or `ContinueFlowFromOutputPin(Executor, OutputPinName)` from `ExecuteNodeWithExecutor`

## Runtime execution

You can execute flows either by creating `UDreamFlowExecutor` directly or by attaching `UDreamFlowExecutorComponent` to an Actor.

The executor currently supports:

- `StartFlow`
- `RestartFlow`
- `FinishFlow`
- `Advance`
- `MoveToChildByIndex`
- `ChooseChild`
- `MoveToOutputPin`
- `GetCurrentNode`
- `GetPendingAsyncNode`
- `GetAvailableChildren`
- `GetAvailableOutputPins`
- `IsCurrentNodeAutomatic`
- `IsWaitingForAdvance`
- `IsWaitingForAsyncNode`
- `GetVisitedNodes`
- `IsRunning`
- `CompleteAsyncNode`
- `PauseExecution`
- `ContinueExecution`
- `StepExecution`
- `SetPauseOnBreakpoints`
- `NotifyExecutionContextChanged`
- `NotifyExecutionContextPropertyChanged`
- `IsPaused`
- `GetPauseOnBreakpoints`
- `GetDebugState`
- `HasVariable`
- `GetVariableValue`
- `SetVariableValue`
- `ResetVariablesToDefaults`
- `GetNodeStateValue`
- `SetNodeStateValue`
- `ResetNodeState`
- `ResetAllNodeStates`
- `BuildExecutionSnapshot`
- `ApplyExecutionSnapshot`
- `GetParentExecutor`
- `GetCurrentChildExecutor`
- `GetActiveSubFlowStack`
- `GetExecutionContextPropertyValue`
- `SetExecutionContextPropertyValue`
- `ResolveBindingValue`
- `ResolveBindingAsBool`

`UDreamFlowBlueprintLibrary` also exposes helper constructors such as:

- `CreateFlowExecutor`
- `MakeBoolFlowValue`
- `MakeIntFlowValue`
- `MakeFloatFlowValue`
- `MakeNameFlowValue`
- `MakeStringFlowValue`
- `MakeTextFlowValue`
- `MakeGameplayTagFlowValue`

It also exposes low-level helper nodes for:

- finding flow variable definitions
- checking node-to-asset compatibility
- reading node output links
- converting and comparing `FDreamFlowValue`
- describing `FDreamFlowValue` and `FDreamFlowValueBinding` for UI/debug output
- creating and editing literal, variable, and execution-context-property bindings
- reading and writing executor node state
- capturing and restoring executor snapshots

And it exposes Blueprint delegates for:

- `OnFlowStarted`
- `OnFlowFinished`
- `OnNodeEntered`
- `OnNodeExited`
- `OnExecutionPaused`
- `OnExecutionResumed`
- `OnDebugStateChanged`

## Logging and diagnostics

DreamFlow includes a built-in log category and a `Developer Settings` page.

- Open `Project Settings -> DreamPlugin -> Dream Flow`
- Use `Logging` to enable or disable DreamFlow logs globally
- Use `Log Verbosity` to clamp output from `Error` through `VeryVerbose`
- Use channel toggles to control `General`, `Execution`, `Variables`, `Replication`, `Validation`, and `Automation Tests` logging independently

Runtime logs are emitted through `LogDreamFlow`, so they can also be filtered through normal Unreal log tooling.

## Node state and snapshots

DreamFlow now exposes two executor-level runtime storage layers:

- flow variables for shared flow-wide environment values
- per-node runtime state for node-specific instance data keyed by `NodeGuid`

Use these when you need counters, local flags, resolved values, or lightweight state without mutating the shared node asset.

The executor also exposes snapshot helpers:

- `BuildExecutionSnapshot`
- `ApplyExecutionSnapshot`

These capture current node, visited nodes, runtime variables, per-node runtime state, and visible child-flow stack metadata.

## Sub flows

`Run Sub Flow` supports more than just fire-and-wait behavior.

- Parent executors can inspect active child executors and sub-flow stack entries.
- Child flows can still inherit shared variables by name.
- You can now define explicit input and output variable mappings when parent and child variable names differ.
- Child executors reuse the same execution context by default.

## Reactive bindings

`Wait Until` and `Wait For Binding Change` are now delegate-driven for reactive binding sources.

- Flow-variable bindings wake up when `SetVariable...Value` changes the target variable
- Execution-context-property bindings wake up when `SetExecutionContextPropertyValue` or `NotifyExecutionContextPropertyChanged` reports a change
- External gameplay code that mutates the execution context directly can manually invalidate reactive waits with `NotifyExecutionContextChanged` or `NotifyExecutionContextPropertyChanged`
- Optional fallback polling is still available for non-reactive sources such as literal-only waits

## Networking

`UDreamFlowExecutorComponent` supports a server-authoritative replication flow.

- The server owns the real `UDreamFlowExecutor` and runs node logic there
- The component replicates flow asset assignment, current node GUID, visited node GUIDs, pause state, and runtime flow variables
- The replicated snapshot also carries async waiting state so clients know when a flow is blocked
- Clients build a mirrored local executor from that replicated snapshot, so Blueprint reads such as `GetCurrentNode` and `GetVariable...Value` still work remotely
- Client calls such as `StartFlow`, `Advance`, `ChooseChild`, `MoveToOutputPin`, `SetVariable...Value`, and `ResetVariablesToDefaults` are forwarded to the server with RPCs
- Manual multi-output nodes can be driven safely over the network with `MoveToOutputPin`

Notes:

- The owning Actor still needs normal Unreal replication enabled
- The replicated state mirrors execution and variables, but it does not replay arbitrary gameplay side effects for your nodes
- `Object` variables are only safe over the network when they point to references Unreal can already replicate or resolve remotely

## Automation tests

DreamFlow ships with automation coverage for core runtime behavior.

- `DreamFlow.Core.Execution.AutomaticBranching`
- `DreamFlow.Core.Execution.ManualMultiOutputStep`
- `DreamFlow.Core.Execution.AsyncManualCompletion`
- `DreamFlow.Core.Execution.ManualEntryStart`
- `DreamFlow.Core.Validation.MissingVariable`
- `DreamFlow.Core.Execution.FinishFlowNodeStopsExecution`
- `DreamFlow.Core.Network.ReplicatedStateMirror`
- `DreamFlow.Core.Logging.SettingsFiltering`
- `DreamFlow.Core.Display.BindingCompactDescription`

Run them from the Automation window or from command line with:

```powershell
UnrealEditor-Cmd.exe PluginsDevelop.uproject -ExecCmds="Automation RunTests DreamFlow.Core; Quit" -TestExit="Automation Test Queue Empty" -unattended -NullRHI
```

## Debugger

The editor debugger is designed for live inspection of `UDreamFlowExecutor` instances tied to the currently opened asset.

- It lists active executors for the asset while PIE or gameplay is running
- It shows current debug state and focused node
- It supports `Pause`, `Continue`, `Step`, and `Stop`
- It can focus the selected runtime node back in the graph editor
- Breakpoints are stored on the flow asset by node GUID, so they stay attached to the graph

## Companion plugins

DreamFlow is designed to be extended by sibling plugins.

Current companion plugins in this repository include:

- `DreamFlowExpansion`: domain packs such as Quest, Dialogue, Story, and Encounter flow assets and nodes
- `DreamFlowGameplayTaskBridge`: bridge nodes that connect DreamFlow execution to the `DreamGameplayTask` plugin

If you need quest/dialogue behavior, enable `DreamFlowExpansion` in addition to `DreamFlow`.

## Validation

`UDreamFlowAsset::ValidateFlow` and `UDreamFlowBlueprintLibrary::ValidateFlow` report:

- missing entry nodes
- duplicate node GUIDs
- duplicate or unnamed flow variables
- null or foreign child references
- unreachable nodes
- nodes with no incoming links
- missing bound variables
- incompatible literal bindings for typed variable writes
- asset-type compatibility issues for incompatible nodes

These rules are intentionally generic so the core framework stays reusable across different flow domains.
