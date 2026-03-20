#pragma once

#include "Modules/ModuleManager.h"

class DREAMFLOWQUEST_API FDreamFlowQuestModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
