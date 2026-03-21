#pragma once

#include "DreamFlowVariableTypes.h"
#include "IDetailCustomization.h"

class IPropertyHandle;
class IPropertyHandleArray;
class IPropertyUtilities;
class IDetailsView;
class SBox;
class STextBlock;
class SWidget;
class UDreamFlowVariablesEditorData;

class FDreamFlowVariablesEditorDataDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    TSharedRef<SWidget> BuildVariablesPanel();
    TSharedRef<SWidget> BuildVariablesSidebar() const;
    TSharedRef<SWidget> BuildAddVariableMenu() const;
    TSharedRef<SWidget> BuildVariablesList() const;
    TSharedRef<SWidget> BuildVariableCard(int32 VariableIndex) const;
    TSharedRef<SWidget> BuildSelectedVariableEditor() const;
    TSharedRef<SWidget> BuildInspectorSummary(int32 VariableIndex) const;
    TSharedRef<SWidget> BuildPropertyEditorRow(const TSharedPtr<IPropertyHandle>& PropertyHandle, bool bUseCustomValueWidget = false) const;
    TSharedRef<SWidget> BuildEmptyState(const FText& Title, const FText& Description) const;
    void AddVariable(EDreamFlowValueType ValueType) const;
    void SelectVariable(int32 VariableIndex) const;
    void DuplicateVariable(int32 VariableIndex) const;
    void RemoveVariable(int32 VariableIndex) const;
    void HandleSearchTextChanged(const FText& InSearchText);
    FText GetSearchText() const;
    FText GetVariablesListCountText() const;
    void RefreshFilteredVariableWidgets() const;
    FString GetStoredSearchText() const;
    int32 GetSelectedVariableIndex() const;
    TArray<int32> GetFilteredVariableIndices() const;
    TSharedPtr<IPropertyHandleArray> GetVariablesArrayHandle() const;
    TSharedPtr<IPropertyHandle> GetVariableHandle(int32 VariableIndex) const;
    UDreamFlowVariablesEditorData* GetVariablesEditorData() const;
    FText GetVariableDisplayName(int32 VariableIndex) const;
    FText GetVariableSummary(int32 VariableIndex) const;

private:
    TSharedPtr<IPropertyHandle> VariablesHandle;
    TSharedPtr<IPropertyUtilities> PropertyUtilities;
    TWeakPtr<const IDetailsView> DetailsView;
    TWeakObjectPtr<UDreamFlowVariablesEditorData> VariablesEditorData;
    mutable TSharedPtr<SBox> VariablesListContainer;
    mutable TSharedPtr<STextBlock> VariablesListCountTextBlock;
};
