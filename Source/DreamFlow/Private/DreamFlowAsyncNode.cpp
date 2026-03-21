#include "DreamFlowAsyncNode.h"

#include "Execution/DreamFlowAsyncContext.h"
#include "Execution/DreamFlowExecutor.h"

namespace
{
    static FDreamFlowNodeDisplayItem MakeAsyncPreviewItem(const FText& Label, const FString& Value)
    {
        FDreamFlowNodeDisplayItem Item;
        Item.Type = EDreamFlowNodeDisplayItemType::Text;
        Item.Label = Label;
        Item.TextValue = FText::FromString(Value);
        return Item;
    }
}

UDreamFlowAsyncNode::UDreamFlowAsyncNode()
{
    Title = FText::FromString(TEXT("Async Node"));
    Description = FText::FromString(TEXT("Starts asynchronous work and resumes when the async context reports completion."));

#if WITH_EDITORONLY_DATA
    NodeTint = FLinearColor(0.66f, 0.28f, 0.74f, 1.0f);
    NodeIconStyleName = TEXT("Icons.Time");
#endif
}

FText UDreamFlowAsyncNode::GetNodeDisplayName_Implementation() const
{
    return Title;
}

FLinearColor UDreamFlowAsyncNode::GetNodeTint_Implementation() const
{
    return FLinearColor(0.66f, 0.28f, 0.74f, 1.0f);
}

FText UDreamFlowAsyncNode::GetNodeAccentLabel_Implementation() const
{
    return FText::FromString(TEXT("Async"));
}

TArray<FDreamFlowNodeDisplayItem> UDreamFlowAsyncNode::GetNodeDisplayItems_Implementation() const
{
    TArray<FDreamFlowNodeDisplayItem> Items = Super::GetNodeDisplayItems_Implementation();
    Items.Add(MakeAsyncPreviewItem(FText::FromString(TEXT("Behavior")), TEXT("Wait for async completion")));
    return Items;
}

TArray<FDreamFlowNodeOutputPin> UDreamFlowAsyncNode::GetOutputPins_Implementation() const
{
    FDreamFlowNodeOutputPin CompletedPin;
    CompletedPin.PinName = TEXT("Completed");
    CompletedPin.DisplayName = FText::FromString(TEXT("Completed"));
    return { CompletedPin };
}

void UDreamFlowAsyncNode::ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor)
{
    if (Executor == nullptr)
    {
        return;
    }

    if (UDreamFlowAsyncContext* AsyncContext = Executor->BeginAsyncNode(this))
    {
        StartAsyncNode(Context, Executor, AsyncContext);
    }
}

void UDreamFlowAsyncNode::StartAsyncNode_Implementation(UObject* Context, UDreamFlowExecutor* Executor, UDreamFlowAsyncContext* AsyncContext)
{
    (void)Context;
    (void)Executor;
    (void)AsyncContext;
}
