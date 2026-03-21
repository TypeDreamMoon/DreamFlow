#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FDragDropOperation;
struct FPointerEvent;
struct FSlateBrush;
class UDreamFlowAsset;
class UDreamFlowNode;
struct FGeometry;

class DREAMFLOWEDITOR_API SDreamFlowNodePalette : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnNodeClassPicked, TSubclassOf<UDreamFlowNode>);
    DECLARE_DELEGATE_TwoParams(FOnNodeClassDropped, TSubclassOf<UDreamFlowNode>, const FVector2f&);

    SLATE_BEGIN_ARGS(SDreamFlowNodePalette) {}
        SLATE_ARGUMENT(UDreamFlowAsset*, FlowAsset)
        SLATE_EVENT(FOnNodeClassPicked, OnNodeClassPicked)
        SLATE_EVENT(FOnNodeClassDropped, OnNodeClassDropped)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void RefreshNodeClasses();

private:
    struct FPaletteEntry
    {
        TSubclassOf<UDreamFlowNode> NodeClass;
        FText DisplayName;
        FText Category;
        FText AccentLabel;
        FLinearColor Tint = FLinearColor::White;
        const FSlateBrush* IconBrush = nullptr;
        TSharedPtr<FSlateBrush> CustomIconBrush;
        FString SearchText;
    };

    void RebuildPalette();
    void HandleSearchTextChanged(const FText& InSearchText);
    FReply HandleEntryClicked(TSubclassOf<UDreamFlowNode> NodeClass) const;
    FReply HandleEntryDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TSubclassOf<UDreamFlowNode> NodeClass) const;
    TSharedRef<SWidget> BuildEntryWidget(const FPaletteEntry& Entry) const;

private:
    TWeakObjectPtr<UDreamFlowAsset> FlowAsset;
    FOnNodeClassPicked OnNodeClassPicked;
    FOnNodeClassDropped OnNodeClassDropped;
    TArray<FPaletteEntry> AllEntries;
    FString SearchText;
    TSharedPtr<class SScrollBox> EntryContainer;
};
