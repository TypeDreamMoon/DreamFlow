#pragma once

#include "DreamFlowVariableTypes.h"
#include "IDetailCustomization.h"

class IPropertyHandle;
class IPropertyUtilities;
class SWidget;

class FDreamFlowVariablesEditorDataDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    TSharedRef<SWidget> BuildAddVariableMenu() const;
    void AddVariable(EDreamFlowValueType ValueType) const;

private:
    TSharedPtr<IPropertyHandle> VariablesHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
};
