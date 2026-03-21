#include "DreamFlowEditorModule.h"

#include "Connections/DreamFlowConnectionDrawingPolicy.h"
#include "Customizations/DreamFlowValueBindingCustomization.h"
#include "Customizations/DreamFlowValueCustomization.h"
#include "Customizations/DreamFlowVariableDefinitionCustomization.h"
#include "Customizations/DreamFlowVariablesEditorDataDetails.h"
#include "DreamDialogueFlowAsset.h"
#include "DreamFlowAsset.h"
#include "DreamFlowAssetTypeActions.h"
#include "DreamFlowEditorToolkit.h"
#include "DreamFlowVariablesEditorData.h"
#include "DreamQuestFlowAsset.h"
#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

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
    RegisterPropertyCustomizations();

    if (!DebuggerTickerHandle.IsValid())
    {
        DebuggerTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateRaw(this, &FDreamFlowEditorModule::HandleDebuggerTicker));
    }
}

void FDreamFlowEditorModule::ShutdownModule()
{
    if (DebuggerTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(DebuggerTickerHandle);
        DebuggerTickerHandle.Reset();
    }

    UnregisterPropertyCustomizations();
    UnregisterConnectionFactory();
    UnregisterAssetTools();
}

void FDreamFlowEditorModule::RegisterAssetTools()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Dream")), LOCTEXT("DreamAssetCategory", "Dream"));

    const auto RegisterTypeAction = [this, &AssetTools](UClass* SupportedClass, const FText& DisplayName, const FColor& TypeColor)
    {
        TSharedRef<FDreamFlowAssetTypeActions> AssetTypeActions = MakeShared<FDreamFlowAssetTypeActions>(
            AssetCategory,
            SupportedClass,
            DisplayName,
            TypeColor);
        AssetTools.RegisterAssetTypeActions(AssetTypeActions);
        RegisteredAssetTypeActions.Add(AssetTypeActions);
    };

    RegisterTypeAction(UDreamFlowAsset::StaticClass(), LOCTEXT("DreamFlowAssetName", "Dream Flow"), FColor(222, 126, 40));
    RegisterTypeAction(UDreamQuestFlowAsset::StaticClass(), LOCTEXT("DreamQuestFlowAssetName", "Dream Quest Flow"), FColor(61, 158, 111));
    RegisterTypeAction(UDreamDialogueFlowAsset::StaticClass(), LOCTEXT("DreamDialogueFlowAssetName", "Dream Dialogue Flow"), FColor(67, 113, 217));
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

void FDreamFlowEditorModule::RegisterPropertyCustomizations()
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyEditorModule.RegisterCustomPropertyTypeLayout(TEXT("DreamFlowValue"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDreamFlowValueCustomization::MakeInstance));
    PropertyEditorModule.RegisterCustomPropertyTypeLayout(TEXT("DreamFlowValueBinding"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDreamFlowValueBindingCustomization::MakeInstance));
    PropertyEditorModule.RegisterCustomPropertyTypeLayout(TEXT("DreamFlowVariableDefinition"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDreamFlowVariableDefinitionCustomization::MakeInstance));
    PropertyEditorModule.RegisterCustomClassLayout(UDreamFlowVariablesEditorData::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FDreamFlowVariablesEditorDataDetails::MakeInstance));
    PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FDreamFlowEditorModule::UnregisterPropertyCustomizations()
{
    if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        return;
    }

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TEXT("DreamFlowValue"));
    PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TEXT("DreamFlowValueBinding"));
    PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TEXT("DreamFlowVariableDefinition"));
    PropertyEditorModule.UnregisterCustomClassLayout(UDreamFlowVariablesEditorData::StaticClass()->GetFName());
    PropertyEditorModule.NotifyCustomizationModuleChanged();
}

bool FDreamFlowEditorModule::HandleDebuggerTicker(float DeltaTime)
{
    (void)DeltaTime;

    if (bHasPendingBreakpointFocus)
    {
        if (!PendingBreakpointFocus.IsValid() || FocusBreakpointLocation(PendingBreakpointFocus))
        {
            bHasPendingBreakpointFocus = false;
            PendingBreakpointFocus.Reset();
        }
    }

    if (GEditor == nullptr || GEngine == nullptr)
    {
        return true;
    }

    UDreamFlowDebuggerSubsystem* DebuggerSubsystem = GEngine->GetEngineSubsystem<UDreamFlowDebuggerSubsystem>();
    if (DebuggerSubsystem == nullptr)
    {
        return true;
    }

    const uint64 BreakpointHitSerial = DebuggerSubsystem->GetBreakpointHitSerial();
    if (BreakpointHitSerial == LastHandledBreakpointHitSerial)
    {
        return true;
    }

    LastHandledBreakpointHitSerial = BreakpointHitSerial;

    FDreamFlowExecutionLocation BreakpointLocation;
    if (!DebuggerSubsystem->GetMostRecentBreakpointHit(BreakpointLocation))
    {
        return true;
    }

    if (GEditor->PlayWorld != nullptr && !GEditor->PlayWorld->bDebugPauseExecution)
    {
        GEditor->SetPIEWorldsPaused(true);
        GEditor->PlaySessionPaused();
    }

    if (!FocusBreakpointLocation(BreakpointLocation))
    {
        bHasPendingBreakpointFocus = true;
        PendingBreakpointFocus = BreakpointLocation;
    }

    return true;
}

bool FDreamFlowEditorModule::FocusBreakpointLocation(const FDreamFlowExecutionLocation& BreakpointLocation)
{
    if (!BreakpointLocation.IsValid())
    {
        return false;
    }

    return FDreamFlowEditorToolkit::OpenAssetAndFocusNode(BreakpointLocation.FlowAsset.Get(), BreakpointLocation.NodeGuid);
}

IMPLEMENT_MODULE(FDreamFlowEditorModule, DreamFlowEditor)

#undef LOCTEXT_NAMESPACE
