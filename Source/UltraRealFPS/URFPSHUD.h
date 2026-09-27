#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "URFPSHUD.generated.h"

UCLASS()
class ULTRAREALFPS_API AURFPSHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
