#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "URFPSImpactEffect.generated.h"

class UStaticMesh;
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
    void InitializeImpact(const FVector& SurfaceNormal, bool bCharacterImpact, EPhysicalSurface SurfaceType, bool bRicochet, bool bPlayAudio = true);

protected:
    virtual void BeginPlay() override;

private:
    void ApplyVisuals();
    void SpawnSurfaceDebris();

    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* MarkMesh;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* FlashLight;
    UPROPERTY() UMaterialInstanceDynamic* ImpactMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* DebrisMaterial = nullptr;
    UPROPERTY() UStaticMesh* DebrisMesh = nullptr;
    UPROPERTY(Transient) TArray<UStaticMeshComponent*> DebrisComponents;

    TArray<FVector> DebrisVelocities;
    TArray<FVector> DebrisInitialScales;
    TArray<float> DebrisLifeRemaining;
    TArray<float> DebrisLifeInitial;
    TArray<float> DebrisGravityScales;

    FVector CachedSurfaceNormal = FVector::UpVector;
    bool bSoftImpact = false;
    bool bWasRicochet = false;
    bool bDebrisSpawned = false;
    EPhysicalSurface ImpactSurface = static_cast<EPhysicalSurface>(0);
};
