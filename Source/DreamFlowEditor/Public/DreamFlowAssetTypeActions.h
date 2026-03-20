#pragma once

#include "AssetTypeActions_Base.h"

class FDreamFlowAssetTypeActions : public FAssetTypeActions_Base
{
public:
    explicit FDreamFlowAssetTypeActions(uint32 InAssetCategory);

    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;
    virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor) override;

private:
    uint32 AssetCategory;
};
