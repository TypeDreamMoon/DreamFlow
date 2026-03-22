#include "DreamFlowNode.h"

#include "DreamFlowAsset.h"
#include "Engine/Engine.h"
#include "Execution/DreamFlowExecutor.h"
#include "Execution/DreamFlowDebuggerSubsystem.h"
#include "Engine/Texture2D.h"
#include "UObject/Field.h"
#include "UObject/Class.h"

UDreamFlowNode::UDreamFlowNode()
{
    NodeGuid = FGuid::NewGuid();
    Title = FText::FromString(TEXT("Flow Node"));
    Description = FText::GetEmpty();
    SupportedFlowAssetType = UDreamFlowAsset::StaticClass();

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.13f, 0.42f, 0.55f, 1.0f);
    NodeIconStyleName = NAME_None;
    EditorPosition = FVector2D::ZeroVector;
#endif
}

void UDreamFlowNode::PostLoad()
{
    Super::PostLoad();

    if (OutputLinks.Num() == 0 && Children.Num() > 0)
    {
        FixupLegacyOutputLinksFromChildren();
    }
    else if (OutputLinks.Num() > 0)
    {
        const TArray<FDreamFlowNodeOutputLink> ExistingOutputLinks = OutputLinks;
        SetOutputLinks(ExistingOutputLinks);
    }
}

FText UDreamFlowNode::GetNodeDisplayName_Implementation() const
{
    if (!Title.IsEmpty())
    {
        return Title;
    }

    return GetClass()->GetDisplayNameText();
}

FLinearColor UDreamFlowNode::GetNodeTint_Implementation() const
{
#if WITH_EDITORONLY_DATA
    return NodeTint;
#else
    return FLinearColor(0.13f, 0.42f, 0.55f, 1.0f);
#endif
}

FText UDreamFlowNode::GetNodeCategory_Implementation() const
{
    return FText::FromString(TEXT("Core"));
}

FText UDreamFlowNode::GetNodeAccentLabel_Implementation() const
{
    return FText::GetEmpty();
}

FName UDreamFlowNode::GetNodeIconStyleName_Implementation() const
{
#if WITH_EDITORONLY_DATA
    return NodeIconStyleName;
#else
    return NAME_None;
#endif
}

UTexture2D* UDreamFlowNode::GetNodeIconTexture_Implementation() const
{
#if WITH_EDITORONLY_DATA
    return NodeIconTexture.LoadSynchronous();
#else
    return nullptr;
#endif
}

TArray<FName> UDreamFlowNode::GetInlineEditablePropertyNames_Implementation() const
{
    TArray<FName> PropertyNames;

#if WITH_EDITOR
    for (TFieldIterator<FProperty> It(GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        const FProperty* Property = *It;
        if (Property != nullptr && Property->HasMetaData(TEXT("DreamFlowInlineEditable")))
        {
            PropertyNames.Add(Property->GetFName());
        }
    }
#endif

    return PropertyNames;
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowNode::GetNodeDisplayItems_Implementation() const
{
    return PreviewItems;
}

bool UDreamFlowNode::SupportsMultipleChildren_Implementation() const
{
    return true;
}

bool UDreamFlowNode::SupportsMultipleParents_Implementation() const
{
    return true;
}

TArray<FDreamFlowNodeOutputPin> UDreamFlowNode::GetOutputPins_Implementation() const
{
    FDreamFlowNodeOutputPin OutputPin;
    OutputPin.PinName = TEXT("Out");
    OutputPin.DisplayName = FText::FromString(TEXT("Next"));
    OutputPin.bAllowMultipleConnections = SupportsMultipleChildren();
    return { OutputPin };
}

bool UDreamFlowNode::IsEntryNode_Implementation() const
{
    return false;
}

bool UDreamFlowNode::IsUserCreatable_Implementation() const
{
    return true;
}

bool UDreamFlowNode::IsTerminalNode_Implementation() const
{
    return false;
}

EDreamFlowNodeTransitionMode UDreamFlowNode::GetTransitionMode_Implementation() const
{
    return TransitionMode;
}

FText UDreamFlowNode::GetTransitionModeLabel_Implementation() const
{
    return GetTransitionMode() == EDreamFlowNodeTransitionMode::Automatic
        ? FText::FromString(TEXT("Auto"))
        : FText::FromString(TEXT("Manual"));
}

bool UDreamFlowNode::CanEnterNode_Implementation(UObject* Context) const
{
    (void)Context;
    return true;
}

bool UDreamFlowNode::CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const
{
    return OtherNode != nullptr && OtherNode != this;
}

void UDreamFlowNode::ExecuteNode_Implementation(UObject* Context)
{
    (void)Context;
}

void UDreamFlowNode::ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor)
{
    (void)Executor;
    ExecuteNode(Context);
}

bool UDreamFlowNode::SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;
    (void)Executor;
    return GetTransitionMode() == EDreamFlowNodeTransitionMode::Automatic;
}

int32 UDreamFlowNode::ResolveAutomaticTransitionChildIndex_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    (void)Context;
    (void)Executor;
    return INDEX_NONE;
}

FName UDreamFlowNode::ResolveAutomaticTransitionOutputPin_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const
{
    const int32 ChildIndex = ResolveAutomaticTransitionChildIndex(Context, Executor);
    const TArray<FDreamFlowNodeOutputPin> OutputPins = GetOutputPins();

    if (OutputPins.IsValidIndex(ChildIndex))
    {
        return OutputPins[ChildIndex].PinName;
    }

    TSet<FName> ConnectedPinNames;
    for (const FDreamFlowNodeOutputLink& OutputLink : OutputLinks)
    {
        if (!OutputLink.PinName.IsNone() && OutputLink.Child != nullptr)
        {
            ConnectedPinNames.Add(OutputLink.PinName);
        }
    }

    if (ConnectedPinNames.Num() == 1)
    {
        TArray<FName> ConnectedPinArray = ConnectedPinNames.Array();
        return ConnectedPinArray[0];
    }

    return OutputPins.Num() == 1
        ? OutputPins[0].PinName
        : NAME_None;
}

bool UDreamFlowNode::ContinueFlow(UDreamFlowExecutor* Executor)
{
    return Executor != nullptr ? Executor->Advance() : false;
}

bool UDreamFlowNode::ContinueFlowFromOutputPin(UDreamFlowExecutor* Executor, FName OutputPinName)
{
    return Executor != nullptr ? Executor->MoveToOutputPin(OutputPinName) : false;
}

void UDreamFlowNode::SetChildren(const TArray<UDreamFlowNode*>& InChildren)
{
    Children.Reset();
    OutputLinks.Reset();

    const TArray<FDreamFlowNodeOutputPin> OutputPins = GetOutputPins();
    const FName DefaultPinName = OutputPins.Num() > 0 ? OutputPins[0].PinName : FName(TEXT("Out"));

    for (UDreamFlowNode* Child : InChildren)
    {
        if (Child != nullptr)
        {
            Children.Add(Child);

            FDreamFlowNodeOutputLink& OutputLink = OutputLinks.AddDefaulted_GetRef();
            OutputLink.PinName = DefaultPinName;
            OutputLink.Child = Child;
        }
    }
}

void UDreamFlowNode::FixupLegacyOutputLinksFromChildren()
{
    if (Children.Num() == 0 || OutputLinks.Num() > 0)
    {
        return;
    }

    const TArray<FDreamFlowNodeOutputPin> OutputPins = GetOutputPins();
    const FName DefaultPinName = OutputPins.Num() > 0 ? OutputPins[0].PinName : FName(TEXT("Out"));

    for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
    {
        UDreamFlowNode* Child = Children[ChildIndex];
        if (Child == nullptr)
        {
            continue;
        }

        FDreamFlowNodeOutputLink& OutputLink = OutputLinks.AddDefaulted_GetRef();
        OutputLink.PinName = OutputPins.IsValidIndex(ChildIndex)
            ? OutputPins[ChildIndex].PinName
            : DefaultPinName;
        OutputLink.Child = Child;
    }
}

void UDreamFlowNode::SetOutputLinks(const TArray<FDreamFlowNodeOutputLink>& InOutputLinks)
{
    OutputLinks.Reset();
    Children.Reset();

    for (const FDreamFlowNodeOutputLink& OutputLink : InOutputLinks)
    {
        OutputLinks.Add(OutputLink);

        if (OutputLink.Child != nullptr)
        {
            Children.AddUnique(OutputLink.Child);
        }
    }
}

TArray<UDreamFlowNode*> UDreamFlowNode::GetChildrenCopy() const
{
    TArray<UDreamFlowNode*> Result;
    Result.Reserve(Children.Num());

    for (UDreamFlowNode* Child : Children)
    {
        Result.Add(Child);
    }

    return Result;
}

TArray<FDreamFlowNodeOutputLink> UDreamFlowNode::GetOutputLinksCopy() const
{
    return OutputLinks;
}

TArray<UDreamFlowNode*> UDreamFlowNode::GetChildrenForOutputPin(FName OutputPinName) const
{
    TArray<UDreamFlowNode*> Result;

    for (const FDreamFlowNodeOutputLink& OutputLink : OutputLinks)
    {
        if (OutputLink.PinName == OutputPinName && OutputLink.Child != nullptr)
        {
            Result.Add(OutputLink.Child);
        }
    }

    return Result;
}

UDreamFlowNode* UDreamFlowNode::GetFirstChildForOutputPin(FName OutputPinName) const
{
    for (const FDreamFlowNodeOutputLink& OutputLink : OutputLinks)
    {
        if (OutputLink.PinName == OutputPinName && OutputLink.Child != nullptr)
        {
            return OutputLink.Child;
        }
    }

    return nullptr;
}

UDreamFlowAsset* UDreamFlowNode::GetOwningFlowAsset() const
{
    return GetTypedOuter<UDreamFlowAsset>();
}

TArray<UDreamFlowExecutor*> UDreamFlowNode::GetActiveExecutors() const
{
    if (GEngine == nullptr)
    {
        return TArray<UDreamFlowExecutor*>();
    }

    if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
    {
        return DebuggerSubsystem->GetExecutorsForAsset(GetOwningFlowAsset());
    }

    return TArray<UDreamFlowExecutor*>();
}

TArray<UDreamFlowExecutor*> UDreamFlowNode::GetExecutorsOnThisNode() const
{
    if (GEngine == nullptr)
    {
        return TArray<UDreamFlowExecutor*>();
    }

    if (UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>())
    {
        return DebuggerSubsystem->GetExecutorsForNode(this);
    }

    return TArray<UDreamFlowExecutor*>();
}

TSubclassOf<UDreamFlowAsset> UDreamFlowNode::GetSupportedFlowAssetType() const
{
    if (SupportedFlowAssetType != nullptr)
    {
        return SupportedFlowAssetType;
    }

    return UDreamFlowAsset::StaticClass();
}

bool UDreamFlowNode::SupportsFlowAsset(const UDreamFlowAsset* FlowAsset) const
{
    return SupportsFlowAssetClass(FlowAsset != nullptr ? FlowAsset->GetClass() : UDreamFlowAsset::StaticClass());
}

bool UDreamFlowNode::SupportsFlowAssetClass(const UClass* FlowAssetClass) const
{
    const UClass* RequiredAssetClass = GetSupportedFlowAssetType().Get();
    const UClass* ActualAssetClass = FlowAssetClass != nullptr ? FlowAssetClass : UDreamFlowAsset::StaticClass();
    return RequiredAssetClass != nullptr && ActualAssetClass != nullptr && ActualAssetClass->IsChildOf(RequiredAssetClass);
}

bool UDreamFlowNode::CanAcceptChild(const UDreamFlowNode* OtherNode) const
{
    if (!CanConnectTo(OtherNode))
    {
        return false;
    }

    if (!SupportsMultipleChildren() && Children.Num() > 0 && !Children.Contains(OtherNode))
    {
        return false;
    }

    return true;
}

void UDreamFlowNode::ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const
{
    (void)OwningAsset;
    (void)OutMessages;
}

#if WITH_EDITOR
void UDreamFlowNode::SetEditorPosition(const FVector2D& InPosition)
{
#if WITH_EDITORONLY_DATA
    EditorPosition = InPosition;
#else
    (void)InPosition;
#endif
}
#endif
