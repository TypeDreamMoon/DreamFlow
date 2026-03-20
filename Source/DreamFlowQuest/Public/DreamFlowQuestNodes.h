#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNode.h"
#include "DreamFlowQuestNodes.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOWQUEST_API UDreamFlowQuestNode : public UDreamFlowNode
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName QuestId;

    virtual FText GetNodeCategory_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOWQUEST_API UDreamFlowQuestTaskNode : public UDreamFlowQuestNode
{
    GENERATED_BODY()

public:
    UDreamFlowQuestTaskNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FText TaskLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    bool bOptional = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FText> Objectives;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOWQUEST_API UDreamFlowQuestConditionNode : public UDreamFlowQuestNode
{
    GENERATED_BODY()

public:
    UDreamFlowQuestConditionNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FText ConditionLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    bool bExpectedResult = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FText FailureHint;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOWQUEST_API UDreamFlowQuestCompleteNode : public UDreamFlowQuestNode
{
    GENERATED_BODY()

public:
    UDreamFlowQuestCompleteNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FText CompletionLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FText RewardPreview;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual bool SupportsMultipleChildren_Implementation() const override;
    virtual bool IsTerminalNode_Implementation() const override;
    virtual bool CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const override;
};
