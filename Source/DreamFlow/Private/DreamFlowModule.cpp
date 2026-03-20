#include "DreamFlowModule.h"

#include "Modules/ModuleManager.h"

FDreamFlowModule& FDreamFlowModule::Get()
{
    return FModuleManager::LoadModuleChecked<FDreamFlowModule>("DreamFlow");
}

bool FDreamFlowModule::IsAvailable()
{
    return FModuleManager::Get().IsModuleLoaded("DreamFlow");
}

void FDreamFlowModule::StartupModule()
{
}

void FDreamFlowModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FDreamFlowModule, DreamFlow)
