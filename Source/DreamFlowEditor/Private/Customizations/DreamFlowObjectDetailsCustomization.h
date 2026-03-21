#pragma once

#include "IDetailCustomization.h"

class IPropertyHandle;
class IPropertyUtilities;
class SWidget;
class UDreamFlowAsset;
class UObject;

class FDreamFlowAssetDetailsCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

class FDreamFlowNodeDetailsCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    void CustomizeFlowVariablePickerProperties(IDetailLayoutBuilder& DetailBuilder) const;
    UDreamFlowAsset* ResolveOwningFlowAsset(const TArray<TWeakObjectPtr<UObject>>& CustomizedObjects) const;
    TSharedRef<SWidget> BuildFlowVariablePickerWidget(const TSharedPtr<IPropertyHandle>& PropertyHandle, const UDreamFlowAsset* FlowAsset) const;
    TSharedRef<SWidget> BuildFlowVariablePickerMenu(const TSharedPtr<IPropertyHandle>& PropertyHandle, const UDreamFlowAsset* FlowAsset) const;
    FText GetFlowVariablePickerLabel(TSharedPtr<IPropertyHandle> PropertyHandle, const UDreamFlowAsset* FlowAsset) const;
};
