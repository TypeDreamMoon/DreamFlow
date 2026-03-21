#include "DreamFlowEditorModule.h"

#include "Connections/DreamFlowConnectionDrawingPolicy.h"
#include "Customizations/DreamFlowObjectDetailsCustomization.h"
#include "Customizations/DreamFlowValueBindingCustomization.h"
#include "Customizations/DreamFlowValueCustomization.h"
#include "Customizations/DreamFlowVariableDefinitionCustomization.h"
#include "Customizations/DreamFlowVariablesEditorDataDetails.h"
#include "DreamFlowAsset.h"
#include "DreamFlowAssetTypeActions.h"
#include "DreamFlowEditorCommands.h"
#include "DreamFlowEditorToolkit.h"
#include "DreamFlowEditorUtils.h"
#include "DreamFlowNode.h"
#include "DreamFlowVariablesEditorData.h"
#include "AssetToolsModule.h"
#include "ContentBrowserDataMenuContexts.h"
#include "ContentBrowserDataSubsystem.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "IContentBrowserDataModule.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "DreamFlowEditorModule"

namespace DreamFlowEditorModule
{
    static FString ResolveTargetPath(const UContentBrowserDataMenuContext_AddNewMenu* Context)
    {
        if (Context != nullptr)
        {
            UContentBrowserDataSubsystem* ContentBrowserData = IContentBrowserDataModule::Get().GetSubsystem();
            if (ContentBrowserData != nullptr)
            {
                for (const FName SelectedPath : Context->SelectedPaths)
                {
                    FName InternalPath;
                    if (ContentBrowserData->TryConvertVirtualPath(SelectedPath, InternalPath) == EContentBrowserPathType::Internal)
                    {
                        return InternalPath.ToString();
                    }
                }
            }
        }

        return FDreamFlowEditorUtils::GetCurrentContentBrowserPath();
    }

    static bool ExecuteQuickCreateNode(const FString& TargetPath)
    {
        const TSubclassOf<UDreamFlowNode> NodeClass = FDreamFlowEditorUtils::PickNodeClass();
        return NodeClass != nullptr
            && FDreamFlowEditorUtils::CreateNodeBlueprintAsset(NodeClass, TargetPath, true) != nullptr;
    }

    static UDreamFlowAsset* ResolveEditorFlowAsset(UDreamFlowAsset* FlowAsset)
    {
        if (FlowAsset == nullptr)
        {
            return nullptr;
        }

        const FString StrippedPath = UWorld::RemovePIEPrefix(FlowAsset->GetPathName());
        if (StrippedPath.IsEmpty() || StrippedPath == FlowAsset->GetPathName())
        {
            return FlowAsset;
        }

        if (UDreamFlowAsset* ExistingAsset = FindObject<UDreamFlowAsset>(nullptr, *StrippedPath))
        {
            return ExistingAsset;
        }

        if (UDreamFlowAsset* LoadedAsset = LoadObject<UDreamFlowAsset>(nullptr, *StrippedPath))
        {
            return LoadedAsset;
        }

        return FlowAsset;
    }
}

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
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDreamFlowEditorModule::RegisterMenus));
    RegisterEditorCommands();
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

    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    UnregisterPropertyCustomizations();
    UnregisterConnectionFactory();
    UnregisterEditorCommands();
    UnregisterAssetTools();
}

void FDreamFlowEditorModule::PausePlaySessionForDreamFlow()
{
    if (GEditor == nullptr || GEditor->PlayWorld == nullptr)
    {
        return;
    }

    if (!GEditor->PlayWorld->bDebugPauseExecution && GEditor->SetPIEWorldsPaused(true))
    {
        GEditor->PlaySessionPaused();
        bOwnsPlaySessionPause = true;
    }
}

void FDreamFlowEditorModule::ResumePlaySessionForDreamFlow()
{
    if (!bOwnsPlaySessionPause)
    {
        return;
    }

    if (GEditor == nullptr || GEditor->PlayWorld == nullptr)
    {
        bOwnsPlaySessionPause = false;
        return;
    }

    if (GEditor->PlayWorld->bDebugPauseExecution)
    {
        GEditor->SetPIEWorldsPaused(false);
        GEditor->PlaySessionResumed();

        uint32 UserIndex = 0;
        FSlateApplication::Get().SetUserFocusToGameViewport(UserIndex);
    }

    bOwnsPlaySessionPause = false;
}

void FDreamFlowEditorModule::SyncPlaySessionToDebuggerState(bool bShouldPause)
{
    if (bShouldPause)
    {
        PausePlaySessionForDreamFlow();
    }
    else
    {
        ResumePlaySessionForDreamFlow();
    }
}

bool FDreamFlowEditorModule::IsPlaySessionPaused() const
{
    return GEditor != nullptr && GEditor->PlayWorld != nullptr && GEditor->PlayWorld->bDebugPauseExecution;
}

void FDreamFlowEditorModule::RegisterAssetTools()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("DreamFlow")), LOCTEXT("DreamFlowAssetCategory", "DreamFlow"));

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

void FDreamFlowEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AddNewContextMenu"))
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("ContentBrowserNewAsset");
        Section.AddDynamicEntry(
            "DreamFlowAddNewNodeClass",
            FNewToolMenuSectionDelegate::CreateRaw(this, &FDreamFlowEditorModule::AddDreamFlowAddNewMenu));
    }
}

void FDreamFlowEditorModule::AddDreamFlowAddNewMenu(FToolMenuSection& Section)
{
    const UContentBrowserDataMenuContext_AddNewMenu* Context = Section.FindContext<UContentBrowserDataMenuContext_AddNewMenu>();
    if (Context == nullptr || !Context->bCanBeModified || !Context->bContainsValidPackagePath)
    {
        return;
    }

    const FString TargetPath = DreamFlowEditorModule::ResolveTargetPath(Context);

    Section.AddSubMenu(
        "DreamFlowAddNewActions",
        LOCTEXT("DreamFlowAddNewMenu", "DreamFlow"),
        LOCTEXT("DreamFlowAddNewMenuTooltip", "Create DreamFlow assets in the current content path."),
        FNewToolMenuDelegate::CreateLambda([TargetPath](UToolMenu* Menu)
        {
            FToolMenuSection& DreamFlowSection = Menu->AddSection("DreamFlowAddNewSection", LOCTEXT("DreamFlowAddNewSection", "DreamFlow"));
            DreamFlowSection.AddMenuEntry(
                "DreamFlow_CreateNodeClass",
                LOCTEXT("DreamFlowCreateNodeClass", "Create Node Class..."),
                LOCTEXT("DreamFlowCreateNodeClassTooltip", "Pick a DreamFlow node parent class and create a Blueprint implementation in the current content browser path."),
                FSlateIcon(),
                FUIAction(
                    FExecuteAction::CreateLambda([TargetPath]()
                    {
                        DreamFlowEditorModule::ExecuteQuickCreateNode(TargetPath);
                    }),
                    FCanExecuteAction::CreateLambda([TargetPath]()
                    {
                        return !TargetPath.IsEmpty();
                    })));
        }));
}

void FDreamFlowEditorModule::RegisterEditorCommands()
{
    if (!FDreamFlowEditorCommands::IsRegistered())
    {
        FDreamFlowEditorCommands::Register();
    }
}

void FDreamFlowEditorModule::UnregisterEditorCommands()
{
    if (FDreamFlowEditorCommands::IsRegistered())
    {
        FDreamFlowEditorCommands::Unregister();
    }
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
    PropertyEditorModule.RegisterCustomClassLayout(UDreamFlowAsset::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FDreamFlowAssetDetailsCustomization::MakeInstance));
    PropertyEditorModule.RegisterCustomClassLayout(UDreamFlowNode::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FDreamFlowNodeDetailsCustomization::MakeInstance));
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
    PropertyEditorModule.UnregisterCustomClassLayout(UDreamFlowAsset::StaticClass()->GetFName());
    PropertyEditorModule.UnregisterCustomClassLayout(UDreamFlowNode::StaticClass()->GetFName());
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

    if (GEditor == nullptr || GEditor->PlayWorld == nullptr)
    {
        bOwnsPlaySessionPause = false;
    }
    else if (!GEditor->PlayWorld->bDebugPauseExecution)
    {
        bOwnsPlaySessionPause = false;
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

    PausePlaySessionForDreamFlow();

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

    return FDreamFlowEditorToolkit::OpenAssetAndFocusNode(
        DreamFlowEditorModule::ResolveEditorFlowAsset(BreakpointLocation.FlowAsset.Get()),
        BreakpointLocation.NodeGuid);
}

IMPLEMENT_MODULE(FDreamFlowEditorModule, DreamFlowEditor)

#undef LOCTEXT_NAMESPACE
