#include "DreamFlowAssetTypeActions.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEditorToolkit.h"

#define LOCTEXT_NAMESPACE "DreamFlowAssetTypeActions"

FDreamFlowAssetTypeActions::FDreamFlowAssetTypeActions(uint32 InAssetCategory)
    : AssetCategory(InAssetCategory)
{
}

FText FDreamFlowAssetTypeActions::GetName() const
{
    return LOCTEXT("DreamFlowAssetName", "Dream Flow");
}

FColor FDreamFlowAssetTypeActions::GetTypeColor() const
{
    return FColor(222, 126, 40);
}

UClass* FDreamFlowAssetTypeActions::GetSupportedClass() const
{
    return UDreamFlowAsset::StaticClass();
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
