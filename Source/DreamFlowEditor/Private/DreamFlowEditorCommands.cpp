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
        AnalyzeReferences,
        "Analyze References",
        "Show a reference report for node classes, variable bindings, execution-context properties, and sub-flow usage in the current flow asset.",
        EUserInterfaceActionType::Button,
        FInputChord());

    UI_COMMAND(
        QuickCreateNode,
        "Create Node Class",
        "Pick a DreamFlow node parent class and create a Blueprint implementation asset in the current content browser path.",
        EUserInterfaceActionType::Button,
        FInputChord());

    UI_COMMAND(
        ValidateFlow,
        "Validate",
        "Run DreamFlow validation for the current asset.",
        EUserInterfaceActionType::Button,
        FInputChord());

    UI_COMMAND(
        ToggleBreakpoint,
        "Toggle Breakpoint",
        "Enable or disable a breakpoint on the selected DreamFlow node.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::F9));
}

#undef LOCTEXT_NAMESPACE
