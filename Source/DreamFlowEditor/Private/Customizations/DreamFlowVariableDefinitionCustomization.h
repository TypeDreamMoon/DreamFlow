#pragma once

#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class IPropertyUtilities;

class FDreamFlowVariableDefinitionCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
    FText GetHeaderText() const;

private:
    TSharedPtr<IPropertyHandle> NameHandle;
    TSharedPtr<IPropertyHandle> DescriptionHandle;
    TSharedPtr<IPropertyHandle> DefaultValueHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
};
