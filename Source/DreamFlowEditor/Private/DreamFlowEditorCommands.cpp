#include "DreamFlowEditorCommands.h"

#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "DreamFlowEditorCommands"

FDreamFlowEditorCommands::FDreamFlowEditorCommands()
    : TCommands<FDreamFlowEditorCommands>(
        TEXT("DreamFlowEditor"),
        LOCTEXT("DreamFlowEditorCommandsContext", "Dream Flow Editor"),
        NAME_None,
        FAppStyle::GetAppStyleSetName())
{
}

void FDreamFlowEditorCommands::RegisterCommands()
{
    UI_COMMAND(
        ToggleBreakpoint,
        "Toggle Breakpoint",
        "Enable or disable a breakpoint on the selected DreamFlow node.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::F9));
}

#undef LOCTEXT_NAMESPACE
