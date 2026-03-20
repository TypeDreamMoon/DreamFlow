#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNodeDisplayTypes.h"
#include "UObject/Object.h"
#include "DreamFlowNode.generated.h"

class UDreamFlowAsset;
class UDreamFlowNode;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class DREAMFLOW_API UDreamFlowNode : public UObject
{
    GENERATED_BODY()

public:
    UDreamFlowNode();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
    FGuid NodeGuid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
    TArray<TObjectPtr<UDreamFlowNode>> Children;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    TArray<FDreamFlowNodeDisplayItem> PreviewItems;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow|Compatibility")
    TSubclassOf<UDreamFlowAsset> SupportedFlowAssetType;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    FLinearColor NodeTint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Editor")
    FVector2D EditorPosition;
#endif

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    FText GetNodeDisplayName() const;
    virtual FText GetNodeDisplayName_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    FLinearColor GetNodeTint() const;
    virtual FLinearColor GetNodeTint_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    FText GetNodeCategory() const;
    virtual FText GetNodeCategory_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    FText GetNodeAccentLabel() const;
    virtual FText GetNodeAccentLabel_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Preview")
    TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems() const;
    virtual TArray<FDreamFlowNodeDisplayItem> GetNodeDisplayItems_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    bool SupportsMultipleChildren() const;
    virtual bool SupportsMultipleChildren_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    bool SupportsMultipleParents() const;
    virtual bool SupportsMultipleParents_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    bool IsEntryNode() const;
    virtual bool IsEntryNode_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    bool IsUserCreatable() const;
    virtual bool IsUserCreatable_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    bool IsTerminalNode() const;
    virtual bool IsTerminalNode_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure = false, Category = "Flow")
    bool CanEnterNode(UObject* Context) const;
    virtual bool CanEnterNode_Implementation(UObject* Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Flow")
    bool CanConnectTo(const UDreamFlowNode* OtherNode) const;
    virtual bool CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Flow")
    void ExecuteNode(UObject* Context);
    virtual void ExecuteNode_Implementation(UObject* Context);

    UFUNCTION(BlueprintCallable, Category = "Flow")
    void SetChildren(const TArray<UDreamFlowNode*>& InChildren);

    UFUNCTION(BlueprintPure, Category = "Flow")
    TArray<UDreamFlowNode*> GetChildrenCopy() const;

    TSubclassOf<UDreamFlowAsset> GetSupportedFlowAssetType() const;
    bool SupportsFlowAsset(const UDreamFlowAsset* FlowAsset) const;
    bool SupportsFlowAssetClass(const UClass* FlowAssetClass) const;
    bool CanAcceptChild(const UDreamFlowNode* OtherNode) const;

#if WITH_EDITOR
    void SetEditorPosition(const FVector2D& InPosition);
#endif
};
