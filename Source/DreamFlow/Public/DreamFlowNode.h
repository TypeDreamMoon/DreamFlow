#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNodeDisplayTypes.h"
#include "DreamFlowTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/Object.h"
#include "DreamFlowNode.generated.h"

class UDreamFlowAsset;
class UDreamFlowExecutor;
class UDreamFlowNode;
class UTexture2D;

UENUM(BlueprintType)
enum class EDreamFlowNodeTransitionMode : uint8
{
    Manual UMETA(DisplayName = "Manual"),
    Automatic UMETA(DisplayName = "Automatic")
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowNodeOutputPin
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FName PinName = TEXT("Out");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    bool bAllowMultipleConnections = false;
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowNodeOutputLink
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
    FName PinName = TEXT("Out");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
    TObjectPtr<UDreamFlowNode> Child = nullptr;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class DREAMFLOW_API UDreamFlowNode : public UObject
{
    GENERATED_BODY()

public:
    UDreamFlowNode();
    virtual void PostLoad() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
    FGuid NodeGuid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
    TArray<TObjectPtr<UDreamFlowNode>> Children;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
    TArray<FDreamFlowNodeOutputLink> OutputLinks;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    TArray<FDreamFlowNodeDisplayItem> PreviewItems;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow|Compatibility")
    TSubclassOf<UDreamFlowAsset> SupportedFlowAssetType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Execution")
    EDreamFlowNodeTransitionMode TransitionMode = EDreamFlowNodeTransitionMode::Manual;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    FLinearColor NodeTint;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Editor")
    FName NodeIconStyleName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Editor")
    TSoftObjectPtr<UTexture2D> NodeIconTexture;

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

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Editor")
    FName GetNodeIconStyleName() const;
    virtual FName GetNodeIconStyleName_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Editor")
    UTexture2D* GetNodeIconTexture() const;
    virtual UTexture2D* GetNodeIconTexture_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Editor")
    TArray<FName> GetInlineEditablePropertyNames() const;
    virtual TArray<FName> GetInlineEditablePropertyNames_Implementation() const;

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
    TArray<FDreamFlowNodeOutputPin> GetOutputPins() const;
    virtual TArray<FDreamFlowNodeOutputPin> GetOutputPins_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    bool IsEntryNode() const;
    virtual bool IsEntryNode_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    bool IsUserCreatable() const;
    virtual bool IsUserCreatable_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow")
    bool IsTerminalNode() const;
    virtual bool IsTerminalNode_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Execution")
    EDreamFlowNodeTransitionMode GetTransitionMode() const;
    virtual EDreamFlowNodeTransitionMode GetTransitionMode_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Execution")
    FText GetTransitionModeLabel() const;
    virtual FText GetTransitionModeLabel_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure = false, Category = "Flow")
    bool CanEnterNode(UObject* Context) const;
    virtual bool CanEnterNode_Implementation(UObject* Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Flow")
    bool CanConnectTo(const UDreamFlowNode* OtherNode) const;
    virtual bool CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Flow")
    void ExecuteNode(UObject* Context);
    virtual void ExecuteNode_Implementation(UObject* Context);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Flow|Execution")
    void ExecuteNodeWithExecutor(UObject* Context, UDreamFlowExecutor* Executor);
    virtual void ExecuteNodeWithExecutor_Implementation(UObject* Context, UDreamFlowExecutor* Executor);

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Execution")
    bool SupportsAutomaticTransition(UObject* Context, UDreamFlowExecutor* Executor) const;
    virtual bool SupportsAutomaticTransition_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Execution")
    int32 ResolveAutomaticTransitionChildIndex(UObject* Context, UDreamFlowExecutor* Executor) const;
    virtual int32 ResolveAutomaticTransitionChildIndex_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Flow|Execution")
    FName ResolveAutomaticTransitionOutputPin(UObject* Context, UDreamFlowExecutor* Executor) const;
    virtual FName ResolveAutomaticTransitionOutputPin_Implementation(UObject* Context, UDreamFlowExecutor* Executor) const;

    UFUNCTION(BlueprintCallable, Category = "Flow|Execution")
    bool ContinueFlow(UDreamFlowExecutor* Executor);

    UFUNCTION(BlueprintCallable, Category = "Flow|Execution")
    bool ContinueFlowFromOutputPin(UDreamFlowExecutor* Executor, FName OutputPinName);

    UFUNCTION(BlueprintCallable, Category = "Flow")
    void SetChildren(const TArray<UDreamFlowNode*>& InChildren);

    UFUNCTION(BlueprintCallable, Category = "Flow")
    void SetOutputLinks(const TArray<FDreamFlowNodeOutputLink>& InOutputLinks);

    UFUNCTION(BlueprintPure, Category = "Flow")
    TArray<UDreamFlowNode*> GetChildrenCopy() const;

    UFUNCTION(BlueprintPure, Category = "Flow")
    TArray<FDreamFlowNodeOutputLink> GetOutputLinksCopy() const;

    UFUNCTION(BlueprintPure, Category = "Flow")
    TArray<UDreamFlowNode*> GetChildrenForOutputPin(FName OutputPinName) const;

    UFUNCTION(BlueprintPure, Category = "Flow")
    UDreamFlowNode* GetFirstChildForOutputPin(FName OutputPinName) const;

    TSubclassOf<UDreamFlowAsset> GetSupportedFlowAssetType() const;
    bool SupportsFlowAsset(const UDreamFlowAsset* FlowAsset) const;
    bool SupportsFlowAssetClass(const UClass* FlowAssetClass) const;
    bool CanAcceptChild(const UDreamFlowNode* OtherNode) const;
    virtual void ValidateNode(const UDreamFlowAsset* OwningAsset, TArray<FDreamFlowValidationMessage>& OutMessages) const;

#if WITH_EDITOR
    void SetEditorPosition(const FVector2D& InPosition);
#endif

protected:
    void FixupLegacyOutputLinksFromChildren();
};
