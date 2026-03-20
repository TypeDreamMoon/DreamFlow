# DreamFlow

DreamFlow is a lightweight Unreal flow graph framework designed as a reusable base for:

- Quest/task graphs
- Dialogue graphs
- State and progression editors
- Custom designer-facing logic graphs

## What it includes

- `UDreamFlowAsset`: graph asset that stores runtime node data
- `UDreamFlowNode`: extensible runtime node base class
- `UDreamFlowEntryNode`: built-in entry node
- `UDreamFlowExecutor`: runtime flow runner with Blueprint events
- `UDreamFlowExecutorComponent`: Actor component wrapper for gameplay use
- Graph validation helpers and validation message structs
- `DreamFlowQuest`: quest/task focused node module
- `DreamFlowDialogue`: dialogue focused node module
- `DreamFlowEditor`: asset factory, asset editor toolkit, graph schema, and custom node Slate UI
- Blueprint helpers in `UDreamFlowBlueprintLibrary`

## How to use

1. Enable the `DreamFlow` plugin.
2. In Content Browser create a new `Dream Flow` asset.
3. Open the asset to edit the graph.
4. Right click in the graph to add nodes.
5. Use `Choose Node Class...` to add Blueprint-derived node types.
6. Use the left `Node Palette` tab to quickly place common node classes.
7. Use the right `Validation` tab to inspect broken links, unreachable nodes, duplicate GUIDs, and other structural issues.

## C++ extension

Derive from `UDreamFlowNode` and override any of the following:

- `GetNodeDisplayName`
- `GetNodeTint`
- `GetNodeCategory`
- `GetNodeAccentLabel`
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

    virtual FText GetNodeDisplayName_Implementation() const override
    {
        return FText::FromName(QuestStepId);
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

It also exposes Blueprint delegates for:

- `OnFlowStarted`
- `OnFlowFinished`
- `OnNodeEntered`
- `OnNodeExited`

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
