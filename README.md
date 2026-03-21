# DreamFlow

DreamFlow is a lightweight Unreal flow graph framework designed as a reusable base for:

- Quest/task graphs
- Dialogue graphs
- State and progression editors
- Custom designer-facing logic graphs

## What it includes

- `UDreamFlowAsset`: graph asset that stores runtime node data
- `UDreamQuestFlowAsset`: quest-specialized flow asset
- `UDreamDialogueFlowAsset`: dialogue-specialized flow asset
- `UDreamFlowNode`: extensible runtime node base class
- `UDreamFlowEntryNode`: built-in entry node
- `UDreamFlowExecutor`: runtime flow runner with Blueprint events
- `UDreamFlowExecutorComponent`: Actor component wrapper for gameplay use
- Per-flow variable definitions with typed runtime values and bindings
- Built-in generic core nodes such as `Branch`, `Compare`, and `Set Variable`
- Multi-output node pins with named branches such as `True` and `False`
- Node preview content area with text, color, and image display items
- Breakpoint-aware DreamFlow debugger tab for active executors
- Graph validation helpers and validation message structs
- `DreamFlowQuest`: quest/task focused node module
- `DreamFlowDialogue`: dialogue focused node module
- `DreamFlowEditor`: asset factory, asset editor toolkit, graph schema, and custom node Slate UI
- Blueprint helpers in `UDreamFlowBlueprintLibrary`

## How to use

1. Enable the `DreamFlow` plugin.
2. In Content Browser create the asset type that matches your domain:
   `Dream Flow`, `Dream Quest Flow`, or `Dream Dialogue Flow`.
3. Open the asset to edit the graph.
4. Right click in the graph to add nodes.
5. Use `Choose Node Class...` to add Blueprint-derived node types.
6. Use the left `Node Palette` tab to quickly place common node classes.
7. Select a node to open its runtime object in the right `Node Editor` tab.
8. Use the right `Validation` tab to inspect broken links, unreachable nodes, duplicate GUIDs, and other structural issues.
9. Open the `Debugger` tab to inspect live executors while running in PIE or game.
10. Use the `Variables` tab to edit flow-scoped environment variables in one dedicated panel.
11. Right click a node and choose `Add Breakpoint` to pause before that node executes.
12. The graph editor supports undo/redo, copy, cut, paste, and duplicate for normal nodes.

## Flow variables

Every `UDreamFlowAsset` now owns a `Variables` array, so each flow asset can define its own environment-style data without pushing everything into one global runtime object.

- Variable definitions are now surfaced in a dedicated `Variables` editor tab.
- The variables tab supports type-first creation, so designers can add `Bool`, `Int`, `Float`, `Name`, `String`, `Text`, `GameplayTag`, and `Object` variables directly from the header menu.
- Each variable has a `Name`, `Description`, and typed `DefaultValue`.
- Supported value types are `Bool`, `Int`, `Float`, `Name`, `String`, `Text`, `GameplayTag`, and `Object`.
- When a `UDreamFlowExecutor` initializes, it copies those defaults into its runtime variable map.

This makes the variable model feel closer to Blueprint variables, but still scoped per DreamFlow asset.

## Variable bindings

DreamFlow now includes a lightweight binding struct, `FDreamFlowValueBinding`, for nodes that need data input.

- A binding can read from either a literal value or a named flow variable.
- The details UI now exposes a dedicated source picker and a variable dropdown so bindings feel closer to the StateTree workflow.
- Bindings can expose an expected value type, and the variable picker filters out incompatible flow variables when possible.
- The executor resolves bindings at runtime and converts values to the target variable type when possible.
- Validation reports missing variables and incompatible literal values.

Out of the box, the built-in core logic nodes already use these bindings.

## Built-in core nodes

The base `DreamFlow` module now ships with a reusable `Core` category for generic flow logic.

- `UDreamFlowBranchNode`: evaluates a bound boolean and routes to child `0` for true or child `1` for false
- `UDreamFlowCompareNode`: compares two bound values and routes to child `0` or `1`
- `UDreamFlowSetVariableNode`: writes a flow variable, then auto-continues to the first child

These nodes target `UDreamFlowAsset`, so they are available in any DreamFlow-derived asset type unless a more specialized node chooses to narrow compatibility.

## Multi-output pins

Nodes can now declare named output pins instead of being limited to one generic `Out`.

- Output pin definitions come from `UDreamFlowNode::GetOutputPins`.
- The graph editor renders one pin per declared output.
- Automatic transitions can now resolve by output pin name instead of relying only on child index.
- Built-in branching nodes use this to expose explicit `True` and `False` outputs.

## Node preview area

Every `UDreamFlowNode` can expose a compact display area inside the graph node card.

- Use the built-in `PreviewItems` array for simple previews.
- Override `GetNodeDisplayItems` for fully data-driven custom previews from C++ or Blueprint.
- Supported display item types are `Text`, `Color`, and `Image`.

This is useful for showing things like dialogue portraits, task state colors, reward summaries, icons, or short designer notes directly on the node.

## Asset compatibility

Nodes can now declare which flow asset type they support through `SupportedFlowAssetType`.

- If a node supports `UDreamFlowAsset`, it can be used in any DreamFlow asset subclass.
- If a node supports a specialized asset such as `UDreamQuestFlowAsset` or `UDreamDialogueFlowAsset`, it will only appear in compatible editors.
- Incompatible nodes are blocked by palette/context-menu creation and are also reported by validation if they already exist in an asset.

The built-in domain nodes are configured like this by default:

- Quest nodes target `UDreamQuestFlowAsset`
- Dialogue nodes target `UDreamDialogueFlowAsset`
- Core nodes target `UDreamFlowAsset`

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
class MYGAME_API UQuestStepNode : public UDreamFlowNode
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName QuestStepId;

    UQuestStepNode()
    {
        SupportedFlowAssetType = UDreamQuestFlowAsset::StaticClass();
    }

    virtual FText GetNodeDisplayName_Implementation() const override
    {
        return FText::FromName(QuestStepId);
    }

    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const override
    {
        TArray<FDreamFlowNodeDisplayItem> Items;

        FDreamFlowNodeDisplayItem TextItem;
        TextItem.Type = EDreamFlowNodeDisplayItemType::Text;
        TextItem.Label = FText::FromString(TEXT("Quest Step"));
        TextItem.TextValue = FText::FromName(QuestStepId);
        Items.Add(TextItem);

        return Items;
    }
};
```

## Blueprint extension

`UDreamFlowNode` is `Blueprintable`, so you can create Blueprint classes derived from it and then instantiate them from the graph editor with `Choose Node Class...`.

## Runtime execution

You can execute graphs either by creating `UDreamFlowExecutor` directly or by attaching `UDreamFlowExecutorComponent` to an Actor.

The executor currently supports:

- `StartFlow`
- `RestartFlow`
- `FinishFlow`
- `Advance`
- `MoveToChildByIndex`
- `ChooseChild`
- `MoveToOutputPin`
- `GetCurrentNode`
- `GetAvailableChildren`
- `PauseExecution`
- `ContinueExecution`
- `StepExecution`
- `SetPauseOnBreakpoints`
- `HasVariable`
- `GetVariableValue`
- `SetVariableValue`
- `ResetVariablesToDefaults`
- `ResolveBindingValue`
- `ResolveBindingAsBool`

For Blueprint convenience, `UDreamFlowBlueprintLibrary` also exposes helper constructors such as:

- `MakeBoolFlowValue`
- `MakeIntFlowValue`
- `MakeFloatFlowValue`
- `MakeNameFlowValue`
- `MakeStringFlowValue`
- `MakeTextFlowValue`
- `MakeGameplayTagFlowValue`

It also exposes Blueprint delegates for:

- `OnFlowStarted`
- `OnFlowFinished`
- `OnNodeEntered`
- `OnNodeExited`
- `OnExecutionPaused`
- `OnExecutionResumed`
- `OnDebugStateChanged`

## Networking

`UDreamFlowExecutorComponent` now supports a server-authoritative replication flow.

- The server owns the real `UDreamFlowExecutor` and runs node logic there.
- The component replicates flow asset assignment, current node GUID, visited node GUIDs, pause state, and runtime flow variables.
- Clients build a mirrored local executor from that replicated snapshot, so Blueprint reads such as `GetCurrentNode` and `GetVariable...Value` continue to work on remote machines.
- Client calls made through `UDreamFlowExecutorComponent` such as `StartFlow`, `Advance`, `ChooseChild`, `MoveToOutputPin`, `SetVariable...Value`, and `ResetVariablesToDefaults` are forwarded to the server with RPCs.
- For multiplayer gameplay, prefer calling the component APIs instead of invoking `UDreamFlowExecutor` directly.

Notes:

- The owning Actor still needs normal Unreal replication enabled.
- This first pass syncs execution state and variables, but it does not replay arbitrary node-side gameplay effects on clients; those should still be driven by your own replicated gameplay systems.
- `Object` variables are only safe to use over the network when they point to references Unreal can already replicate or resolve on remote machines.

## Debugger

The editor debugger is designed for live inspection of `UDreamFlowExecutor` instances tied to the currently opened asset.

- It lists active executors for the asset while PIE or gameplay is running.
- It shows current debug state and focused node.
- It supports `Pause`, `Continue`, `Step`, and `Stop`.
- It can focus the selected runtime node back in the graph editor.
- Breakpoints are stored on the flow asset by node GUID, so they stay attached to the graph.

## Included domain modules

### Quest module

`DreamFlowQuest` ships with:

- `UDreamFlowQuestTaskNode`
- `UDreamFlowQuestConditionNode`
- `UDreamFlowQuestCompleteNode`

### Dialogue module

`DreamFlowDialogue` ships with:

- `UDreamFlowDialogueLineNode`
- `UDreamFlowDialogueChoiceNode`
- `UDreamFlowDialogueEndNode`

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

These rules are intentionally generic so the framework stays reusable across quest, dialogue, and custom flow systems.
