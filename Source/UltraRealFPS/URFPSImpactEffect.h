#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "URFPSImpactEffect.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UCLASS()
class ULTRAREALFPS_API AURFPSImpactEffect : public AActor
{
    GENERATED_BODY()

public:
    AURFPSImpactEffect();
    virtual void Tick(float DeltaSeconds) override;
    void InitializeImpact(const FVector& SurfaceNormal, bool bCharacterImpact, EPhysicalSurface SurfaceType, bool bRicochet);

protected:
    virtual void BeginPlay() override;

private:
    void ApplyVisuals();

    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* MarkMesh;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* FlashLight;
    UPROPERTY() UMaterialInstanceDynamic* ImpactMaterial = nullptr;
    bool bSoftImpact = false;
    bool bWasRicochet = false;
    EPhysicalSurface ImpactSurface = static_cast<EPhysicalSurface>(0);
};
