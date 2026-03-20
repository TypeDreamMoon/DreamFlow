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
