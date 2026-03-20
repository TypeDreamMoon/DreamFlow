#include "DreamFlowEditorModule.h"

#include "Connections/DreamFlowConnectionDrawingPolicy.h"
#include "DreamDialogueFlowAsset.h"
#include "DreamFlowAsset.h"
#include "DreamFlowAssetTypeActions.h"
#include "DreamQuestFlowAsset.h"
#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "DreamFlowEditorModule"

FDreamFlowEditorModule& FDreamFlowEditorModule::Get()
{
    return FModuleManager::LoadModuleChecked<FDreamFlowEditorModule>("DreamFlowEditor");
}

bool FDreamFlowEditorModule::IsAvailable()
{
    return FModuleManager::Get().IsModuleLoaded("DreamFlowEditor");
}

void FDreamFlowEditorModule::StartupModule()
{
    RegisterAssetTools();
    RegisterConnectionFactory();
}

void FDreamFlowEditorModule::ShutdownModule()
{
    UnregisterConnectionFactory();
    UnregisterAssetTools();
}

void FDreamFlowEditorModule::RegisterAssetTools()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Dream")), LOCTEXT("DreamAssetCategory", "Dream"));

    const auto RegisterTypeAction = [this, &AssetTools](UClass* SupportedClass, const FText& DisplayName, const FColor& TypeColor)
    {
        TSharedRef<FDreamFlowAssetTypeActions> AssetTypeActions = MakeShared<FDreamFlowAssetTypeActions>(
            AssetCategory,
            SupportedClass,
            DisplayName,
            TypeColor);
        AssetTools.RegisterAssetTypeActions(AssetTypeActions);
        RegisteredAssetTypeActions.Add(AssetTypeActions);
    };

    RegisterTypeAction(UDreamFlowAsset::StaticClass(), LOCTEXT("DreamFlowAssetName", "Dream Flow"), FColor(222, 126, 40));
    RegisterTypeAction(UDreamQuestFlowAsset::StaticClass(), LOCTEXT("DreamQuestFlowAssetName", "Dream Quest Flow"), FColor(61, 158, 111));
    RegisterTypeAction(UDreamDialogueFlowAsset::StaticClass(), LOCTEXT("DreamDialogueFlowAssetName", "Dream Dialogue Flow"), FColor(67, 113, 217));
}

void FDreamFlowEditorModule::UnregisterAssetTools()
{
    if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        RegisteredAssetTypeActions.Reset();
        return;
    }

    IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

    for (const TSharedPtr<IAssetTypeActions>& AssetTypeAction : RegisteredAssetTypeActions)
    {
        if (AssetTypeAction.IsValid())
        {
            AssetTools.UnregisterAssetTypeActions(AssetTypeAction.ToSharedRef());
        }
    }

    RegisteredAssetTypeActions.Reset();
}

void FDreamFlowEditorModule::RegisterConnectionFactory()
{
    if (!ConnectionFactory.IsValid())
    {
        ConnectionFactory = MakeShared<FDreamFlowConnectionFactory>();
        FEdGraphUtilities::RegisterVisualPinConnectionFactory(ConnectionFactory);
    }
}

void FDreamFlowEditorModule::UnregisterConnectionFactory()
{
    if (ConnectionFactory.IsValid())
    {
        FEdGraphUtilities::UnregisterVisualPinConnectionFactory(ConnectionFactory);
        ConnectionFactory.Reset();
    }
}

IMPLEMENT_MODULE(FDreamFlowEditorModule, DreamFlowEditor)

#undef LOCTEXT_NAMESPACE
