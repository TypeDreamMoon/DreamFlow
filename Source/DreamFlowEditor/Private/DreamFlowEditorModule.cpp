#include "DreamFlowEditorModule.h"

#include "Connections/DreamFlowConnectionDrawingPolicy.h"
#include "DreamFlowAssetTypeActions.h"
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

    TSharedRef<FDreamFlowAssetTypeActions> FlowAssetActions = MakeShared<FDreamFlowAssetTypeActions>(AssetCategory);
    AssetTools.RegisterAssetTypeActions(FlowAssetActions);
    RegisteredAssetTypeActions.Add(FlowAssetActions);
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
