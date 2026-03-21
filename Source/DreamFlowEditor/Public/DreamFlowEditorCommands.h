#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class DREAMFLOWEDITOR_API FDreamFlowEditorCommands : public TCommands<FDreamFlowEditorCommands>
{
public:
    FDreamFlowEditorCommands();

    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> ToggleBreakpoint;
    TSharedPtr<FUICommandInfo> ValidateFlow;
};
