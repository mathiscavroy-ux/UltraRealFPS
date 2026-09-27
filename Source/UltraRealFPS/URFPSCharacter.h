#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "URFPSCharacter.generated.h"

class UCameraComponent;
class USceneComponent;
class UStaticMeshComponent;
class USpotLightComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UCLASS()
class ULTRAREALFPS_API AURFPSCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AURFPSCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintPure, Category="FPS") float GetHealth() const { return Health; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetMaxHealth() const { return MaxHealth; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetArmor() const { return Armor; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetMaxArmor() const { return MaxArmor; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetStamina() const { return Stamina; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetMaxStamina() const { return MaxStamina; }
    UFUNCTION(BlueprintPure, Category="FPS") int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
    UFUNCTION(BlueprintPure, Category="FPS") int32 GetReserveAmmo() const { return ReserveAmmo; }
    UFUNCTION(BlueprintPure, Category="FPS") int32 GetGrenadeCount() const { return GrenadeCount; }
    UFUNCTION(BlueprintPure, Category="FPS") int32 GetBandageCount() const { return BandageCount; }
    UFUNCTION(BlueprintPure, Category="FPS") int32 GetShotsFired() const { return ShotsFired; }
    UFUNCTION(BlueprintPure, Category="FPS") int32 GetConfirmedHits() const { return ConfirmedHits; }
    UFUNCTION(BlueprintPure, Category="FPS") int32 GetHeadshotCount() const { return HeadshotCount; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetAccuracyPercent() const;
    UFUNCTION(BlueprintPure, Category="FPS") float GetBleedSeverity() const { return BleedSeverity; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetTreatmentProgress() const;
    UFUNCTION(BlueprintPure, Category="FPS") bool IsTreating() const { return bTreating; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsLowReady() const { return bLowReady; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsAiming() const { return bAiming; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsReloading() const { return bReloading; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsSprinting() const { return bSprinting; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsWalkingSlow() const { return bWalkingSlow; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsAutomaticFire() const { return FireModeIndex == 2; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsFlashlightOn() const { return bFlashlightOn; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsHoldingBreath() const { return bHoldingBreath; }
    UFUNCTION(BlueprintPure, Category="FPS") bool IsDead() const { return Health <= 0.f; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetCurrentSpreadDegrees() const;
    UFUNCTION(BlueprintPure, Category="FPS") float GetHitMarkerAlpha() const { return HitMarkerAlpha; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetHeadshotMarkerAlpha() const { return HeadshotMarkerAlpha; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetDamageFeedback() const { return DamageFeedback; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetDamageDirectionAlpha() const { return DamageDirectionAlpha; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetDamageDirectionDegrees() const { return DamageDirectionDegrees; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetLeanAmount() const { return LeanCurrent; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetSuppression() const { return SuppressionFeedback; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetWeaponObstructionAlpha() const { return WeaponObstructionAlpha; }
    UFUNCTION(BlueprintPure, Category="FPS") float GetReloadProgress() const;
    UFUNCTION(BlueprintPure, Category="FPS") FString GetFireModeName() const;
    UFUNCTION(BlueprintPure, Category="FPS") int32 GetZeroDistanceMeters() const;
    UFUNCTION(BlueprintPure, Category="FPS") FString GetInteractionPrompt() const;

    void RegisterConfirmedHit(bool bHeadshot);
    void ApplySuppression(float Intensity, const FVector& SourceLocation);

protected:
    virtual void BeginPlay() override;
    virtual void Landed(const FHitResult& Hit) override;

private:
    UPROPERTY(VisibleAnywhere) UCameraComponent* FirstPersonCamera;
    UPROPERTY(VisibleAnywhere) USceneComponent* WeaponRoot;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponReceiver;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponHandguard;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponBarrel;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponStock;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponMagazine;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponSight;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponOpticLens;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponRail;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponGrip;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponMuzzle;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponUpperReceiver;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponFrontSight;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponTriggerGuard;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponForegrip;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* LeftArm;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* RightArm;
    UPROPERTY(VisibleAnywhere) USpotLightComponent* Flashlight;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* MuzzleFlashLight;
    UPROPERTY(VisibleAnywhere) USpotLightComponent* MuzzleFlashCone;
    UPROPERTY() UMaterialInstanceDynamic* WeaponMaterial;
    UPROPERTY() UMaterialInstanceDynamic* WeaponAccentMaterial;
    UPROPERTY() UMaterialInstanceDynamic* OpticLensMaterial;
    UPROPERTY() UMaterialInstanceDynamic* ArmMaterial;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void BeginSprint();
    void EndSprint();
    void BeginSlowWalk();
    void EndSlowWalk();
    void ToggleCrouch();
    void BeginAim();
    void EndAim();
    void BeginFire();
    void EndFire();
    void FireShot();
    void FireBurstShot();
    void Reload();
    void FinishReload();
    void ToggleFlashlight();
    void ToggleFireMode();
    void ToggleLowReady();
    void CycleZeroDistance();
    void ThrowGrenade();
    void UseBandage();
    void FinishBandage();
    void CancelTreatment();
    void BeginLeanLeft();
    void EndLeanLeft();
    void BeginLeanRight();
    void EndLeanRight();
    void Interact();
    void ExitCursor();
    void HideMuzzleFlash();
    void RespawnSelf();

    FVector GetShotDirection() const;
    void UpdateMovement(float DeltaSeconds);
    void UpdateCamera(float DeltaSeconds);
    void UpdateWeaponPresentation(float DeltaSeconds);
    void UpdateFeedback(float DeltaSeconds);
    void UpdateMedical(float DeltaSeconds);
    void UpdateFootsteps(float DeltaSeconds);
    void PlayFootstep(float VolumeMultiplier = 0.62f);
    void UpdateInteractionTarget();
    void UpdateWeaponObstruction(float DeltaSeconds);
    void NotifyEnemiesOfNoise(const FVector& SourceLocation, float AudibleRange, float AlertStrength);
    void NotifyEnemiesOfGunshot(const FVector& ShotDirection);
    void ApplyDamageDirection(AActor* DamageCauser);
    void SpawnShellCasing();
    void HandleDeath();

    float WalkSpeed = 325.f;
    float SlowWalkSpeed = 185.f;
    float SprintSpeed = 565.f;
    float CrouchSpeed = 185.f;
    float InjuredWalkSpeed = 245.f;

    float MaxStamina = 100.f;
    float Stamina = 100.f;
    float StaminaDrainPerSecond = 25.f;
    float HoldBreathDrainPerSecond = 17.f;
    float StaminaRecoveryPerSecond = 20.f;
    float StaminaRecoveryDelay = 0.7f;
    float StaminaRecoveryCooldown = 0.f;
    bool bSprinting = false;
    bool bWalkingSlow = false;
    bool bHoldingBreath = false;

    float MaxHealth = 100.f;
    float Health = 100.f;
    float MaxArmor = 50.f;
    float Armor = 50.f;
    float SpawnProtectionRemaining = 1.35f;
    float BleedSeverity = 0.f;
    float BleedDamageAccumulator = 0.f;
    int32 BandageCount = 2;
    int32 MaxBandages = 3;
    bool bTreating = false;
    float TreatmentDuration = 3.1f;
    float TreatmentElapsed = 0.f;

    bool bAiming = false;
    bool bReloading = false;
    bool bTriggerHeld = false;
    bool bFlashlightOn = false;
    bool bReloadStartedWithChamberedRound = false;
    bool bLowReady = false;

    float HipFOV = 90.f;
    float SprintFOV = 96.f;
    float AimFOV = 66.f;
    float FOVInterpSpeed = 11.f;
    float MouseSensitivity = 0.78f;

    int32 MagazineCapacity = 30;
    int32 AmmoInMagazine = 30;
    int32 ReserveAmmo = 150;
    int32 MaxReserveAmmo = 210;
    int32 GrenadeCount = 2;
    int32 MaxGrenades = 3;
    int32 FireModeIndex = 2;
    int32 BurstShotsRemaining = 0;
    int32 ShotCounter = 0;
    int32 ShotsFired = 0;
    int32 ConfirmedHits = 0;
    int32 HeadshotCount = 0;
    int32 ZeroDistanceIndex = 1;
    float RoundsPerMinute = 720.f;
    float BaseDamage = 36.f;
    float FireRange = 80000.f;
    float BulletMuzzleVelocity = 82000.f;
    float HipSpreadDegrees = 0.72f;
    float AimSpreadDegrees = 0.10f;
    float CurrentBloom = 0.f;
    float BloomPerShot = 0.08f;
    float MaxBloom = 0.82f;
    float BloomRecoveryPerSecond = 1.45f;
    float VerticalRecoil = 0.53f;
    float HorizontalRecoil = 0.16f;
    float TacticalReloadDuration = 2.12f;
    float EmergencyReloadDuration = 1.72f;
    float ActiveReloadDuration = 2.12f;
    float ReloadElapsed = 0.f;

    float MoveForwardInput = 0.f;
    float MoveRightInput = 0.f;
    float MouseInputX = 0.f;
    float MouseInputY = 0.f;

    FVector BaseCameraLocation = FVector(0.f, 0.f, 64.f);
    FVector HipWeaponLocation = FVector(34.f, 13.f, -16.f);
    FVector AimWeaponLocation = FVector(28.f, 0.4f, -8.7f);
    FVector SprintWeaponLocation = FVector(26.f, 18.f, -24.f);
    FVector LowReadyWeaponLocation = FVector(25.f, 16.f, -27.f);
    FVector MagazineBaseLocation = FVector(-1.f, 0.f, -12.f);
    FRotator HipWeaponRotation = FRotator::ZeroRotator;
    FRotator SprintWeaponRotation = FRotator(-16.f, 10.f, 12.f);
    FRotator LowReadyWeaponRotation = FRotator(-28.f, 8.f, 10.f);

    float HeadBobTime = 0.f;
    float BreathTime = 0.f;
    float LandingKick = 0.f;
    float PreviousVerticalVelocity = 0.f;
    float FootstepDistanceAccumulator = 0.f;
    float DamageFeedback = 0.f;
    float DamageDirectionAlpha = 0.f;
    float DamageDirectionDegrees = 0.f;
    float HitMarkerAlpha = 0.f;
    float HeadshotMarkerAlpha = 0.f;
    float SuppressionFeedback = 0.f;
    float WeaponObstructionAlpha = 0.f;
    float LeanTarget = 0.f;
    float LeanCurrent = 0.f;
    FVector WeaponKickLocation = FVector::ZeroVector;
    FRotator WeaponKickRotation = FRotator::ZeroRotator;
    FVector WeaponInertiaLocation = FVector::ZeroVector;
    FRotator WeaponInertiaRotation = FRotator::ZeroRotator;
    FVector PreviousPlanarVelocity = FVector::ZeroVector;

    TWeakObjectPtr<AActor> InteractionTarget;
    FVector InitialSpawnLocation = FVector::ZeroVector;
    FRotator InitialSpawnRotation = FRotator::ZeroRotator;

    FTimerHandle FireTimerHandle;
    FTimerHandle ReloadTimerHandle;
    FTimerHandle MuzzleFlashTimerHandle;
    FTimerHandle TreatmentTimerHandle;
};
