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
10. Right click a node and choose `Add Breakpoint` to pause before that node executes.

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
- `GetCurrentNode`
- `GetAvailableChildren`
- `PauseExecution`
- `ContinueExecution`
- `StepExecution`
- `SetPauseOnBreakpoints`

It also exposes Blueprint delegates for:

- `OnFlowStarted`
- `OnFlowFinished`
- `OnNodeEntered`
- `OnNodeExited`
- `OnExecutionPaused`
- `OnExecutionResumed`
- `OnDebugStateChanged`

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
- null or foreign child references
- unreachable nodes
- nodes with no incoming links

These rules are intentionally generic so the framework stays reusable across quest, dialogue, and custom flow systems.
