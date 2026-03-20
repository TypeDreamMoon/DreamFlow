#include "DreamFlowAssetFactory.h"

#include "DreamFlowAsset.h"
#include "DreamFlowEditorUtils.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "DreamFlowAssetFactory"

UDreamFlowAssetFactory::UDreamFlowAssetFactory()
{
    bCreateNew = true;
    bEditAfterNew = true;
    SupportedClass = UDreamFlowAsset::StaticClass();
}

UObject* UDreamFlowAssetFactory::FactoryCreateNew(
    UClass* InClass,
    UObject* InParent,
    FName InName,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn,
    FName CallingContext)
{
    (void)Context;
    (void)Warn;
    (void)CallingContext;

    if (!InClass->IsChildOf(UDreamFlowAsset::StaticClass()))
    {
        return nullptr;
    }

    UDreamFlowAsset* NewAsset = NewObject<UDreamFlowAsset>(InParent, InClass, InName, Flags | RF_Transactional);
    FDreamFlowEditorUtils::GetOrCreateGraph(NewAsset);
    return NewAsset;
}

bool UDreamFlowAssetFactory::ShouldShowInNewMenu() const
{
    return true;
}

uint32 UDreamFlowAssetFactory::GetMenuCategories() const
{
    return EAssetTypeCategories::Misc;
}

FText UDreamFlowAssetFactory::GetDisplayName() const
{
    return LOCTEXT("DreamFlowAssetDisplayName", "Dream Flow");
}

#undef LOCTEXT_NAMESPACE
