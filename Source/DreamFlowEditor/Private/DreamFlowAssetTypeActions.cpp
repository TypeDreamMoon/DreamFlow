#include "DreamFlowAssetTypeActions.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEditorToolkit.h"

#define LOCTEXT_NAMESPACE "DreamFlowAssetTypeActions"

FDreamFlowAssetTypeActions::FDreamFlowAssetTypeActions(uint32 InAssetCategory, UClass* InSupportedClass, FText InDisplayName, FColor InTypeColor)
    : AssetCategory(InAssetCategory)
    , SupportedClass(InSupportedClass)
    , DisplayName(MoveTemp(InDisplayName))
    , TypeColor(InTypeColor)
{
}

FText FDreamFlowAssetTypeActions::GetName() const
{
    return DisplayName;
}

FColor FDreamFlowAssetTypeActions::GetTypeColor() const
{
    return TypeColor;
}

UClass* FDreamFlowAssetTypeActions::GetSupportedClass() const
{
    return SupportedClass;
}

uint32 FDreamFlowAssetTypeActions::GetCategories()
{
    return AssetCategory;
}

void FDreamFlowAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    const EToolkitMode::Type ToolkitMode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

    for (UObject* Object : InObjects)
    {
        if (UDreamFlowAsset* FlowAsset = Cast<UDreamFlowAsset>(Object))
        {
            TSharedRef<FDreamFlowEditorToolkit> EditorToolkit = MakeShared<FDreamFlowEditorToolkit>();
            EditorToolkit->InitEditor(ToolkitMode, EditWithinLevelEditor, FlowAsset);
        }
    }
}

#undef LOCTEXT_NAMESPACE
