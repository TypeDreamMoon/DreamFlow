#pragma once

#include "CoreMinimal.h"
#include "DreamFlowNodeDisplayTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EDreamFlowNodeDisplayItemType : uint8
{
    Text,
    Color,
    Image,
};

USTRUCT(BlueprintType)
struct DREAMFLOW_API FDreamFlowNodeDisplayItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    EDreamFlowNodeDisplayItemType Type = EDreamFlowNodeDisplayItemType::Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (MultiLine = true))
    FText TextValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FLinearColor ColorValue = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    TSoftObjectPtr<UTexture2D> Image;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FVector2D ImageSize = FVector2D(88.0f, 56.0f);
};
