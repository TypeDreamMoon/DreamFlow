#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "DreamFlowAssetFactory.generated.h"

UCLASS()
class DREAMFLOWEDITOR_API UDreamFlowAssetFactory : public UFactory
{
    GENERATED_BODY()

public:
    UDreamFlowAssetFactory();

    virtual UObject* FactoryCreateNew(
        UClass* InClass,
        UObject* InParent,
        FName InName,
        EObjectFlags Flags,
        UObject* Context,
        FFeedbackContext* Warn,
        FName CallingContext) override;

    virtual bool ShouldShowInNewMenu() const override;
    virtual uint32 GetMenuCategories() const override;
    virtual FText GetDisplayName() const override;
};

UCLASS()
class DREAMFLOWEDITOR_API UDreamQuestFlowAssetFactory : public UDreamFlowAssetFactory
{
    GENERATED_BODY()

public:
    UDreamQuestFlowAssetFactory();

    virtual FText GetDisplayName() const override;
};

UCLASS()
class DREAMFLOWEDITOR_API UDreamDialogueFlowAssetFactory : public UDreamFlowAssetFactory
{
    GENERATED_BODY()

public:
    UDreamDialogueFlowAssetFactory();

    virtual FText GetDisplayName() const override;
};
