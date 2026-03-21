#pragma once

#include "DreamFlowVariableTypes.h"
#include "IPropertyTypeCustomization.h"

struct FDreamFlowVariableDefinition;
class IPropertyHandle;
class IPropertyUtilities;
class UDreamFlowAsset;
class SWidget;

class FDreamFlowValueBindingCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
    EDreamFlowValueSourceType GetCurrentSourceType() const;
    UDreamFlowAsset* ResolveOwningFlowAsset() const;
    TOptional<EDreamFlowValueType> ResolveExpectedValueType() const;
    bool IsVariableDefinitionCompatible(const FDreamFlowVariableDefinition& VariableDefinition) const;
    bool TryGetBindingValue(FDreamFlowValueBinding& OutBinding) const;
    FText GetExpectedTypeLabel() const;
    FText GetBindingSummary() const;
    FText GetSourceTypeLabel() const;
    FText GetVariablePickerLabel() const;
    void EnsureLiteralValueMatchesExpectedType() const;
    void SetSourceType(EDreamFlowValueSourceType NewSourceType) const;
    void SetVariableName(FName NewVariableName) const;
    TSharedRef<SWidget> BuildSourceTypeMenu() const;
    TSharedRef<SWidget> BuildVariableMenu() const;

private:
    TSharedPtr<IPropertyHandle> StructHandle;
    TSharedPtr<IPropertyHandle> SourceTypeHandle;
    TSharedPtr<IPropertyHandle> VariableNameHandle;
    TSharedPtr<IPropertyHandle> LiteralValueHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
};
