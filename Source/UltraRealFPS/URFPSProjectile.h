#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "URFPSProjectile.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;


UCLASS()
class ULTRAREALFPS_API AURFPSProjectile : public AActor
{
    GENERATED_BODY()

public:
    AURFPSProjectile();

    virtual void Tick(float DeltaSeconds) override;

    void Initialize(const FVector& InVelocity, float InDamage, AController* InInstigatorController,
        AActor* InDamageCauser, bool bInTracerRound = false);

private:
    UPROPERTY(VisibleAnywhere) USceneComponent* Root;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* TracerMesh;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* TracerLight;

    bool TryHandleSurfaceInteraction(const FHitResult& Hit, EPhysicalSurface SurfaceType, const FVector& ImpactDirection);
    void SpawnImpact(const FHitResult& Hit, EPhysicalSurface SurfaceType, bool bCharacterImpact, bool bRicochet);

    FVector Velocity = FVector::ZeroVector;
    float InitialSpeed = 0.f;
    float Damage = 35.f;
    float GravityZ = -980.f;
    float DragPerSecond = 0.16f;
    float MaxSubstep = 0.004f;
    float TraveledDistance = 0.f;
    bool bTracerRound = false;
    bool bSuppressionApplied = false;
    int32 RemainingPenetrations = 2;
    int32 RemainingRicochets = 1;

    UPROPERTY() AController* InstigatorController = nullptr;
    UPROPERTY() AActor* DamageCauserActor = nullptr;
};
