#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNode.h"
#include "DreamFlowDialogueNodes.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOWDIALOGUE_API UDreamFlowDialogueNode : public UDreamFlowNode
{
    GENERATED_BODY()

public:
    UDreamFlowDialogueNode();

    virtual FText GetNodeCategory_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOWDIALOGUE_API UDreamFlowDialogueLineNode : public UDreamFlowDialogueNode
{
    GENERATED_BODY()

public:
    UDreamFlowDialogueLineNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "20"))
    FText SpeakerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true, DreamFlowInlineEditable, DreamFlowInlinePriority = "30"))
    FText LineText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "40"))
    FName VoiceTag;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual bool SupportsMultipleChildren_Implementation() const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOWDIALOGUE_API UDreamFlowDialogueChoiceNode : public UDreamFlowDialogueNode
{
    GENERATED_BODY()

public:
    UDreamFlowDialogueChoiceNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true, DreamFlowInlineEditable, DreamFlowInlinePriority = "20"))
    FText Prompt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "30"))
    TArray<FText> ChoiceLabels;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class DREAMFLOWDIALOGUE_API UDreamFlowDialogueEndNode : public UDreamFlowDialogueNode
{
    GENERATED_BODY()

public:
    UDreamFlowDialogueEndNode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (DreamFlowInlineEditable, DreamFlowInlinePriority = "20"))
    FText EndLabel;

    virtual FText GetNodeDisplayName_Implementation() const override;
    virtual FLinearColor GetNodeTint_Implementation() const override;
    virtual FText GetNodeAccentLabel_Implementation() const override;
    virtual bool SupportsMultipleChildren_Implementation() const override;
    virtual bool IsTerminalNode_Implementation() const override;
    virtual bool CanConnectTo_Implementation(const UDreamFlowNode* OtherNode) const override;
};
