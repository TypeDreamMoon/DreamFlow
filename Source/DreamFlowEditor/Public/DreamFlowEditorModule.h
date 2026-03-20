#pragma once

#include "Modules/ModuleManager.h"

class IAssetTypeActions;
struct FGraphPanelPinConnectionFactory;

class DREAMFLOWEDITOR_API FDreamFlowEditorModule : public IModuleInterface
{
public:
    static FDreamFlowEditorModule& Get();
    static bool IsAvailable();

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterAssetTools();
    void UnregisterAssetTools();
    void RegisterConnectionFactory();
    void UnregisterConnectionFactory();
    void RegisterPropertyCustomizations();
    void UnregisterPropertyCustomizations();

private:
    TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
    TSharedPtr<FGraphPanelPinConnectionFactory> ConnectionFactory;
    uint32 AssetCategory = 0;
};
