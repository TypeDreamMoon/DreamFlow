#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UDreamFlowAsset;
class UDreamFlowNode;

class DREAMFLOWEDITOR_API SDreamFlowNodePalette : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnNodeClassPicked, TSubclassOf<UDreamFlowNode>);

    SLATE_BEGIN_ARGS(SDreamFlowNodePalette) {}
        SLATE_ARGUMENT(UDreamFlowAsset*, FlowAsset)
        SLATE_EVENT(FOnNodeClassPicked, OnNodeClassPicked)
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
        FString SearchText;
    };

    void RebuildPalette();
    void HandleSearchTextChanged(const FText& InSearchText);
    FReply HandleEntryClicked(TSubclassOf<UDreamFlowNode> NodeClass) const;
    TSharedRef<SWidget> BuildEntryWidget(const FPaletteEntry& Entry) const;

private:
    TWeakObjectPtr<UDreamFlowAsset> FlowAsset;
    FOnNodeClassPicked OnNodeClassPicked;
    TArray<FPaletteEntry> AllEntries;
    FString SearchText;
    TSharedPtr<class SScrollBox> EntryContainer;
};
