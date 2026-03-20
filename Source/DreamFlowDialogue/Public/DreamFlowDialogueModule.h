#pragma once

#include "Modules/ModuleManager.h"

class DREAMFLOWDIALOGUE_API FDreamFlowDialogueModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
