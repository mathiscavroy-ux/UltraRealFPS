#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "URFPSEnemy.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class USpotLightComponent;
class UMaterialInstanceDynamic;

enum class EURFPSEnemyRole : uint8
{
    Rifleman,
    Suppressor,
    Marksman,
    Breacher
};

// What an enemy does while it has no recent contact with the player.
enum class EURFPSEnemyIntent : uint8
{
    Guard,
    Patrol,
    Search,
    Hunt
};

UCLASS()
class ULTRAREALFPS_API AURFPSEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    AURFPSEnemy();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    void ConfigureForWave(int32 WaveNumber, int32 RoleSeed = 0);
    void AlertFromNoise(const FVector& SourceLocation, float AudibleRange, float AlertStrength = 0.65f);
    void AlertFromGunshot(const FVector& SourceLocation);
    void ApplySuppression(const FVector& SourceLocation);
    void ReactToGrenade(const FVector& GrenadeLocation);
    bool IsEnemyDead() const { return bDead; }
    bool HasFireSlot() const { return bHasFireSlot; }
    EURFPSEnemyRole GetRole() const { return Role; }
    EURFPSEnemyIntent GetIntent() const { return Intent; }

    // Wave pacing set by the GameMode: once the delay has passed the enemy leaves its sector and
    // moves on the player's area. BeginHunt starts it immediately (last survivors of a wave).
    void ScheduleHunt(float DelaySeconds);
    void BeginHunt();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* BodyMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* HeadMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* HelmetMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* VestMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* BackpackMesh;
    UPROPERTY(VisibleAnywhere) USceneComponent* LeftHipPivot;
    UPROPERTY(VisibleAnywhere) USceneComponent* RightHipPivot;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* LeftLegMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* RightLegMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponBarrelMesh;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* MuzzleFlashLight;
    UPROPERTY(VisibleAnywhere) USpotLightComponent* MuzzleFlashCone;
    UPROPERTY() UMaterialInstanceDynamic* UniformMaterial;
    UPROPERTY() UMaterialInstanceDynamic* GearMaterial;
    UPROPERTY() UMaterialInstanceDynamic* WeaponMaterial;
    UPROPERTY() UMaterialInstanceDynamic* FaceMaterial;

    bool HasLineOfSightToPlayer() const;
    bool IsPlayerDown(const APawn* Player) const;
    void DisengageFromDownedPlayer();
    void UpdateLegSwing(float DeltaSeconds);
    void BeginDeathFall();
    void UpdateDeathFall(float DeltaSeconds);
    FVector GetAvoidanceDirection(const FVector& DesiredDirection) const;
    FVector FindCoverBiasedDirection(APawn* Player) const;
    void UpdateCombatMovement(APawn* Player, float DeltaSeconds, bool bHasLOS);
    void TryFire(float DeltaSeconds, APawn* Player, bool bHasLOS);
    void FireOneShot(APawn* Player);
    void UpdateFootsteps(float DeltaSeconds);
    void AlertNearbySquad(const FVector& AlertLocation);
    void ApplyRoleVisuals();
    void SetRoleFromSeed(int32 RoleSeed);
    bool AcquireFireSlot();
    void ReleaseFireSlot();

    // Navigation and non-combat behaviour (patrol, search, hunt) over the GameMode nav grid.
    void UpdateNavigation(APawn* Player, float DeltaSeconds);
    void SetIntent(EURFPSEnemyIntent NewIntent);
    bool MoveTowards(const FVector& Goal, float SpeedScale, float AcceptRadius, float DeltaSeconds);
    void ClearPath();
    void BeginSearch(const FVector& Center, int32 Points);
    void PickHuntTarget(const APawn* Player);
    bool PickPatrolPoint();
    void UpdateLookAround(float DeltaSeconds);
    void TryOpenDoorAhead(const FVector& Direction);
    FVector GetSquadSeparation() const;
    float GetWorldTimeSeconds() const;

    EURFPSEnemyRole Role = EURFPSEnemyRole::Rifleman;

    float MaxHealth = 95.f;
    float Health = 95.f;
    float PreferredDistance = 1250.f;
    float FireCooldown = 0.f;
    float DamagePerShot = 6.4f;
    float BaseAccuracyDegrees = 2.45f;
    float AggroRange = 4100.f;
    float ReactionMin = 0.62f;
    float ReactionMax = 1.10f;
    float BurstPauseMin = 1.05f;
    float BurstPauseMax = 1.70f;
    float BurstIntervalMin = 0.15f;
    float BurstIntervalMax = 0.22f;
    float ProjectileSpeed = 72000.f;
    float RepositionTimer = 0.f;
    float WeaponKick = 0.f;
    float MuzzleFlashTimer = 0.f;
    float CoverSeekTimer = 0.f;
    float GrenadeAvoidTimer = 0.f;
    float BaseConfiguredWalkSpeed = 265.f;
    float HelmetDurability = 28.f;
    float VestDurability = 38.f;
    float LegInjury = 0.f;
    float AimInjury = 0.f;
    float FootstepDistanceAccumulator = 0.f;
    float LegSwingPhase = 0.f;
    float LegSwingAmplitude = 0.f;

    FVector DeathStartLocation = FVector::ZeroVector;
    FVector DeathRestLocation = FVector::ZeroVector;
    FRotator DeathStartRotation = FRotator::ZeroRotator;
    float DeathRestRoll = 0.f;
    float DeathFallElapsed = 0.f;
    float DeathFallDuration = 0.42f;

    int32 MinBurstShots = 2;
    int32 MaxBurstShots = 3;
    int32 StrafeDirection = 1;
    int32 ConfiguredWave = 1;
    int32 EnemyShotCounter = 0;
    float StrafeChangeTimer = 0.f;
    int32 BurstShotsRemaining = 0;
    float ReactionTimer = 0.f;
    float TimeSinceSeen = 999.f;
    float SuppressionTimer = 0.f;
    float SearchLingerTime = 8.f;
    FVector LastSeenLocation = FVector::ZeroVector;
    FVector GrenadeThreatLocation = FVector::ZeroVector;
    bool bAlerted = false;
    bool bDead = false;
    bool bHasFireSlot = false;

    EURFPSEnemyIntent Intent = EURFPSEnemyIntent::Guard;
    TArray<FVector> PathPoints;
    int32 PathIndex = 0;
    FVector PathGoal = FVector::ZeroVector;
    bool bHasPath = false;
    float RepathTimer = 0.f;
    float ProgressTimer = 0.f;
    FVector ProgressAnchor = FVector::ZeroVector;
    int32 StuckCount = 0;
    float SidestepTimer = 0.f;
    FVector SidestepDirection = FVector::ZeroVector;
    FVector MoveTarget = FVector::ZeroVector;
    bool bHasMoveTarget = false;
    FVector SearchCenter = FVector::ZeroVector;
    int32 SearchPointsLeft = 0;
    float LookAroundTimer = 0.f;
    float LookAroundYaw = 0.f;
    float HuntStartTime = -1.f;
    float HuntRefreshTimer = 0.f;
    int32 HuntRefreshCount = 0;
    FVector GuardLocation = FVector::ZeroVector;
    float PatrolWaitTimer = 0.f;
    float DoorCooldown = 0.f;
    float DoorPauseTimer = 0.f;
};
