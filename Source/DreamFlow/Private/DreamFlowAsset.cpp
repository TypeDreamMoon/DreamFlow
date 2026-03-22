#include "DreamFlowAsset.h"

#include "DreamFlowLog.h"
#include "DreamFlowNode.h"
#include "Engine/Engine.h"
#include "Execution/DreamFlowDebuggerSubsystem.h"
#include "Execution/DreamFlowExecutor.h"
#include "Containers/Queue.h"
#include "EdGraph/EdGraph.h"
#include "UObject/UObjectGlobals.h"

UDreamFlowAsset::UDreamFlowAsset()
{
}

UDreamFlowNode* UDreamFlowAsset::GetEntryNode() const
{
    return EntryNode;
}

TArray<UDreamFlowNode*> UDreamFlowAsset::GetNodesCopy() const
{
    TArray<UDreamFlowNode*> Result;
    Result.Reserve(Nodes.Num());

    for (UDreamFlowNode* Node : Nodes)
    {
        Result.Add(Node);
    }

    return Result;
}

UDreamFlowNode* UDreamFlowAsset::FindNodeByGuid(const FGuid& InNodeGuid) const
{
    if (!InNodeGuid.IsValid())
    {
        return nullptr;
    }

    for (UDreamFlowNode* Node : Nodes)
    {
        if (Node != nullptr && Node->NodeGuid == InNodeGuid)
        {
            return Node;
        }
    }

    return nullptr;
}

TArray<FDreamFlowVariableDefinition> UDreamFlowAsset::GetVariablesCopy() const
{
    return Variables;
}

UDreamFlowExecutor* UDreamFlowAsset::CreateExecutor(UObject* ExecutionContext, TSubclassOf<UDreamFlowExecutor> ExecutorClass)
{
    UClass* EffectiveExecutorClass = ExecutorClass != nullptr ? ExecutorClass.Get() : UDreamFlowExecutor::StaticClass();
    UObject* Outer = ExecutionContext != nullptr ? ExecutionContext : GetTransientPackage();
    UDreamFlowExecutor* Executor = NewObject<UDreamFlowExecutor>(Outer, EffectiveExecutorClass, NAME_None, RF_Transient);
    if (Executor != nullptr)
    {
        Executor->Initialize(this, ExecutionContext);
    }

    return Executor;
}

TArray<UDreamFlowExecutor*> UDreamFlowAsset::GetActiveExecutors() const
{
    if (GEngine == nullptr)
    {
        return TArray<UDreamFlowExecutor*>();
    }

    if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
    {
        return DebuggerSubsystem->GetExecutorsForAsset(this);
    }

    return TArray<UDreamFlowExecutor*>();
}

TArray<UDreamFlowExecutor*> UDreamFlowAsset::GetExecutorsOnNode(const UDreamFlowNode* Node) const
{
    if (!OwnsNode(Node) || GEngine == nullptr)
    {
        return TArray<UDreamFlowExecutor*>();
    }

    if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
    {
        return DebuggerSubsystem->GetExecutorsForNode(Node);
    }

    return TArray<UDreamFlowExecutor*>();
}

void UDreamFlowAsset::ValidateFlow(TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    OutMessages.Reset();

    if (Nodes.Num() == 0)
    {
        FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
        Message.Severity = EDreamFlowValidationSeverity::Error;
        Message.Message = FText::FromString(TEXT("The graph does not contain any nodes."));
        return;
    }

    TSet<const UDreamFlowNode*> KnownNodes;
    TSet<FGuid> SeenGuids;
    TSet<FName> SeenVariableNames;
    TMap<const UDreamFlowNode*, int32> IncomingConnectionCount;
    TArray<const UDreamFlowNode*> EntryCandidates;

    for (const FDreamFlowVariableDefinition& Variable : Variables)
    {
        if (Variable.Name.IsNone())
        {
            FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
            Message.Severity = EDreamFlowValidationSeverity::Error;
            Message.Message = FText::FromString(TEXT("A flow variable is missing its name."));
            continue;
        }

        if (SeenVariableNames.Contains(Variable.Name))
        {
            FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
            Message.Severity = EDreamFlowValidationSeverity::Error;
            Message.Message = FText::Format(
                FText::FromString(TEXT("The flow variable '{0}' is defined more than once.")),
                FText::FromName(Variable.Name));
            continue;
        }

        SeenVariableNames.Add(Variable.Name);
    }

    for (UDreamFlowNode* Node : Nodes)
    {
        if (Node == nullptr)
        {
            FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
            Message.Severity = EDreamFlowValidationSeverity::Error;
            Message.Message = FText::FromString(TEXT("The graph contains a null node reference."));
            continue;
        }

        KnownNodes.Add(Node);
        IncomingConnectionCount.FindOrAdd(Node, 0);

        if (!Node->SupportsFlowAsset(this))
        {
            FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
            Message.Severity = EDreamFlowValidationSeverity::Error;
            Message.NodeGuid = Node->NodeGuid;
            Message.NodeTitle = Node->GetNodeDisplayName();
            Message.Message = FText::Format(
                FText::FromString(TEXT("This node type supports '{0}', but the current asset is '{1}'.")),
                Node->GetSupportedFlowAssetType() != nullptr
                    ? Node->GetSupportedFlowAssetType()->GetDisplayNameText()
                    : UDreamFlowAsset::StaticClass()->GetDisplayNameText(),
                GetClass()->GetDisplayNameText());
        }

        if (SeenGuids.Contains(Node->NodeGuid))
        {
            FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
            Message.Severity = EDreamFlowValidationSeverity::Error;
            Message.NodeGuid = Node->NodeGuid;
            Message.NodeTitle = Node->GetNodeDisplayName();
            Message.Message = FText::FromString(TEXT("This node shares a duplicate GUID with another node."));
        }
        else
        {
            SeenGuids.Add(Node->NodeGuid);
        }

        if (Node->IsEntryNode())
        {
            EntryCandidates.Add(Node);
        }

        if (Node->GetNodeDisplayName().IsEmpty())
        {
            FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
            Message.Severity = EDreamFlowValidationSeverity::Warning;
            Message.NodeGuid = Node->NodeGuid;
            Message.NodeTitle = FText::FromString(TEXT("Unnamed Node"));
            Message.Message = FText::FromString(TEXT("This node does not have a display title."));
        }

        Node->ValidateNode(this, OutMessages);
    }

    if (EntryNode == nullptr)
    {
        FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
        Message.Severity = EDreamFlowValidationSeverity::Error;
        Message.Message = FText::FromString(TEXT("The graph does not define an entry node."));
    }
    else if (!KnownNodes.Contains(EntryNode))
    {
        FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
        Message.Severity = EDreamFlowValidationSeverity::Error;
        Message.NodeGuid = EntryNode->NodeGuid;
        Message.NodeTitle = EntryNode->GetNodeDisplayName();
        Message.Message = FText::FromString(TEXT("The entry node is not present in the node list."));
    }

    if (EntryCandidates.Num() > 1)
    {
        for (const UDreamFlowNode* EntryCandidate : EntryCandidates)
        {
            FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
            Message.Severity = EDreamFlowValidationSeverity::Warning;
            Message.NodeGuid = EntryCandidate->NodeGuid;
            Message.NodeTitle = EntryCandidate->GetNodeDisplayName();
            Message.Message = FText::FromString(TEXT("Multiple nodes are marked as entry nodes. Only one should act as the starting point."));
        }
    }

    for (UDreamFlowNode* Node : Nodes)
    {
        if (Node == nullptr)
        {
            continue;
        }

        for (UDreamFlowNode* ChildNode : Node->GetChildrenCopy())
        {
            if (ChildNode == nullptr)
            {
                FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
                Message.Severity = EDreamFlowValidationSeverity::Error;
                Message.NodeGuid = Node->NodeGuid;
                Message.NodeTitle = Node->GetNodeDisplayName();
                Message.Message = FText::FromString(TEXT("This node has an empty child reference."));
                continue;
            }

            if (!KnownNodes.Contains(ChildNode))
            {
                FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
                Message.Severity = EDreamFlowValidationSeverity::Error;
                Message.NodeGuid = Node->NodeGuid;
                Message.NodeTitle = Node->GetNodeDisplayName();
                Message.Message = FText::Format(
                    FText::FromString(TEXT("This node points to '{0}', but that child is not owned by the same flow asset.")),
                    ChildNode->GetNodeDisplayName());
                continue;
            }

            IncomingConnectionCount.FindOrAdd(ChildNode)++;
        }
    }

    if (EntryNode != nullptr && KnownNodes.Contains(EntryNode))
    {
        TQueue<const UDreamFlowNode*> PendingNodes;
        TSet<const UDreamFlowNode*> ReachableNodes;

        PendingNodes.Enqueue(EntryNode);
        ReachableNodes.Add(EntryNode);

        const UDreamFlowNode* CurrentNode = nullptr;
        while (PendingNodes.Dequeue(CurrentNode))
        {
            for (UDreamFlowNode* ChildNode : CurrentNode->GetChildrenCopy())
            {
                if (ChildNode != nullptr && KnownNodes.Contains(ChildNode) && !ReachableNodes.Contains(ChildNode))
                {
                    ReachableNodes.Add(ChildNode);
                    PendingNodes.Enqueue(ChildNode);
                }
            }
        }

        for (UDreamFlowNode* Node : Nodes)
        {
            if (Node == nullptr)
            {
                continue;
            }

            if (!ReachableNodes.Contains(Node))
            {
                FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
                Message.Severity = EDreamFlowValidationSeverity::Warning;
                Message.NodeGuid = Node->NodeGuid;
                Message.NodeTitle = Node->GetNodeDisplayName();
                Message.Message = FText::FromString(TEXT("This node cannot be reached from the entry node."));
            }

            if (!Node->IsEntryNode() && IncomingConnectionCount.FindRef(Node) == 0)
            {
                FDreamFlowValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
                Message.Severity = EDreamFlowValidationSeverity::Info;
                Message.NodeGuid = Node->NodeGuid;
                Message.NodeTitle = Node->GetNodeDisplayName();
                Message.Message = FText::FromString(TEXT("This node has no incoming links."));
            }
        }
    }

    if (FDreamFlowLog::ShouldLog(ELogVerbosity::Log, EDreamFlowLogChannel::Validation))
    {
        int32 ErrorCount = 0;
        int32 WarningCount = 0;
        int32 InfoCount = 0;

        for (const FDreamFlowValidationMessage& Message : OutMessages)
        {
            switch (Message.Severity)
            {
            case EDreamFlowValidationSeverity::Error:
                ++ErrorCount;
                break;

            case EDreamFlowValidationSeverity::Warning:
                ++WarningCount;
                break;

            case EDreamFlowValidationSeverity::Info:
            default:
                ++InfoCount;
                break;
            }
        }

        DREAMFLOW_LOG(Validation, Log, "Validated flow '%s'. Errors=%d Warnings=%d Info=%d", *GetNameSafe(this), ErrorCount, WarningCount, InfoCount);
    }
}

bool UDreamFlowAsset::HasBreakpointOnNode(const FGuid& InNodeGuid) const
{
#if WITH_EDITORONLY_DATA
    return InNodeGuid.IsValid() && BreakpointNodeGuids.Contains(InNodeGuid);
#else
    (void)InNodeGuid;
    return false;
#endif
}

void UDreamFlowAsset::SetBreakpointOnNode(const FGuid& InNodeGuid, bool bEnabled)
{
#if WITH_EDITORONLY_DATA
    if (!InNodeGuid.IsValid())
    {
        return;
    }

    if (bEnabled)
    {
        BreakpointNodeGuids.AddUnique(InNodeGuid);
    }
    else
    {
        BreakpointNodeGuids.Remove(InNodeGuid);
    }
#else
    (void)InNodeGuid;
    (void)bEnabled;
#endif
}

const TArray<FGuid>& UDreamFlowAsset::GetBreakpointNodeGuids() const
{
#if WITH_EDITORONLY_DATA
    return BreakpointNodeGuids;
#else
    static const TArray<FGuid> EmptyBreakpoints;
    return EmptyBreakpoints;
#endif
}

void UDreamFlowAsset::ReplaceNodes(const TArray<UDreamFlowNode*>& InNodes)
{
    Nodes.Reset();

    for (UDreamFlowNode* Node : InNodes)
    {
        if (Node != nullptr)
        {
            Nodes.Add(Node);
        }
    }
}

void UDreamFlowAsset::SetEntryNodeInternal(UDreamFlowNode* InEntryNode)
{
    EntryNode = InEntryNode;
}

const TArray<TObjectPtr<UDreamFlowNode>>& UDreamFlowAsset::GetNodes() const
{
    return Nodes;
}

bool UDreamFlowAsset::OwnsNode(const UDreamFlowNode* Node) const
{
    if (Node == nullptr)
    {
        return false;
    }

    return EntryNode == Node || Nodes.Contains(Node);
}

bool UDreamFlowAsset::HasVariableDefinition(FName VariableName) const
{
    return FindVariableDefinition(VariableName) != nullptr;
}

const FDreamFlowVariableDefinition* UDreamFlowAsset::FindVariableDefinition(FName VariableName) const
{
    if (VariableName.IsNone())
    {
        return nullptr;
    }

    for (const FDreamFlowVariableDefinition& Variable : Variables)
    {
        if (Variable.Name == VariableName)
        {
            return &Variable;
        }
    }

    return nullptr;
}

#if WITH_EDITOR
UEdGraph* UDreamFlowAsset::GetEditorGraph() const
{
#if WITH_EDITORONLY_DATA
    return EditorGraph;
#else
    return nullptr;
#endif
}

void UDreamFlowAsset::SetEditorGraph(UEdGraph* InEditorGraph)
{
#if WITH_EDITORONLY_DATA
    EditorGraph = InEditorGraph;
#else
    (void)InEditorGraph;
#endif
}
#endif
