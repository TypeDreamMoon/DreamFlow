#pragma once

#include "Modules/ModuleManager.h"

class DREAMFLOW_API FDreamFlowModule : public IModuleInterface
{
public:
    static FDreamFlowModule& Get();
    static bool IsAvailable();

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
