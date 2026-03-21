#pragma once

#include "Execution/DreamFlowDebuggerSubsystem.h"
#include "Containers/Ticker.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;
struct FToolMenuSection;
struct FGraphPanelPinConnectionFactory;

class DREAMFLOWEDITOR_API FDreamFlowEditorModule : public IModuleInterface
{
public:
    static FDreamFlowEditorModule& Get();
    static bool IsAvailable();

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void PausePlaySessionForDreamFlow();
    void ResumePlaySessionForDreamFlow();
    void SyncPlaySessionToDebuggerState(bool bShouldPause);
    bool IsPlaySessionPaused() const;

private:
    void RegisterAssetTools();
    void UnregisterAssetTools();
    void RegisterMenus();
    void AddDreamFlowAddNewMenu(FToolMenuSection& Section);
    void RegisterEditorCommands();
    void UnregisterEditorCommands();
    void RegisterConnectionFactory();
    void UnregisterConnectionFactory();
    void RegisterPropertyCustomizations();
    void UnregisterPropertyCustomizations();
    bool HandleDebuggerTicker(float DeltaTime);
    bool FocusBreakpointLocation(const FDreamFlowExecutionLocation& BreakpointLocation);

private:
    TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
    TSharedPtr<FGraphPanelPinConnectionFactory> ConnectionFactory;
    FTSTicker::FDelegateHandle DebuggerTickerHandle;
    uint64 LastHandledBreakpointHitSerial = 0;
    bool bHasPendingBreakpointFocus = false;
    bool bOwnsPlaySessionPause = false;
    FDreamFlowExecutionLocation PendingBreakpointFocus;
    uint32 AssetCategory = 0;
};
