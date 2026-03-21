#include "DreamFlowEditorUtils.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEdGraph.h"
#include "DreamFlowEdGraphNode.h"
#include "DreamFlowEdGraphRerouteNode.h"
#include "DreamFlowEdGraphSchema.h"
#include "DreamFlowEntryNode.h"
#include "DreamFlowNode.h"
#include "Algo/Sort.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "IContentBrowserSingleton.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "DreamFlowEditorUtils"

namespace DreamFlowEditorUtils
{
    class FDreamFlowClassFilter : public IClassViewerFilter
    {
    public:
        explicit FDreamFlowClassFilter(UDreamFlowAsset* InFlowAsset)
            : FlowAsset(InFlowAsset)
        {
        }

        virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
        {
            (void)InInitOptions;
            (void)InFilterFuncs;
            return FDreamFlowEditorUtils::IsNodeClassCreatable(InClass, FlowAsset.Get());
        }

        virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
        {
            (void)InInitOptions;
            (void)InFilterFuncs;

            if (InUnloadedClassData->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown))
            {
                return false;
            }

            return InUnloadedClassData->IsChildOf(UDreamFlowNode::StaticClass()) && !InUnloadedClassData->IsChildOf(UDreamFlowEntryNode::StaticClass());
        }

    private:
        TWeakObjectPtr<UDreamFlowAsset> FlowAsset;
    };

    static void CollectRuntimeTargetsFromLinkedPin(
        const UEdGraphPin* LinkedPin,
        UDreamFlowNode* SourceNode,
        TArray<UDreamFlowNode*>& OutTargets,
        TSet<const UEdGraphPin*>& VisitedPins)
    {
        if (LinkedPin == nullptr || VisitedPins.Contains(LinkedPin))
        {
            return;
        }

        VisitedPins.Add(LinkedPin);

        if (const UDreamFlowEdGraphNode* TargetGraphNode = Cast<UDreamFlowEdGraphNode>(LinkedPin->GetOwningNode()))
        {
            if (UDreamFlowNode* TargetNode = TargetGraphNode->GetRuntimeNode())
            {
                if (TargetNode != SourceNode)
                {
                    OutTargets.AddUnique(TargetNode);
                }
            }

            return;
        }

        if (const UDreamFlowEdGraphRerouteNode* RerouteNode = Cast<UDreamFlowEdGraphRerouteNode>(LinkedPin->GetOwningNode()))
        {
            if (const UEdGraphPin* PassThroughPin = RerouteNode->GetPassThroughPin(LinkedPin))
            {
                for (const UEdGraphPin* NextLinkedPin : PassThroughPin->LinkedTo)
                {
                    CollectRuntimeTargetsFromLinkedPin(NextLinkedPin, SourceNode, OutTargets, VisitedPins);
                }
            }
        }
    }
}

TSubclassOf<UDreamFlowNode> FDreamFlowEditorUtils::PickNodeClass(UDreamFlowAsset* FlowAsset)
{
    FClassViewerInitializationOptions PickerOptions;
    PickerOptions.Mode = EClassViewerMode::ClassPicker;
    PickerOptions.DisplayMode = EClassViewerDisplayMode::TreeView;
    PickerOptions.bShowUnloadedBlueprints = true;
    PickerOptions.bShowObjectRootClass = false;
    PickerOptions.NameTypeToDisplay = EClassViewerNameTypeToDisplay::Dynamic;
    PickerOptions.ClassFilters.Add(MakeShared<DreamFlowEditorUtils::FDreamFlowClassFilter>(FlowAsset));

    UClass* ChosenClass = nullptr;
    const bool bPickedClass = SClassPickerDialog::PickClass(
        LOCTEXT("PickNodeClassDialog", "Select DreamFlow Node Class"),
        PickerOptions,
        ChosenClass,
        UDreamFlowNode::StaticClass());

    return bPickedClass ? TSubclassOf<UDreamFlowNode>(ChosenClass) : nullptr;
}

FString FDreamFlowEditorUtils::GetCurrentContentBrowserPath()
{
    const FString InternalPath = IContentBrowserSingleton::Get().GetCurrentPath().GetInternalPathString();
    return InternalPath.IsEmpty() ? TEXT("/Game") : InternalPath;
}

UBlueprint* FDreamFlowEditorUtils::CreateNodeBlueprintAsset(
    TSubclassOf<UDreamFlowNode> NodeClass,
    const FString& TargetPath,
    bool bOpenInEditor)
{
    UClass* NodeParentClass = NodeClass.Get();
    if (NodeParentClass == nullptr || !FKismetEditorUtilities::CanCreateBlueprintOfClass(NodeParentClass))
    {
        return nullptr;
    }

    FString PackagePath = TargetPath.IsEmpty() ? GetCurrentContentBrowserPath() : TargetPath;
    FText ValidationError;
    if (!FPackageName::IsValidLongPackageName(PackagePath, false, &ValidationError))
    {
        PackagePath = TEXT("/Game");
    }

    const FString BaseAssetName = GetNodeBlueprintBaseName(NodeParentClass);
    FString UniquePackageName;
    FString UniqueAssetName;
    FAssetToolsModule::GetModule().Get().CreateUniqueAssetName(PackagePath / BaseAssetName, TEXT(""), UniquePackageName, UniqueAssetName);

    UPackage* Package = CreatePackage(*UniquePackageName);
    if (Package == nullptr)
    {
        return nullptr;
    }

    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        NodeParentClass,
        Package,
        *UniqueAssetName,
        BPTYPE_Normal,
        FName(TEXT("DreamFlowEditor")));

    if (Blueprint == nullptr)
    {
        return nullptr;
    }

    FAssetRegistryModule::AssetCreated(Blueprint);
    Package->MarkPackageDirty();

    TArray<UObject*> AssetsToSync;
    AssetsToSync.Add(Blueprint);
    IContentBrowserSingleton::Get().SyncBrowserToAssets(AssetsToSync, false, true);

    if (bOpenInEditor && GEditor != nullptr)
    {
        if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
        }
    }

    return Blueprint;
}

FVector2f FDreamFlowEditorUtils::GetSuggestedNodeLocation(const UEdGraph* Graph)
{
    constexpr float NodeSpacingX = 420.0f;
    constexpr float DefaultY = 0.0f;

    const UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(Graph);
    if (FlowGraph == nullptr)
    {
        return FVector2f(NodeSpacingX, DefaultY);
    }

    TArray<UDreamFlowEdGraphNode*> GraphNodes;
    FlowGraph->GetNodesOfClass(GraphNodes);

    bool bFoundNode = false;
    float RightmostX = 0.0f;
    float RightmostY = DefaultY;

    for (const UDreamFlowEdGraphNode* GraphNode : GraphNodes)
    {
        if (GraphNode == nullptr)
        {
            continue;
        }

        const float NodeX = static_cast<float>(GraphNode->NodePosX);
        const float NodeY = static_cast<float>(GraphNode->NodePosY);

        if (!bFoundNode || NodeX > RightmostX || (FMath::IsNearlyEqual(NodeX, RightmostX) && NodeY > RightmostY))
        {
            bFoundNode = true;
            RightmostX = NodeX;
            RightmostY = NodeY;
        }
    }

    return bFoundNode
        ? FVector2f(RightmostX + NodeSpacingX, RightmostY)
        : FVector2f(NodeSpacingX, DefaultY);
}

UDreamFlowEdGraph* FDreamFlowEditorUtils::GetOrCreateGraph(UDreamFlowAsset* FlowAsset)
{
    if (FlowAsset == nullptr)
    {
        return nullptr;
    }

    UDreamFlowEdGraph* Graph = Cast<UDreamFlowEdGraph>(FlowAsset->GetEditorGraph());

    if (Graph == nullptr)
    {
        FlowAsset->Modify();
        Graph = NewObject<UDreamFlowEdGraph>(FlowAsset, NAME_None, RF_Transactional);
        Graph->Schema = UDreamFlowEdGraphSchema::StaticClass();
        Graph->SetFlowAsset(FlowAsset);
        FlowAsset->SetEditorGraph(Graph);
        RebuildGraphFromAsset(FlowAsset, Graph);
        return Graph;
    }

    Graph->SetFlowAsset(FlowAsset);

    if (Graph->Schema == nullptr)
    {
        Graph->Schema = UDreamFlowEdGraphSchema::StaticClass();
    }

    if (Graph->Nodes.Num() == 0)
    {
        RebuildGraphFromAsset(FlowAsset, Graph);
    }
    else
    {
        SynchronizeAssetFromGraph(FlowAsset);
    }

    return Graph;
}

UDreamFlowAsset* FDreamFlowEditorUtils::GetFlowAssetFromGraph(const UEdGraph* Graph)
{
    if (const UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(Graph))
    {
        return FlowGraph->GetFlowAsset();
    }

    return nullptr;
}

UDreamFlowEdGraphNode* FDreamFlowEditorUtils::QuickCreateNode(
    UDreamFlowAsset* FlowAsset,
    TSubclassOf<UDreamFlowNode> NodeClass,
    bool bSelectNewNode)
{
    if (FlowAsset == nullptr || NodeClass == nullptr)
    {
        return nullptr;
    }

    UDreamFlowEdGraph* FlowGraph = GetOrCreateGraph(FlowAsset);
    if (FlowGraph == nullptr)
    {
        return nullptr;
    }

    return CreateNodeInGraph(
        FlowGraph,
        NodeClass,
        GetSuggestedNodeLocation(FlowGraph),
        nullptr,
        bSelectNewNode);
}

UDreamFlowEdGraphNode* FDreamFlowEditorUtils::CreateNodeInGraph(
    UEdGraph* Graph,
    TSubclassOf<UDreamFlowNode> NodeClass,
    const FVector2f& Location,
    UEdGraphPin* FromPin,
    bool bSelectNewNode)
{
    UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(Graph);
    UDreamFlowAsset* FlowAsset = FlowGraph ? FlowGraph->GetFlowAsset() : nullptr;

    if (FlowGraph == nullptr || FlowAsset == nullptr || !IsNodeClassCreatable(*NodeClass, FlowAsset))
    {
        return nullptr;
    }

    const FScopedTransaction Transaction(LOCTEXT("CreateFlowNode", "Create Dream Flow Node"));

    FlowAsset->Modify();
    FlowGraph->Modify();

    UDreamFlowNode* RuntimeNode = NewObject<UDreamFlowNode>(FlowAsset, NodeClass, NAME_None, RF_Transactional);
    RuntimeNode->SetEditorPosition(FVector2D(Location.X, Location.Y));

    FGraphNodeCreator<UDreamFlowEdGraphNode> NodeCreator(*FlowGraph);
    UDreamFlowEdGraphNode* GraphNode = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
    GraphNode->SetRuntimeNode(RuntimeNode);
    GraphNode->NodePosX = FMath::RoundToInt(Location.X);
    GraphNode->NodePosY = FMath::RoundToInt(Location.Y);
    NodeCreator.Finalize();
    GraphNode->AutowireNewNode(FromPin);

    SynchronizeAssetFromGraph(FlowAsset);
    FlowGraph->NotifyGraphChanged();
    return GraphNode;
}

UDreamFlowEdGraphRerouteNode* FDreamFlowEditorUtils::CreateRerouteNodeInGraph(
    UEdGraph* Graph,
    const FVector2f& Location,
    UEdGraphPin* FromPin,
    bool bSelectNewNode)
{
    UDreamFlowEdGraph* FlowGraph = Cast<UDreamFlowEdGraph>(Graph);
    UDreamFlowAsset* FlowAsset = FlowGraph ? FlowGraph->GetFlowAsset() : nullptr;

    if (FlowGraph == nullptr || FlowAsset == nullptr)
    {
        return nullptr;
    }

    const FScopedTransaction Transaction(LOCTEXT("CreateFlowRerouteNode", "Create Dream Flow Reroute Node"));

    FlowAsset->Modify();
    FlowGraph->Modify();

    FGraphNodeCreator<UDreamFlowEdGraphRerouteNode> NodeCreator(*FlowGraph);
    UDreamFlowEdGraphRerouteNode* GraphNode = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
    GraphNode->NodePosX = FMath::RoundToInt(Location.X);
    GraphNode->NodePosY = FMath::RoundToInt(Location.Y);
    NodeCreator.Finalize();
    GraphNode->AutowireNewNode(FromPin);

    SynchronizeAssetFromGraph(FlowAsset);
    FlowGraph->NotifyGraphChanged();
    return GraphNode;
}

FString FDreamFlowEditorUtils::GetNodeBlueprintBaseName(const UClass* NodeClass)
{
    FString ParentName = TEXT("DreamFlowNode");
    if (NodeClass != nullptr)
    {
        if (const UBlueprint* ParentBlueprint = Cast<UBlueprint>(NodeClass->ClassGeneratedBy))
        {
            ParentName = ParentBlueprint->GetName();
        }
        else
        {
            ParentName = NodeClass->GetName();
            ParentName.RemoveFromEnd(TEXT("_C"));
        }
    }

    return FString::Printf(
        TEXT("%s_Child"),
        ParentName.StartsWith(TEXT("BP_")) ? *ParentName : *FString::Printf(TEXT("BP_%s"), *ParentName));
}

void FDreamFlowEditorUtils::SynchronizeAssetFromGraph(UDreamFlowAsset* FlowAsset)
{
    UDreamFlowEdGraph* FlowGraph = FlowAsset ? Cast<UDreamFlowEdGraph>(FlowAsset->GetEditorGraph()) : nullptr;
    if (FlowGraph == nullptr)
    {
        return;
    }

    FlowGraph->SetFlowAsset(FlowAsset);

    TArray<UDreamFlowEdGraphNode*> GraphNodes;
    FlowGraph->GetNodesOfClass(GraphNodes);

    TArray<UDreamFlowNode*> RuntimeNodes;
    TMap<UDreamFlowNode*, FVector2D> PositionsByNode;
    TMap<UDreamFlowNode*, TArray<FDreamFlowNodeOutputLink>> OutputLinksByNode;
    UDreamFlowNode* EntryNode = nullptr;

    for (UDreamFlowEdGraphNode* GraphNode : GraphNodes)
    {
        if (GraphNode == nullptr)
        {
            continue;
        }

        UDreamFlowNode* RuntimeNode = GraphNode->GetRuntimeNode();
        if (RuntimeNode == nullptr)
        {
            continue;
        }

        RuntimeNode->SetEditorPosition(FVector2D(GraphNode->NodePosX, GraphNode->NodePosY));
        RuntimeNodes.AddUnique(RuntimeNode);
        PositionsByNode.Add(RuntimeNode, FVector2D(GraphNode->NodePosX, GraphNode->NodePosY));

        if (RuntimeNode->IsEntryNode() && EntryNode == nullptr)
        {
            EntryNode = RuntimeNode;
        }
    }

    for (UDreamFlowEdGraphNode* GraphNode : GraphNodes)
    {
        if (GraphNode == nullptr)
        {
            continue;
        }

        UDreamFlowNode* SourceNode = GraphNode->GetRuntimeNode();
        if (SourceNode == nullptr)
        {
            continue;
        }

        TArray<FDreamFlowNodeOutputLink>& SourceOutputLinks = OutputLinksByNode.FindOrAdd(SourceNode);

        for (UEdGraphPin* OutputPin : GraphNode->GetOutputPins())
        {
            if (OutputPin == nullptr)
            {
                continue;
            }

            for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
            {
                TArray<UDreamFlowNode*> ResolvedTargets;
                TSet<const UEdGraphPin*> VisitedPins;
                DreamFlowEditorUtils::CollectRuntimeTargetsFromLinkedPin(LinkedPin, SourceNode, ResolvedTargets, VisitedPins);

                for (UDreamFlowNode* TargetNode : ResolvedTargets)
                {
                    const bool bAlreadyLinked = SourceOutputLinks.ContainsByPredicate([OutputPin, TargetNode](const FDreamFlowNodeOutputLink& ExistingLink)
                    {
                        return ExistingLink.PinName == OutputPin->PinName && ExistingLink.Child == TargetNode;
                    });

                    if (!bAlreadyLinked)
                    {
                        FDreamFlowNodeOutputLink& OutputLink = SourceOutputLinks.AddDefaulted_GetRef();
                        OutputLink.PinName = OutputPin->PinName;
                        OutputLink.Child = TargetNode;
                    }
                }
            }
        }
    }

    Algo::Sort(RuntimeNodes, [&PositionsByNode](const UDreamFlowNode* A, const UDreamFlowNode* B)
    {
        if (A == nullptr || B == nullptr)
        {
            return A != nullptr;
        }

        const bool bAEntry = A->IsEntryNode();
        const bool bBEntry = B->IsEntryNode();
        if (bAEntry != bBEntry)
        {
            return bAEntry;
        }

        const FVector2D* PosA = PositionsByNode.Find(const_cast<UDreamFlowNode*>(A));
        const FVector2D* PosB = PositionsByNode.Find(const_cast<UDreamFlowNode*>(B));
        const FVector2D SafeA = PosA ? *PosA : FVector2D::ZeroVector;
        const FVector2D SafeB = PosB ? *PosB : FVector2D::ZeroVector;
        return SafeA.X == SafeB.X ? SafeA.Y < SafeB.Y : SafeA.X < SafeB.X;
    });

    FlowAsset->Modify();

    for (UDreamFlowNode* RuntimeNode : RuntimeNodes)
    {
        RuntimeNode->SetOutputLinks(OutputLinksByNode.FindRef(RuntimeNode));
    }

    FlowAsset->ReplaceNodes(RuntimeNodes);
    FlowAsset->SetEntryNodeInternal(EntryNode != nullptr ? EntryNode : (RuntimeNodes.Num() > 0 ? RuntimeNodes[0] : nullptr));
}

TArray<TSubclassOf<UDreamFlowNode>> FDreamFlowEditorUtils::GetLoadedCreatableNodeClasses(const UDreamFlowAsset* FlowAsset)
{
    TArray<TSubclassOf<UDreamFlowNode>> Result;
    TArray<UClass*> DerivedClasses;
    GetDerivedClasses(UDreamFlowNode::StaticClass(), DerivedClasses, true);

    for (UClass* DerivedClass : DerivedClasses)
    {
        if (IsNodeClassCreatable(DerivedClass, FlowAsset))
        {
            Result.Add(DerivedClass);
        }
    }

    Algo::Sort(Result, [](const TSubclassOf<UDreamFlowNode>& A, const TSubclassOf<UDreamFlowNode>& B)
    {
        const UClass* ClassA = *A;
        const UClass* ClassB = *B;
        const UDreamFlowNode* DefaultA = ClassA ? Cast<UDreamFlowNode>(ClassA->GetDefaultObject()) : nullptr;
        const UDreamFlowNode* DefaultB = ClassB ? Cast<UDreamFlowNode>(ClassB->GetDefaultObject()) : nullptr;
        const FString CategoryA = DefaultA ? DefaultA->GetNodeCategory().ToString() : FString();
        const FString CategoryB = DefaultB ? DefaultB->GetNodeCategory().ToString() : FString();
        const FString NameA = ClassA ? ClassA->GetDisplayNameText().ToString() : FString();
        const FString NameB = ClassB ? ClassB->GetDisplayNameText().ToString() : FString();

        if (CategoryA != CategoryB)
        {
            return CategoryA < CategoryB;
        }

        return NameA < NameB;
    });

    return Result;
}

bool FDreamFlowEditorUtils::IsNodeClassCreatable(const UClass* NodeClass, const UDreamFlowAsset* FlowAsset)
{
    if (NodeClass == nullptr)
    {
        return false;
    }

    if (!NodeClass->IsChildOf(UDreamFlowNode::StaticClass()))
    {
        return false;
    }

    if (NodeClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown))
    {
        return false;
    }

    if (NodeClass->IsChildOf(UDreamFlowEntryNode::StaticClass()))
    {
        return false;
    }

    const UDreamFlowNode* DefaultNode = Cast<UDreamFlowNode>(NodeClass->GetDefaultObject());
    return DefaultNode != nullptr && DefaultNode->IsUserCreatable() && DefaultNode->SupportsFlowAsset(FlowAsset);
}

void FDreamFlowEditorUtils::RebuildGraphFromAsset(UDreamFlowAsset* FlowAsset, UDreamFlowEdGraph* Graph)
{
    if (FlowAsset == nullptr || Graph == nullptr)
    {
        return;
    }

    Graph->Modify();
    Graph->Schema = UDreamFlowEdGraphSchema::StaticClass();
    Graph->SetFlowAsset(FlowAsset);

    if (FlowAsset->GetNodes().Num() == 0)
    {
        CreateDefaultEntryNode(FlowAsset, Graph);
        return;
    }

    Graph->Nodes.Reset();

    TMap<UDreamFlowNode*, UDreamFlowEdGraphNode*> GraphNodeByRuntimeNode;

    for (UDreamFlowNode* RuntimeNode : FlowAsset->GetNodes())
    {
        if (RuntimeNode == nullptr)
        {
            continue;
        }

        FGraphNodeCreator<UDreamFlowEdGraphNode> NodeCreator(*Graph);
        UDreamFlowEdGraphNode* GraphNode = NodeCreator.CreateNode(false);
        GraphNode->SetRuntimeNode(RuntimeNode);
#if WITH_EDITORONLY_DATA
        GraphNode->NodePosX = FMath::RoundToInt(RuntimeNode->EditorPosition.X);
        GraphNode->NodePosY = FMath::RoundToInt(RuntimeNode->EditorPosition.Y);
#else
        GraphNode->NodePosX = 0;
        GraphNode->NodePosY = 0;
#endif
        NodeCreator.Finalize();
        GraphNodeByRuntimeNode.Add(RuntimeNode, GraphNode);
    }

    for (UDreamFlowNode* RuntimeNode : FlowAsset->GetNodes())
    {
        UDreamFlowEdGraphNode* SourceGraphNode = GraphNodeByRuntimeNode.FindRef(RuntimeNode);
        if (SourceGraphNode == nullptr)
        {
            continue;
        }

        const TArray<FDreamFlowNodeOutputLink> OutputLinks = RuntimeNode->GetOutputLinksCopy();
        if (OutputLinks.Num() > 0)
        {
            for (const FDreamFlowNodeOutputLink& OutputLink : OutputLinks)
            {
                UDreamFlowEdGraphNode* TargetGraphNode = GraphNodeByRuntimeNode.FindRef(OutputLink.Child);
                UEdGraphPin* InputPin = TargetGraphNode ? TargetGraphNode->FindPin(UDreamFlowEdGraphNode::InputPinName, EGPD_Input) : nullptr;
                UEdGraphPin* OutputPin = SourceGraphNode->FindOutputPinByName(OutputLink.PinName);

                if (OutputPin == nullptr)
                {
                    OutputPin = SourceGraphNode->GetPrimaryOutputPin();
                }

                if (OutputPin != nullptr && InputPin != nullptr)
                {
                    OutputPin->MakeLinkTo(InputPin);
                }
            }
        }
        else
        {
            UEdGraphPin* OutputPin = SourceGraphNode->GetPrimaryOutputPin();
            if (OutputPin == nullptr)
            {
                continue;
            }

            for (UDreamFlowNode* ChildNode : RuntimeNode->Children)
            {
                UDreamFlowEdGraphNode* TargetGraphNode = GraphNodeByRuntimeNode.FindRef(ChildNode);
                UEdGraphPin* InputPin = TargetGraphNode ? TargetGraphNode->FindPin(UDreamFlowEdGraphNode::InputPinName, EGPD_Input) : nullptr;

                if (InputPin != nullptr)
                {
                    OutputPin->MakeLinkTo(InputPin);
                }
            }
        }
    }

    Graph->NotifyGraphChanged();
    SynchronizeAssetFromGraph(FlowAsset);
}

void FDreamFlowEditorUtils::CreateDefaultEntryNode(UDreamFlowAsset* FlowAsset, UDreamFlowEdGraph* Graph)
{
    if (FlowAsset == nullptr || Graph == nullptr)
    {
        return;
    }

    FlowAsset->Modify();
    Graph->Modify();

    UDreamFlowEntryNode* EntryNode = NewObject<UDreamFlowEntryNode>(FlowAsset, NAME_None, RF_Transactional);
    EntryNode->SetEditorPosition(FVector2D(0.0f, 0.0f));

    FGraphNodeCreator<UDreamFlowEdGraphNode> NodeCreator(*Graph);
    UDreamFlowEdGraphNode* EntryGraphNode = NodeCreator.CreateNode(false);
    EntryGraphNode->SetRuntimeNode(EntryNode);
    EntryGraphNode->NodePosX = 0;
    EntryGraphNode->NodePosY = 0;
    NodeCreator.Finalize();

    FlowAsset->ReplaceNodes({ EntryNode });
    FlowAsset->SetEntryNodeInternal(EntryNode);
    Graph->NotifyGraphChanged();
}

#undef LOCTEXT_NAMESPACE
