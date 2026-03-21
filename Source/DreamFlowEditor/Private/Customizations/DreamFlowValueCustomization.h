#pragma once

#include "DreamFlowVariableTypes.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class IPropertyUtilities;

class FDreamFlowValueCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
    TSharedPtr<IPropertyHandle> GetActiveValueHandle() const;
    EDreamFlowValueType GetCurrentValueType() const;

private:
    TSharedPtr<IPropertyHandle> TypeHandle;
    TSharedPtr<IPropertyHandle> BoolHandle;
    TSharedPtr<IPropertyHandle> IntHandle;
    TSharedPtr<IPropertyHandle> FloatHandle;
    TSharedPtr<IPropertyHandle> NameHandle;
    TSharedPtr<IPropertyHandle> StringHandle;
    TSharedPtr<IPropertyHandle> TextHandle;
    TSharedPtr<IPropertyHandle> GameplayTagHandle;
    TSharedPtr<IPropertyHandle> ObjectHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
};
