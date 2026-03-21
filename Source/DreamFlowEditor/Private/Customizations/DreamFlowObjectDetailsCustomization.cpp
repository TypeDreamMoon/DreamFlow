#include "Customizations/DreamFlowObjectDetailsCustomization.h"

#include "DreamFlowAsset.h"
#include "DreamFlowNode.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FDreamFlowAssetDetailsCustomization::MakeInstance()
{
    return MakeShared<FDreamFlowAssetDetailsCustomization>();
}

void FDreamFlowAssetDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    if (TSharedPtr<IPropertyHandle> NodesHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowAsset, Nodes)))
    {
        NodesHandle->MarkHiddenByCustomization();
    }
}

TSharedRef<IDetailCustomization> FDreamFlowNodeDetailsCustomization::MakeInstance()
{
    return MakeShared<FDreamFlowNodeDetailsCustomization>();
}

void FDreamFlowNodeDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    if (TSharedPtr<IPropertyHandle> ChildrenHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowNode, Children)))
    {
        ChildrenHandle->MarkHiddenByCustomization();
    }

    if (TSharedPtr<IPropertyHandle> OutputLinksHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowNode, OutputLinks)))
    {
        OutputLinksHandle->MarkHiddenByCustomization();
    }

    if (TSharedPtr<IPropertyHandle> PreviewItemsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowNode, PreviewItems)))
    {
        PreviewItemsHandle->MarkHiddenByCustomization();
    }

    if (TSharedPtr<IPropertyHandle> NodeGuidHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDreamFlowNode, NodeGuid)))
    {
        NodeGuidHandle->MarkHiddenByCustomization();

        IDetailCategoryBuilder& FlowCategory = DetailBuilder.EditCategory(TEXT("Flow"));
        FlowCategory.AddProperty(NodeGuidHandle, EPropertyLocation::Advanced);
    }

    CustomizeFlowVariablePickerProperties(DetailBuilder);
}

void FDreamFlowNodeDetailsCustomization::CustomizeFlowVariablePickerProperties(IDetailLayoutBuilder& DetailBuilder) const
{
    TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
    DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

    UDreamFlowAsset* FlowAsset = ResolveOwningFlowAsset(CustomizedObjects);
    const TSharedPtr<IPropertyUtilities> PropertyUtilities = DetailBuilder.GetPropertyUtilities();

    TSet<FName> CustomizedPropertyNames;
    for (const TWeakObjectPtr<UObject>& WeakObject : CustomizedObjects)
    {
        const UDreamFlowNode* FlowNode = Cast<UDreamFlowNode>(WeakObject.Get());
        if (FlowNode == nullptr)
        {
            continue;
        }

        for (TFieldIterator<FProperty> It(FlowNode->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
        {
            const FProperty* Property = *It;
            if (Property == nullptr
                || !Property->HasMetaData(TEXT("DreamFlowVariablePicker"))
                || !Property->IsA<FNameProperty>())
            {
                continue;
            }

            if (CustomizedPropertyNames.Contains(Property->GetFName()))
            {
                continue;
            }

            CustomizedPropertyNames.Add(Property->GetFName());

            TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(Property->GetFName(), FlowNode->GetClass());
            if (!PropertyHandle.IsValid() || !PropertyHandle->IsValidHandle())
            {
                continue;
            }

            if (PropertyUtilities.IsValid())
            {
                TWeakPtr<IPropertyUtilities> WeakPropertyUtilities = PropertyUtilities;
                PropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([WeakPropertyUtilities]()
                {
                    if (const TSharedPtr<IPropertyUtilities> PinnedPropertyUtilities = WeakPropertyUtilities.Pin())
                    {
                        PinnedPropertyUtilities->ForceRefresh();
                    }
                }));
            }

            const FString CategoryName = Property->GetMetaData(TEXT("Category"));
            IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(
                CategoryName.IsEmpty() ? TEXT("Flow") : *CategoryName);

            IDetailPropertyRow& PropertyRow = CategoryBuilder.AddProperty(PropertyHandle);
            PropertyRow.CustomWidget()
                .NameContent()
                [
                    PropertyHandle->CreatePropertyNameWidget()
                ]
                .ValueContent()
                .MinDesiredWidth(320.0f)
                [
                    BuildFlowVariablePickerWidget(PropertyHandle, FlowAsset)
                ];
        }
    }
}

UDreamFlowAsset* FDreamFlowNodeDetailsCustomization::ResolveOwningFlowAsset(const TArray<TWeakObjectPtr<UObject>>& CustomizedObjects) const
{
    for (const TWeakObjectPtr<UObject>& WeakObject : CustomizedObjects)
    {
        for (UObject* Current = WeakObject.Get(); Current != nullptr; Current = Current->GetOuter())
        {
            if (UDreamFlowAsset* FlowAsset = Cast<UDreamFlowAsset>(Current))
            {
                return FlowAsset;
            }
        }
    }

    return nullptr;
}

TSharedRef<SWidget> FDreamFlowNodeDetailsCustomization::BuildFlowVariablePickerWidget(const TSharedPtr<IPropertyHandle>& PropertyHandle, const UDreamFlowAsset* FlowAsset) const
{
    return SNew(SComboButton)
        .OnGetMenuContent_Lambda([this, PropertyHandle, FlowAsset]()
        {
            return BuildFlowVariablePickerMenu(PropertyHandle, FlowAsset);
        })
        .ButtonContent()
        [
            SNew(STextBlock)
            .Text(this, &FDreamFlowNodeDetailsCustomization::GetFlowVariablePickerLabel, PropertyHandle, FlowAsset)
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ];
}

TSharedRef<SWidget> FDreamFlowNodeDetailsCustomization::BuildFlowVariablePickerMenu(const TSharedPtr<IPropertyHandle>& PropertyHandle, const UDreamFlowAsset* FlowAsset) const
{
    FMenuBuilder MenuBuilder(true, nullptr);
    MenuBuilder.BeginSection(NAME_None, FText::FromString(TEXT("Flow Variables")));

    if (FlowAsset != nullptr && FlowAsset->Variables.Num() > 0)
    {
        for (const FDreamFlowVariableDefinition& VariableDefinition : FlowAsset->Variables)
        {
            if (VariableDefinition.Name.IsNone())
            {
                continue;
            }

            const FText ToolTipText = VariableDefinition.Description.IsEmpty()
                ? VariableDefinition.DefaultValue.Type == EDreamFlowValueType::Object
                    ? FText::FromString(TEXT("Object"))
                    : FText::FromString(VariableDefinition.DefaultValue.DescribeType())
                : FText::Format(
                    FText::FromString(TEXT("{0}\nType: {1}")),
                    VariableDefinition.Description,
                    FText::FromString(VariableDefinition.DefaultValue.DescribeType()));

            MenuBuilder.AddMenuEntry(
                FText::FromString(FString::Printf(TEXT("%s (%s)"), *VariableDefinition.Name.ToString(), *VariableDefinition.DefaultValue.DescribeType())),
                ToolTipText,
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([PropertyHandle, VariableName = VariableDefinition.Name]()
                {
                    if (PropertyHandle.IsValid())
                    {
                        PropertyHandle->SetValue(VariableName);
                    }
                })));
        }

        MenuBuilder.AddMenuSeparator();
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("Clear Selection")),
            FText::FromString(TEXT("Remove the current target variable binding.")),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([PropertyHandle]()
            {
                if (PropertyHandle.IsValid())
                {
                    PropertyHandle->SetValue(NAME_None);
                }
            })));
    }
    else
    {
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("No Variables Available")),
            FText::FromString(TEXT("Create flow variables first, then pick one here.")),
            FSlateIcon(),
            FUIAction());
    }

    MenuBuilder.EndSection();
    return MenuBuilder.MakeWidget();
}

FText FDreamFlowNodeDetailsCustomization::GetFlowVariablePickerLabel(TSharedPtr<IPropertyHandle> PropertyHandle, const UDreamFlowAsset* FlowAsset) const
{
    FName VariableName = NAME_None;
    if (PropertyHandle.IsValid())
    {
        PropertyHandle->GetValue(VariableName);
    }

    if (VariableName.IsNone())
    {
        return FText::FromString(TEXT("Select Variable"));
    }

    if (FlowAsset != nullptr)
    {
        if (const FDreamFlowVariableDefinition* VariableDefinition = FlowAsset->FindVariableDefinition(VariableName))
        {
            return FText::Format(
                FText::FromString(TEXT("{0} ({1})")),
                FText::FromName(VariableName),
                FText::FromString(VariableDefinition->DefaultValue.DescribeType()));
        }
    }

    return FText::FromName(VariableName);
}
