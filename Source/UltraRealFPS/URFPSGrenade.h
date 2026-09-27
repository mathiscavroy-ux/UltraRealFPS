#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "URFPSGrenade.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UProjectileMovementComponent;
class AController;

UCLASS()
class ULTRAREALFPS_API AURFPSGrenade : public AActor
{
    GENERATED_BODY()

public:
    AURFPSGrenade();

    virtual void Tick(float DeltaSeconds) override;
    void InitializeGrenade(const FVector& InitialVelocity, AController* InInstigatorController, AActor* InDamageCauser);

protected:
    virtual void BeginPlay() override;

private:
    void Explode();
    void PulseThreat();
    float ComputeOccludedDamage(AActor* Target, float Distance) const;

    UPROPERTY(VisibleAnywhere) USphereComponent* CollisionSphere;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* GrenadeMesh;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* ExplosionLight;
    UPROPERTY(VisibleAnywhere) UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY() AController* InstigatorController = nullptr;
    UPROPERTY() AActor* DamageCauserActor = nullptr;

    float FuseSeconds = 3.15f;
    float ExplosionRadius = 720.f;
    float MaxEnemyDamage = 155.f;
    float MaxPlayerDamage = 62.f;
    float ThreatPulseCooldown = 0.f;
    bool bExploded = false;

    FTimerHandle FuseTimerHandle;
};
