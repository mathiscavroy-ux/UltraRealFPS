#include "URFPSCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "URFPSAudio.h"
#include "URFPSDoor.h"
#include "URFPSEnemy.h"
#include "URFPSGameMode.h"
#include "URFPSGrenade.h"
#include "URFPSProjectile.h"
#include "URFPSShellCasing.h"

AURFPSCharacter::AURFPSCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 92.f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Move = GetCharacterMovement();
    Move->bOrientRotationToMovement = false;
    Move->MaxWalkSpeed = WalkSpeed;
    Move->MaxAcceleration = 1450.f;
    Move->BrakingDecelerationWalking = 1300.f;
    Move->GroundFriction = 7.f;
    Move->AirControl = 0.18f;
    Move->JumpZVelocity = 410.f;
    Move->GetNavAgentPropertiesRef().bCanCrouch = true;
    Move->SetCrouchedHalfHeight(58.f);

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(BaseCameraLocation);
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->SetFieldOfView(HipFOV);

    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    WeaponRoot->SetupAttachment(FirstPersonCamera);
    WeaponRoot->SetRelativeLocation(HipWeaponLocation);

    auto CreateWeaponPart = [this](const TCHAR* Name, const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator)
    {
        UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(FName(Name));
        Part->SetupAttachment(WeaponRoot);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetRelativeLocation(Location);
        Part->SetRelativeRotation(Rotation);
        Part->SetRelativeScale3D(Scale);
        Part->SetCastShadow(false);
        return Part;
    };

    WeaponReceiver = CreateWeaponPart(TEXT("WeaponReceiver"), FVector::ZeroVector, FVector(0.34f, 0.075f, 0.085f));
    WeaponHandguard = CreateWeaponPart(TEXT("WeaponHandguard"), FVector(30.f, 0.f, -0.5f), FVector(0.25f, 0.06f, 0.065f));
    WeaponBarrel = CreateWeaponPart(TEXT("WeaponBarrel"), FVector(52.f, 0.f, -0.1f), FVector(0.015f, 0.015f, 0.33f), FRotator(0.f, 90.f, 0.f));
    WeaponStock = CreateWeaponPart(TEXT("WeaponStock"), FVector(-29.f, 0.f, 0.5f), FVector(0.27f, 0.065f, 0.075f), FRotator(0.f, 0.f, -4.f));
    WeaponMagazine = CreateWeaponPart(TEXT("WeaponMagazine"), MagazineBaseLocation, FVector(0.08f, 0.055f, 0.15f), FRotator(0.f, 0.f, 12.f));
    WeaponRail = CreateWeaponPart(TEXT("WeaponRail"), FVector(11.f, 0.f, 7.1f), FVector(0.27f, 0.025f, 0.015f));
    WeaponSight = CreateWeaponPart(TEXT("WeaponSight"), FVector(5.f, 0.f, 9.0f), FVector(0.065f, 0.052f, 0.055f));
    WeaponOpticLens = CreateWeaponPart(TEXT("WeaponOpticLens"), FVector(10.f, 0.f, 9.0f), FVector(0.022f, 0.022f, 0.030f), FRotator(0.f, 90.f, 0.f));
    WeaponGrip = CreateWeaponPart(TEXT("WeaponGrip"), FVector(7.f, 0.f, -11.f), FVector(0.06f, 0.05f, 0.13f), FRotator(0.f, 0.f, 12.f));
    WeaponMuzzle = CreateWeaponPart(TEXT("WeaponMuzzle"), FVector(74.f, 0.f, -0.1f), FVector(0.026f, 0.026f, 0.075f), FRotator(0.f, 90.f, 0.f));

    LeftArm = CreateWeaponPart(TEXT("LeftArm"), FVector(18.f, -11.f, -19.f), FVector(0.24f, 0.055f, 0.055f), FRotator(0.f, -12.f, 16.f));
    RightArm = CreateWeaponPart(TEXT("RightArm"), FVector(-4.f, 10.f, -20.f), FVector(0.27f, 0.055f, 0.055f), FRotator(0.f, 15.f, -18.f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CubeMesh.Succeeded())
    {
        TArray<UStaticMeshComponent*> CubeParts =
        {
            WeaponReceiver, WeaponHandguard, WeaponStock, WeaponMagazine, WeaponRail,
            WeaponSight, WeaponGrip, LeftArm, RightArm
        };
        for (UStaticMeshComponent* Part : CubeParts)
        {
            if (Part) Part->SetStaticMesh(CubeMesh.Object);
        }
    }
    if (CylinderMesh.Succeeded())
    {
        WeaponBarrel->SetStaticMesh(CylinderMesh.Object);
        WeaponMuzzle->SetStaticMesh(CylinderMesh.Object);
        WeaponOpticLens->SetStaticMesh(CylinderMesh.Object);
    }
    else if (CubeMesh.Succeeded())
    {
        WeaponBarrel->SetStaticMesh(CubeMesh.Object);
        WeaponMuzzle->SetStaticMesh(CubeMesh.Object);
        WeaponOpticLens->SetStaticMesh(CubeMesh.Object);
    }

    Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
    Flashlight->SetupAttachment(WeaponRoot);
    Flashlight->SetRelativeLocation(FVector(59.f, 3.f, -5.f));
    Flashlight->SetRelativeRotation(FRotator::ZeroRotator);
    Flashlight->SetIntensity(5200.f);
    Flashlight->SetAttenuationRadius(2600.f);
    Flashlight->SetInnerConeAngle(14.f);
    Flashlight->SetOuterConeAngle(26.f);
    Flashlight->SetLightColor(FLinearColor(1.f, 0.94f, 0.82f));
    Flashlight->SetVisibility(false);
    Flashlight->SetCastShadows(true);

    MuzzleFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlashLight"));
    MuzzleFlashLight->SetupAttachment(WeaponMuzzle);
    MuzzleFlashLight->SetRelativeLocation(FVector(8.f, 0.f, 0.f));
    MuzzleFlashLight->SetIntensity(4200.f);
    MuzzleFlashLight->SetAttenuationRadius(260.f);
    MuzzleFlashLight->SetLightColor(FLinearColor(1.f, 0.42f, 0.12f));
    MuzzleFlashLight->SetCastShadows(false);
    MuzzleFlashLight->SetVisibility(false);
}

void AURFPSCharacter::BeginPlay()
{
    Super::BeginPlay();

    InitialSpawnLocation = GetActorLocation();
    InitialSpawnRotation = GetActorRotation();

    if (UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        WeaponMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        ArmMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);

        if (WeaponMaterial)
        {
            WeaponMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.035f, 0.040f, 0.045f, 1.f));
            const TArray<UStaticMeshComponent*> WeaponParts =
            {
                WeaponReceiver, WeaponHandguard, WeaponBarrel, WeaponStock, WeaponMagazine,
                WeaponRail, WeaponSight, WeaponOpticLens, WeaponGrip, WeaponMuzzle
            };
            for (UStaticMeshComponent* Part : WeaponParts)
            {
                if (Part) Part->SetMaterial(0, WeaponMaterial);
            }
        }

        if (ArmMaterial)
        {
            ArmMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.16f, 0.12f, 0.095f, 1.f));
            if (LeftArm) LeftArm->SetMaterial(0, ArmMaterial);
            if (RightArm) RightArm->SetMaterial(0, ArmMaterial);
        }
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
}

void AURFPSCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    BreathTime += DeltaSeconds;
    if (bReloading) ReloadElapsed += DeltaSeconds;
    if (bTreating) TreatmentElapsed += DeltaSeconds;

    UpdateMedical(DeltaSeconds);
    UpdateMovement(DeltaSeconds);
    UpdateFootsteps(DeltaSeconds);
    UpdateWeaponObstruction(DeltaSeconds);
    UpdateCamera(DeltaSeconds);
    UpdateWeaponPresentation(DeltaSeconds);
    UpdateFeedback(DeltaSeconds);
    UpdateInteractionTarget();

    CurrentBloom = FMath::Max(0.f, CurrentBloom - BloomRecoveryPerSecond * DeltaSeconds);
    SpawnProtectionRemaining = FMath::Max(0.f, SpawnProtectionRemaining - DeltaSeconds);
    MouseInputX = FMath::FInterpTo(MouseInputX, 0.f, DeltaSeconds, 13.f);
    MouseInputY = FMath::FInterpTo(MouseInputY, 0.f, DeltaSeconds, 13.f);
    PreviousVerticalVelocity = GetVelocity().Z;
}

void AURFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AURFPSCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AURFPSCharacter::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &AURFPSCharacter::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &AURFPSCharacter::LookUp);

    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AURFPSCharacter::BeginSprint);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AURFPSCharacter::EndSprint);
    PlayerInputComponent->BindAction("Walk", IE_Pressed, this, &AURFPSCharacter::BeginSlowWalk);
    PlayerInputComponent->BindAction("Walk", IE_Released, this, &AURFPSCharacter::EndSlowWalk);
    PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AURFPSCharacter::ToggleCrouch);
    PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AURFPSCharacter::BeginAim);
    PlayerInputComponent->BindAction("Aim", IE_Released, this, &AURFPSCharacter::EndAim);
    PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AURFPSCharacter::BeginFire);
    PlayerInputComponent->BindAction("Fire", IE_Released, this, &AURFPSCharacter::EndFire);
    PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AURFPSCharacter::Reload);
    PlayerInputComponent->BindAction("Flashlight", IE_Pressed, this, &AURFPSCharacter::ToggleFlashlight);
    PlayerInputComponent->BindAction("FireMode", IE_Pressed, this, &AURFPSCharacter::ToggleFireMode);
    PlayerInputComponent->BindAction("LowReady", IE_Pressed, this, &AURFPSCharacter::ToggleLowReady);
    PlayerInputComponent->BindAction("Zeroing", IE_Pressed, this, &AURFPSCharacter::CycleZeroDistance);
    PlayerInputComponent->BindAction("Grenade", IE_Pressed, this, &AURFPSCharacter::ThrowGrenade);
    PlayerInputComponent->BindAction("Bandage", IE_Pressed, this, &AURFPSCharacter::UseBandage);
    PlayerInputComponent->BindAction("LeanLeft", IE_Pressed, this, &AURFPSCharacter::BeginLeanLeft);
    PlayerInputComponent->BindAction("LeanLeft", IE_Released, this, &AURFPSCharacter::EndLeanLeft);
    PlayerInputComponent->BindAction("LeanRight", IE_Pressed, this, &AURFPSCharacter::BeginLeanRight);
    PlayerInputComponent->BindAction("LeanRight", IE_Released, this, &AURFPSCharacter::EndLeanRight);
    PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AURFPSCharacter::Interact);
    PlayerInputComponent->BindAction("ExitCursor", IE_Pressed, this, &AURFPSCharacter::ExitCursor);
}

void AURFPSCharacter::MoveForward(float Value)
{
    MoveForwardInput = Value;
    if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER && !IsDead())
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
    }
}

void AURFPSCharacter::MoveRight(float Value)
{
    MoveRightInput = Value;
    if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER && !IsDead())
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
    }
}

void AURFPSCharacter::Turn(float Value)
{
    if (IsDead()) return;
    MouseInputX = Value;
    AddControllerYawInput(Value * MouseSensitivity);
}

void AURFPSCharacter::LookUp(float Value)
{
    if (IsDead()) return;
    MouseInputY = Value;
    AddControllerPitchInput(Value * MouseSensitivity);
}

void AURFPSCharacter::BeginSprint()
{
    if (IsDead() || bTreating) return;

    if (bAiming && !bReloading && Stamina > 12.f)
    {
        bHoldingBreath = true;
        bSprinting = false;
        return;
    }

    if (!bReloading && Stamina > 5.f && !bIsCrouched && MoveForwardInput > 0.2f)
    {
        bWalkingSlow = false;
        bHoldingBreath = false;
        bLowReady = false;
        LeanTarget = 0.f;
        bSprinting = true;
    }
}

void AURFPSCharacter::EndSprint()
{
    bSprinting = false;
    bHoldingBreath = false;
}

void AURFPSCharacter::BeginSlowWalk()
{
    if (IsDead()) return;
    bWalkingSlow = true;
    bSprinting = false;
}

void AURFPSCharacter::EndSlowWalk()
{
    bWalkingSlow = false;
}

void AURFPSCharacter::ToggleCrouch()
{
    if (IsDead()) return;
    bSprinting = false;
    bHoldingBreath = false;
    if (bIsCrouched) UnCrouch(); else Crouch();
}

void AURFPSCharacter::BeginAim()
{
    if (bReloading || bTreating || IsDead()) return;
    bLowReady = false;
    bAiming = true;
    bSprinting = false;
}

void AURFPSCharacter::EndAim()
{
    bAiming = false;
    bHoldingBreath = false;
}

void AURFPSCharacter::BeginFire()
{
    if (bReloading || bTreating || IsDead() || bSprinting || bLowReady || WeaponObstructionAlpha > 0.90f) return;

    bTriggerHeld = true;
    const float Interval = 60.f / RoundsPerMinute;

    if (FireModeIndex == 0)
    {
        FireShot();
    }
    else if (FireModeIndex == 1)
    {
        BurstShotsRemaining = 3;
        FireBurstShot();
    }
    else
    {
        FireShot();
        GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AURFPSCharacter::FireShot, Interval, true, Interval);
    }
}

void AURFPSCharacter::EndFire()
{
    bTriggerHeld = false;
    BurstShotsRemaining = 0;
    GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void AURFPSCharacter::FireBurstShot()
{
    if (!bTriggerHeld || BurstShotsRemaining <= 0 || bReloading || bTreating || IsDead() || bLowReady)
    {
        EndFire();
        return;
    }

    FireShot();
    --BurstShotsRemaining;

    if (BurstShotsRemaining > 0 && AmmoInMagazine > 0)
    {
        const float Interval = 60.f / RoundsPerMinute;
        GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AURFPSCharacter::FireBurstShot, Interval, false);
    }
    else
    {
        EndFire();
    }
}

float AURFPSCharacter::GetCurrentSpreadDegrees() const
{
    const float SpeedAlpha = FMath::Clamp(GetVelocity().Size2D() / SprintSpeed, 0.f, 1.f);
    float Spread = (bAiming ? AimSpreadDegrees : HipSpreadDegrees) + CurrentBloom;
    Spread += SpeedAlpha * (bAiming ? 0.16f : 0.58f);
    Spread += SuppressionFeedback * (bAiming ? 0.30f : 0.46f);
    Spread += BleedSeverity * (bAiming ? 0.16f : 0.10f);
    if (bWalkingSlow) Spread *= 0.86f;
    if (bIsCrouched) Spread *= 0.72f;
    if (bHoldingBreath && bAiming) Spread *= 0.58f;
    if (bSprinting) Spread += 1.5f;
    if (Stamina < 20.f && bAiming) Spread += (20.f - Stamina) * 0.006f;
    return Spread;
}

float AURFPSCharacter::GetReloadProgress() const
{
    if (!bReloading || ActiveReloadDuration <= KINDA_SMALL_NUMBER) return 0.f;
    return FMath::Clamp(ReloadElapsed / ActiveReloadDuration, 0.f, 1.f);
}

float AURFPSCharacter::GetTreatmentProgress() const
{
    if (!bTreating || TreatmentDuration <= KINDA_SMALL_NUMBER) return 0.f;
    return FMath::Clamp(TreatmentElapsed / TreatmentDuration, 0.f, 1.f);
}

float AURFPSCharacter::GetAccuracyPercent() const
{
    if (ShotsFired <= 0) return 0.f;
    return 100.f * static_cast<float>(ConfirmedHits) / static_cast<float>(ShotsFired);
}

FString AURFPSCharacter::GetFireModeName() const
{
    if (FireModeIndex == 0) return TEXT("SEMI");
    if (FireModeIndex == 1) return TEXT("BURST");
    return TEXT("AUTO");
}

int32 AURFPSCharacter::GetZeroDistanceMeters() const
{
    switch (ZeroDistanceIndex)
    {
    case 0: return 50;
    case 2: return 200;
    case 3: return 300;
    default: return 100;
    }
}

FVector AURFPSCharacter::GetShotDirection() const
{
    FVector BaseDirection = FirstPersonCamera->GetForwardVector();
    if (bAiming)
    {
        const float LowStaminaScale = Stamina < 25.f ? 1.6f : 1.f;
        const float HoldScale = bHoldingBreath ? 0.25f : 1.f;
        const float SuppressionScale = 1.f + SuppressionFeedback * 1.4f;
        const FRotator BreathSway(
            FMath::Sin(BreathTime * 1.55f) * 0.028f * LowStaminaScale * HoldScale * SuppressionScale,
            FMath::Cos(BreathTime * 1.13f) * 0.022f * LowStaminaScale * HoldScale * SuppressionScale,
            0.f);
        BaseDirection = BreathSway.RotateVector(BaseDirection).GetSafeNormal();
    }

    const float ConeHalfAngleRadians = FMath::DegreesToRadians(GetCurrentSpreadDegrees());
    return FMath::VRandCone(BaseDirection, ConeHalfAngleRadians);
}

void AURFPSCharacter::FireShot()
{
    if (bReloading || bTreating || IsDead() || bSprinting || bLowReady || WeaponObstructionAlpha > 0.90f)
    {
        if (WeaponObstructionAlpha > 0.90f || bLowReady || bTreating) EndFire();
        return;
    }

    if (AmmoInMagazine <= 0)
    {
        EndFire();
        URFPSAudio::PlayLocal(this, EURFPSAudioEvent::DryFire, 0.55f, FMath::FRandRange(0.98f, 1.03f));
        Reload();
        return;
    }

    --AmmoInMagazine;
    ++ShotCounter;
    ++ShotsFired;
    CurrentBloom = FMath::Min(MaxBloom, CurrentBloom + BloomPerShot * (bAiming ? 0.70f : 1.f));

    const FVector CameraStart = FirstPersonCamera->GetComponentLocation();
    const FVector CameraDirection = GetShotDirection();
    const FVector CameraEnd = CameraStart + CameraDirection * FireRange;

    FCollisionQueryParams AimParams(SCENE_QUERY_STAT(PlayerAimTrace), true, this);
    AimParams.AddIgnoredActor(this);

    FHitResult AimHit;
    FVector AimPoint = CameraEnd;
    if (GetWorld()->LineTraceSingleByChannel(AimHit, CameraStart, CameraEnd, ECC_Visibility, AimParams))
    {
        AimPoint = AimHit.ImpactPoint;
    }

    const FVector MuzzleStart = WeaponMuzzle
        ? WeaponMuzzle->GetComponentLocation() + FirstPersonCamera->GetForwardVector() * 5.f
        : CameraStart;
    FVector BallisticDirection = (AimPoint - MuzzleStart).GetSafeNormal();

    // Mechanical zeroing: the barrel receives a small elevation angle so the projectile arc
    // intersects the sight line at the selected zero distance. With an 820 m/s rifle round
    // the correction is subtle at 50-100 m but becomes noticeable at 200-300 m.
    const float ZeroDistanceCm = static_cast<float>(GetZeroDistanceMeters()) * 100.f;
    const float ZeroFlightTime = ZeroDistanceCm / FMath::Max(BulletMuzzleVelocity, 1.f);
    const float ZeroDropCm = 0.5f * 980.f * ZeroFlightTime * ZeroFlightTime;
    const float ZeroAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(ZeroDropCm, ZeroDistanceCm));
    FRotator BallisticRotation = BallisticDirection.Rotation();
    BallisticRotation.Pitch += ZeroAngleDegrees;
    BallisticDirection = BallisticRotation.Vector();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AURFPSProjectile* Projectile = GetWorld()->SpawnActor<AURFPSProjectile>(
        AURFPSProjectile::StaticClass(), MuzzleStart, BallisticDirection.Rotation(), SpawnParams))
    {
        const bool bTracerRound = (ShotCounter % 4) == 0;
        Projectile->Initialize(BallisticDirection * BulletMuzzleVelocity, BaseDamage, GetController(), this, bTracerRound);
    }

    URFPSAudio::PlayGunshot(this, MuzzleStart, false, 1.0f, FMath::FRandRange(0.985f, 1.02f));
    SpawnShellCasing();
    NotifyEnemiesOfGunshot(CameraDirection);

    if (MuzzleFlashLight)
    {
        MuzzleFlashLight->SetVisibility(true);
        GetWorldTimerManager().ClearTimer(MuzzleFlashTimerHandle);
        GetWorldTimerManager().SetTimer(MuzzleFlashTimerHandle, this, &AURFPSCharacter::HideMuzzleFlash, 0.045f, false);
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        const float RecoilScale = bAiming ? 0.67f : 1.f;
        const float HoldBreathRecoil = bHoldingBreath ? 0.94f : 1.f;
        const float BurstPattern = FireModeIndex == 1 ? (1.f + static_cast<float>(3 - BurstShotsRemaining) * 0.08f) : 1.f;
        PC->AddPitchInput(-VerticalRecoil * RecoilScale * HoldBreathRecoil * BurstPattern);
        PC->AddYawInput(FMath::FRandRange(-HorizontalRecoil, HorizontalRecoil) * RecoilScale * BurstPattern);
    }

    WeaponKickLocation += FVector(-4.6f, FMath::FRandRange(-0.3f, 0.3f), FMath::FRandRange(-0.4f, 0.15f));
    WeaponKickRotation += FRotator(FMath::FRandRange(-2.15f, -1.15f), FMath::FRandRange(-0.75f, 0.75f), FMath::FRandRange(-0.6f, 0.6f));
}

void AURFPSCharacter::Reload()
{
    if (IsDead())
    {
        RespawnSelf();
        return;
    }

    const int32 TacticalMaximum = MagazineCapacity + 1;
    if (bReloading || bTreating || AmmoInMagazine >= TacticalMaximum || ReserveAmmo <= 0) return;

    EndFire();
    bLowReady = false;
    bReloadStartedWithChamberedRound = AmmoInMagazine > 0;
    ActiveReloadDuration = bReloadStartedWithChamberedRound ? TacticalReloadDuration : EmergencyReloadDuration;
    bReloading = true;
    ReloadElapsed = 0.f;
    bSprinting = false;
    bAiming = false;
    bHoldingBreath = false;
    URFPSAudio::PlayLocal(this, EURFPSAudioEvent::ReloadStart, 0.62f, FMath::FRandRange(0.97f, 1.03f));
    GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AURFPSCharacter::FinishReload, ActiveReloadDuration, false);
}

void AURFPSCharacter::FinishReload()
{
    const int32 TargetLoadedRounds = MagazineCapacity + (bReloadStartedWithChamberedRound ? 1 : 0);
    const int32 Needed = FMath::Max(0, TargetLoadedRounds - AmmoInMagazine);
    const int32 ToLoad = FMath::Min(Needed, ReserveAmmo);
    AmmoInMagazine += ToLoad;
    ReserveAmmo -= ToLoad;
    bReloading = false;
    ReloadElapsed = 0.f;
    URFPSAudio::PlayLocal(this, EURFPSAudioEvent::ReloadEnd, 0.66f, FMath::FRandRange(0.98f, 1.02f));
}

void AURFPSCharacter::ToggleFlashlight()
{
    if (!Flashlight || IsDead()) return;
    bFlashlightOn = !bFlashlightOn;
    Flashlight->SetVisibility(bFlashlightOn);
}

void AURFPSCharacter::ToggleFireMode()
{
    if (bReloading || bTreating || IsDead()) return;
    EndFire();
    FireModeIndex = (FireModeIndex + 1) % 3;
}

void AURFPSCharacter::ToggleLowReady()
{
    if (IsDead() || bReloading || bTreating) return;
    EndFire();
    bAiming = false;
    bHoldingBreath = false;
    bSprinting = false;
    bLowReady = !bLowReady;
}

void AURFPSCharacter::CycleZeroDistance()
{
    if (IsDead() || bReloading || bTreating) return;
    ZeroDistanceIndex = (ZeroDistanceIndex + 1) % 4;
}

void AURFPSCharacter::ThrowGrenade()
{
    if (IsDead() || bReloading || bTreating || bSprinting || GrenadeCount <= 0 || !GetWorld() || !FirstPersonCamera) return;

    EndFire();
    bAiming = false;
    bHoldingBreath = false;
    bLowReady = false;
    --GrenadeCount;

    const FVector Forward = FirstPersonCamera->GetForwardVector();
    const FVector Right = FirstPersonCamera->GetRightVector();
    const FVector SpawnLocation = FirstPersonCamera->GetComponentLocation() + Forward * 45.f + Right * 9.f - FVector(0.f, 0.f, 10.f);
    const FVector ThrowVelocity = Forward * 1280.f + GetVelocity() * 0.35f + FVector(0.f, 0.f, 280.f);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AURFPSGrenade* Grenade = GetWorld()->SpawnActor<AURFPSGrenade>(
        AURFPSGrenade::StaticClass(), SpawnLocation, Forward.Rotation(), SpawnParams))
    {
        Grenade->InitializeGrenade(ThrowVelocity, GetController(), this);
    }

    WeaponKickLocation += FVector(-7.f, 7.f, -12.f);
    WeaponKickRotation += FRotator(-18.f, 5.f, 18.f);
}

void AURFPSCharacter::UseBandage()
{
    if (IsDead() || bTreating || bReloading || BandageCount <= 0 || BleedSeverity <= 0.01f) return;

    EndFire();
    bTreating = true;
    TreatmentElapsed = 0.f;
    bAiming = false;
    bSprinting = false;
    bHoldingBreath = false;
    bLowReady = false;
    LeanTarget = 0.f;
    GetWorldTimerManager().SetTimer(TreatmentTimerHandle, this, &AURFPSCharacter::FinishBandage, TreatmentDuration, false);
}

void AURFPSCharacter::FinishBandage()
{
    if (!bTreating) return;
    bTreating = false;
    TreatmentElapsed = 0.f;
    BandageCount = FMath::Max(0, BandageCount - 1);
    BleedSeverity = 0.f;
    BleedDamageAccumulator = 0.f;
    Health = FMath::Min(MaxHealth, Health + 8.f);
}

void AURFPSCharacter::CancelTreatment()
{
    if (!bTreating) return;
    bTreating = false;
    TreatmentElapsed = 0.f;
    GetWorldTimerManager().ClearTimer(TreatmentTimerHandle);
}

void AURFPSCharacter::BeginLeanLeft()
{
    if (!IsDead() && !bSprinting) LeanTarget = -1.f;
}

void AURFPSCharacter::EndLeanLeft()
{
    if (LeanTarget < 0.f) LeanTarget = 0.f;
}

void AURFPSCharacter::BeginLeanRight()
{
    if (!IsDead() && !bSprinting) LeanTarget = 1.f;
}

void AURFPSCharacter::EndLeanRight()
{
    if (LeanTarget > 0.f) LeanTarget = 0.f;
}

FString AURFPSCharacter::GetInteractionPrompt() const
{
    if (!InteractionTarget.IsValid() || IsDead()) return FString();
    if (const AURFPSDoor* Door = Cast<AURFPSDoor>(InteractionTarget.Get())) return Door->GetInteractionText();
    if (InteractionTarget->ActorHasTag(FName(TEXT("SupplyAmmo")))) return TEXT("E  PRENDRE MUNITIONS");
    if (InteractionTarget->ActorHasTag(FName(TEXT("SupplyMedical")))) return TEXT("E  UTILISER KIT MEDICAL");
    if (InteractionTarget->ActorHasTag(FName(TEXT("SupplyGrenade")))) return TEXT("E  PRENDRE GRENADES");
    if (InteractionTarget->ActorHasTag(FName(TEXT("WaveControl"))))
    {
        if (UWorld* World = GetWorld())
        {
            if (const AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(World->GetAuthGameMode()))
            {
                if (GameMode->IsWaveCleared()) return TEXT("E  LANCER LA PROCHAINE VAGUE");
            }
        }
    }
    return FString();
}

void AURFPSCharacter::Interact()
{
    if (!InteractionTarget.IsValid() || IsDead() || bTreating) return;

    AActor* Target = InteractionTarget.Get();
    bool bConsumed = false;

    if (AURFPSDoor* Door = Cast<AURFPSDoor>(Target))
    {
        Door->Interact(this);
        NotifyEnemiesOfNoise(Door->GetActorLocation(), 1050.f, 0.72f);
        return;
    }

    if (Target->ActorHasTag(FName(TEXT("SupplyAmmo"))))
    {
        if (ReserveAmmo < MaxReserveAmmo)
        {
            ReserveAmmo = FMath::Min(MaxReserveAmmo, ReserveAmmo + 90);
            bConsumed = true;
        }
    }
    else if (Target->ActorHasTag(FName(TEXT("SupplyMedical"))))
    {
        if (Health < MaxHealth || Armor < MaxArmor || BandageCount < MaxBandages || BleedSeverity > 0.f)
        {
            Health = FMath::Min(MaxHealth, Health + 30.f);
            Armor = FMath::Min(MaxArmor, Armor + 24.f);
            BandageCount = FMath::Min(MaxBandages, BandageCount + 1);
            BleedSeverity = FMath::Max(0.f, BleedSeverity - 0.70f);
            bConsumed = true;
        }
    }
    else if (Target->ActorHasTag(FName(TEXT("SupplyGrenade"))))
    {
        if (GrenadeCount < MaxGrenades)
        {
            GrenadeCount = FMath::Min(MaxGrenades, GrenadeCount + 2);
            bConsumed = true;
        }
    }
    else if (Target->ActorHasTag(FName(TEXT("WaveControl"))))
    {
        if (AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(GetWorld()->GetAuthGameMode()))
        {
            GameMode->RequestNextWave();
        }
    }

    if (bConsumed)
    {
        Target->Destroy();
        InteractionTarget.Reset();
    }
}

void AURFPSCharacter::ExitCursor()
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = !PC->bShowMouseCursor;
        if (PC->bShowMouseCursor)
        {
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(Mode);
        }
        else
        {
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
}

void AURFPSCharacter::HideMuzzleFlash()
{
    if (MuzzleFlashLight) MuzzleFlashLight->SetVisibility(false);
}

void AURFPSCharacter::RespawnSelf()
{
    EndFire();
    GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
    GetWorldTimerManager().ClearTimer(MuzzleFlashTimerHandle);
    GetWorldTimerManager().ClearTimer(TreatmentTimerHandle);

    Health = MaxHealth;
    Armor = MaxArmor;
    Stamina = MaxStamina;
    AmmoInMagazine = MagazineCapacity;
    ReserveAmmo = 150;
    GrenadeCount = 2;
    BandageCount = 2;
    BleedSeverity = 0.f;
    BleedDamageAccumulator = 0.f;
    SpawnProtectionRemaining = 1.75f;
    CurrentBloom = 0.f;
    DamageFeedback = 0.f;
    DamageDirectionAlpha = 0.f;
    SuppressionFeedback = 0.f;
    WeaponObstructionAlpha = 0.f;
    bReloading = false;
    bTreating = false;
    bAiming = false;
    bSprinting = false;
    bWalkingSlow = false;
    bHoldingBreath = false;
    bLowReady = false;
    LeanTarget = 0.f;
    LeanCurrent = 0.f;
    ReloadElapsed = 0.f;
    TreatmentElapsed = 0.f;

    if (Flashlight)
    {
        bFlashlightOn = false;
        Flashlight->SetVisibility(false);
    }
    if (MuzzleFlashLight) MuzzleFlashLight->SetVisibility(false);

    SetActorLocationAndRotation(InitialSpawnLocation, InitialSpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetControlRotation(InitialSpawnRotation);
    }
}

void AURFPSCharacter::UpdateMovement(float DeltaSeconds)
{
    if (IsDead()) return;

    const bool bMoving = GetVelocity().SizeSquared2D() > 100.f;
    const bool bCanMaintainSprint = bSprinting && !bTreating && bMoving && MoveForwardInput > 0.15f && !bAiming && !bIsCrouched && Stamina > 0.f;

    if (bHoldingBreath && bAiming)
    {
        Stamina = FMath::Max(0.f, Stamina - HoldBreathDrainPerSecond * DeltaSeconds);
        StaminaRecoveryCooldown = FMath::Max(StaminaRecoveryCooldown, 1.1f);
        if (Stamina <= 1.f) bHoldingBreath = false;
    }
    else if (bCanMaintainSprint)
    {
        Stamina = FMath::Max(0.f, Stamina - StaminaDrainPerSecond * DeltaSeconds);
        StaminaRecoveryCooldown = StaminaRecoveryDelay;
        if (Stamina <= 0.f) bSprinting = false;
    }
    else
    {
        if (bSprinting && (!bMoving || MoveForwardInput <= 0.15f)) bSprinting = false;
        StaminaRecoveryCooldown = FMath::Max(0.f, StaminaRecoveryCooldown - DeltaSeconds);
        if (StaminaRecoveryCooldown <= 0.f)
        {
            const float RecoveryScale = BleedSeverity > 0.45f ? 0.72f : 1.f;
            Stamina = FMath::Min(MaxStamina, Stamina + StaminaRecoveryPerSecond * RecoveryScale * DeltaSeconds);
        }
    }

    float TargetSpeed = WalkSpeed;
    if (bTreating) TargetSpeed = 105.f;
    else if (bIsCrouched) TargetSpeed = CrouchSpeed;
    else if (bSprinting) TargetSpeed = SprintSpeed;
    else if (bWalkingSlow) TargetSpeed = SlowWalkSpeed;

    if (BleedSeverity > 0.65f && !bSprinting)
    {
        TargetSpeed = FMath::Min(TargetSpeed, InjuredWalkSpeed);
    }

    GetCharacterMovement()->MaxWalkSpeed = FMath::FInterpTo(GetCharacterMovement()->MaxWalkSpeed, TargetSpeed, DeltaSeconds, 8.f);
}

void AURFPSCharacter::UpdateWeaponObstruction(float DeltaSeconds)
{
    float TargetObstruction = 0.f;
    if (GetWorld() && FirstPersonCamera && !IsDead())
    {
        const FVector Start = FirstPersonCamera->GetComponentLocation();
        const FVector End = Start + FirstPersonCamera->GetForwardVector() * 118.f;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponObstruction), false, this);
        Params.AddIgnoredActor(this);

        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            if (!Cast<AURFPSEnemy>(Hit.GetActor()))
            {
                TargetObstruction = FMath::Clamp(1.f - Hit.Distance / 118.f, 0.f, 1.f);
            }
        }
    }

    WeaponObstructionAlpha = FMath::FInterpTo(WeaponObstructionAlpha, TargetObstruction, DeltaSeconds, 14.f);
}

void AURFPSCharacter::UpdateCamera(float DeltaSeconds)
{
    const float HorizontalSpeed = GetVelocity().Size2D();
    const bool bGroundMoving = HorizontalSpeed > 15.f && GetCharacterMovement()->IsMovingOnGround() && !IsDead();

    const float BobFrequency = bSprinting ? 12.5f : (bWalkingSlow ? 6.4f : (bIsCrouched ? 7.2f : 9.2f));
    if (bGroundMoving)
    {
        HeadBobTime += DeltaSeconds * BobFrequency * FMath::Clamp(HorizontalSpeed / WalkSpeed, 0.45f, 1.45f);
    }

    const float AimScale = bAiming ? 0.22f : 1.f;
    const float WalkScale = bWalkingSlow ? 0.58f : 1.f;
    const float InjuryScale = BleedSeverity > 0.5f ? 1.0f + BleedSeverity * 0.30f : 1.f;
    const float BobAmplitude = (bSprinting ? 2.1f : (bIsCrouched ? 0.7f : 1.15f)) * AimScale * WalkScale * InjuryScale;
    const float BobZ = bGroundMoving ? FMath::Sin(HeadBobTime) * BobAmplitude : 0.f;
    const float BobY = bGroundMoving ? FMath::Cos(HeadBobTime * 0.5f) * BobAmplitude * 0.55f : 0.f;

    LandingKick = FMath::FInterpTo(LandingKick, 0.f, DeltaSeconds, 10.f);

    float EffectiveLeanTarget = LeanTarget;
    if (GetWorld() && FMath::Abs(LeanTarget) > 0.01f)
    {
        const FVector LeanStart = GetActorLocation() + FVector(0.f, 0.f, bIsCrouched ? 47.f : 64.f);
        const FVector LeanOffsetWorld = GetActorRightVector() * (LeanTarget * 18.f);
        FCollisionQueryParams LeanParams(SCENE_QUERY_STAT(LeanClearance), false, this);
        LeanParams.AddIgnoredActor(this);
        FHitResult LeanHit;
        if (GetWorld()->LineTraceSingleByChannel(LeanHit, LeanStart, LeanStart + LeanOffsetWorld, ECC_Visibility, LeanParams))
        {
            const float SafeAlpha = FMath::Clamp((LeanHit.Distance - 3.f) / FMath::Max(LeanOffsetWorld.Size(), 1.f), 0.f, 1.f);
            EffectiveLeanTarget *= SafeAlpha;
        }
    }
    LeanCurrent = FMath::FInterpTo(LeanCurrent, EffectiveLeanTarget, DeltaSeconds, 10.f);

    const float SuppressionJitter = SuppressionFeedback * 0.45f;
    const float SuppressionY = FMath::Sin(BreathTime * 15.3f) * SuppressionJitter;
    const float SuppressionZ = FMath::Cos(BreathTime * 12.7f) * SuppressionJitter * 0.65f;
    const float InjurySway = BleedSeverity * 0.22f;
    const float CrouchOffset = bIsCrouched ? -15.f : 0.f;
    const FVector LeanOffset(0.f, LeanCurrent * 13.f, -FMath::Abs(LeanCurrent) * 1.5f);
    const FVector CameraTarget = BaseCameraLocation + FVector(
        LandingKick,
        BobY + SuppressionY + FMath::Sin(BreathTime * 1.8f) * InjurySway,
        CrouchOffset + BobZ + SuppressionZ + FMath::Cos(BreathTime * 1.35f) * InjurySway) + LeanOffset;
    FirstPersonCamera->SetRelativeLocation(FMath::VInterpTo(FirstPersonCamera->GetRelativeLocation(), CameraTarget, DeltaSeconds, 12.f));
    FirstPersonCamera->SetRelativeRotation(FRotator(0.f, 0.f, LeanCurrent * 8.f));

    float TargetFOV = HipFOV;
    if (bAiming) TargetFOV = AimFOV;
    else if (bSprinting) TargetFOV = SprintFOV;
    FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaSeconds, FOVInterpSpeed));
}

void AURFPSCharacter::UpdateWeaponPresentation(float DeltaSeconds)
{
    const float AimScale = bAiming ? 0.28f : 1.f;
    const float SpeedAlpha = FMath::Clamp(GetVelocity().Size2D() / SprintSpeed, 0.f, 1.f);
    const bool bMoving = GetVelocity().SizeSquared2D() > 100.f && GetCharacterMovement()->IsMovingOnGround();

    FVector TargetLocation = HipWeaponLocation;
    FRotator TargetRotation = HipWeaponRotation;

    if (bAiming)
    {
        TargetLocation = AimWeaponLocation;
        const float HoldScale = bHoldingBreath ? 0.24f : 1.f;
        const float SuppressionScale = 1.f + SuppressionFeedback * 1.3f;
        TargetRotation.Pitch += FMath::Sin(BreathTime * 1.55f) * 0.12f * HoldScale * SuppressionScale;
        TargetRotation.Yaw += FMath::Cos(BreathTime * 1.13f) * 0.09f * HoldScale * SuppressionScale;
    }
    else if (bSprinting)
    {
        TargetLocation = SprintWeaponLocation;
        TargetRotation = SprintWeaponRotation;
    }
    else if (bTreating)
    {
        TargetLocation = HipWeaponLocation + FVector(-20.f, 13.f, -27.f);
        TargetRotation = FRotator(-35.f, 8.f, 30.f);
    }
    else if (bReloading)
    {
        TargetLocation = HipWeaponLocation + FVector(-5.f, 10.f, -11.f);
        TargetRotation = FRotator(-12.f, 2.f, 24.f);
    }
    else if (bLowReady)
    {
        TargetLocation = LowReadyWeaponLocation;
        TargetRotation = LowReadyWeaponRotation;
    }

    const FVector MouseSway(0.f, -MouseInputX * 0.33f * AimScale, MouseInputY * 0.27f * AimScale);
    FVector WalkSway = FVector::ZeroVector;
    if (bMoving && !bAiming)
    {
        const float WalkMultiplier = bWalkingSlow ? 0.55f : 1.f;
        WalkSway.Y = FMath::Sin(HeadBobTime * 0.5f) * 0.9f * SpeedAlpha * WalkMultiplier;
        WalkSway.Z = FMath::Abs(FMath::Sin(HeadBobTime)) * 0.8f * SpeedAlpha * WalkMultiplier;
    }

    WeaponKickLocation = FMath::VInterpTo(WeaponKickLocation, FVector::ZeroVector, DeltaSeconds, 16.f);
    WeaponKickRotation = FMath::RInterpTo(WeaponKickRotation, FRotator::ZeroRotator, DeltaSeconds, 15.f);

    TargetLocation += MouseSway + WalkSway + WeaponKickLocation;
    TargetRotation += WeaponKickRotation;

    if (WeaponObstructionAlpha > 0.01f && !bSprinting && !bReloading && !bTreating)
    {
        TargetLocation += FVector(-18.f, 7.f, -4.f) * WeaponObstructionAlpha;
        TargetRotation += FRotator(28.f, 9.f, -15.f) * WeaponObstructionAlpha;
    }

    WeaponRoot->SetRelativeLocation(FMath::VInterpTo(WeaponRoot->GetRelativeLocation(), TargetLocation, DeltaSeconds, bAiming ? 18.f : 12.f));
    WeaponRoot->SetRelativeRotation(FMath::RInterpTo(WeaponRoot->GetRelativeRotation(), TargetRotation, DeltaSeconds, bAiming ? 18.f : 12.f));

    if (WeaponMagazine)
    {
        FVector TargetMagazineLocation = MagazineBaseLocation;
        FRotator TargetMagazineRotation(0.f, 0.f, 12.f);
        if (bReloading && ActiveReloadDuration > KINDA_SMALL_NUMBER)
        {
            const float Phase = FMath::Clamp(ReloadElapsed / ActiveReloadDuration, 0.f, 1.f);
            const float MagazineMotion = FMath::Sin(Phase * PI);
            TargetMagazineLocation += FVector(-2.f * MagazineMotion, -7.f * MagazineMotion, -28.f * MagazineMotion);
            TargetMagazineRotation.Roll += 26.f * MagazineMotion;
        }
        WeaponMagazine->SetRelativeLocation(FMath::VInterpTo(WeaponMagazine->GetRelativeLocation(), TargetMagazineLocation, DeltaSeconds, 15.f));
        WeaponMagazine->SetRelativeRotation(FMath::RInterpTo(WeaponMagazine->GetRelativeRotation(), TargetMagazineRotation, DeltaSeconds, 15.f));
    }
}

void AURFPSCharacter::UpdateFeedback(float DeltaSeconds)
{
    DamageFeedback = FMath::FInterpTo(DamageFeedback, 0.f, DeltaSeconds, 4.0f);
    DamageDirectionAlpha = FMath::Max(0.f, DamageDirectionAlpha - DeltaSeconds * 1.55f);
    HitMarkerAlpha = FMath::Max(0.f, HitMarkerAlpha - DeltaSeconds * 4.2f);
    HeadshotMarkerAlpha = FMath::Max(0.f, HeadshotMarkerAlpha - DeltaSeconds * 3.5f);
    SuppressionFeedback = FMath::Max(0.f, SuppressionFeedback - DeltaSeconds * 0.62f);
}

void AURFPSCharacter::UpdateMedical(float DeltaSeconds)
{
    if (IsDead() || BleedSeverity <= 0.01f) return;

    const float BleedPerSecond = FMath::Lerp(0.12f, 0.72f, FMath::Clamp(BleedSeverity, 0.f, 1.f));
    BleedDamageAccumulator += BleedPerSecond * DeltaSeconds;

    if (BleedDamageAccumulator >= 0.20f)
    {
        const float DamageTick = BleedDamageAccumulator;
        BleedDamageAccumulator = 0.f;
        Health = FMath::Max(0.f, Health - DamageTick);
        DamageFeedback = FMath::Max(DamageFeedback, FMath::Clamp(BleedSeverity * 0.24f, 0.f, 0.35f));

        if (Health <= 0.f)
        {
            HandleDeath();
        }
    }
}

void AURFPSCharacter::UpdateInteractionTarget()
{
    InteractionTarget.Reset();
    if (!GetWorld() || IsDead() || !FirstPersonCamera) return;

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * 285.f;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false, this);
    Params.AddIgnoredActor(this);

    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        AActor* Actor = Hit.GetActor();
        if (Actor && (Cast<AURFPSDoor>(Actor) ||
            Actor->ActorHasTag(FName(TEXT("SupplyAmmo"))) ||
            Actor->ActorHasTag(FName(TEXT("SupplyMedical"))) ||
            Actor->ActorHasTag(FName(TEXT("SupplyGrenade"))) ||
            Actor->ActorHasTag(FName(TEXT("WaveControl")))))
        {
            InteractionTarget = Actor;
        }
    }
}

void AURFPSCharacter::UpdateFootsteps(float DeltaSeconds)
{
    if (IsDead() || !GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
    {
        FootstepDistanceAccumulator = 0.f;
        return;
    }

    const float Speed = GetVelocity().Size2D();
    if (Speed < 72.f)
    {
        FootstepDistanceAccumulator = FMath::Max(0.f, FootstepDistanceAccumulator - DeltaSeconds * 90.f);
        return;
    }

    FootstepDistanceAccumulator += Speed * DeltaSeconds;

    float StepDistance = 158.f;
    if (bSprinting) StepDistance = 142.f;
    else if (bWalkingSlow) StepDistance = 192.f;
    else if (bIsCrouched) StepDistance = 176.f;

    if (FootstepDistanceAccumulator >= StepDistance)
    {
        FootstepDistanceAccumulator = FMath::Fmod(FootstepDistanceAccumulator, StepDistance);
        float Volume = 0.62f;
        if (bSprinting) Volume = 0.94f;
        else if (bWalkingSlow) Volume = 0.36f;
        else if (bIsCrouched) Volume = 0.28f;
        PlayFootstep(Volume);
    }
}

void AURFPSCharacter::PlayFootstep(float VolumeMultiplier)
{
    if (!GetWorld()) return;

    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 26.f);
    const FVector End = GetActorLocation() - FVector(0.f, 0.f, 145.f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerFootstepSurface), false, this);
    Params.AddIgnoredActor(this);
    Params.bReturnPhysicalMaterial = true;

    FHitResult Hit;
    EURFPSAudioEvent Event = EURFPSAudioEvent::FootstepConcrete;
    FVector SoundLocation = GetActorLocation() - FVector(0.f, 0.f, 78.f);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        SoundLocation = Hit.ImpactPoint + Hit.ImpactNormal * 3.f;
        const EPhysicalSurface Surface = UGameplayStatics::GetSurfaceType(Hit);
        if (Surface == SurfaceType2) Event = EURFPSAudioEvent::FootstepMetal;
        else if (Surface == SurfaceType3) Event = EURFPSAudioEvent::FootstepWood;
    }

    const float ClampedVolume = FMath::Clamp(VolumeMultiplier, 0.18f, 1.15f);
    URFPSAudio::PlaySpatial(this, Event, SoundLocation, ClampedVolume, FMath::FRandRange(0.93f, 1.07f));

    float AudibleRange = 210.f;
    float AlertStrength = 0.32f;
    if (ClampedVolume >= 0.80f)
    {
        AudibleRange = 1050.f;
        AlertStrength = 0.72f;
    }
    else if (ClampedVolume >= 0.52f)
    {
        AudibleRange = 610.f;
        AlertStrength = 0.50f;
    }
    else if (ClampedVolume >= 0.32f)
    {
        AudibleRange = 360.f;
        AlertStrength = 0.38f;
    }
    NotifyEnemiesOfNoise(SoundLocation, AudibleRange, AlertStrength);
}

void AURFPSCharacter::NotifyEnemiesOfNoise(const FVector& SourceLocation, float AudibleRange, float AlertStrength)
{
    if (!GetWorld() || AudibleRange <= 0.f) return;

    for (TActorIterator<AURFPSEnemy> It(GetWorld()); It; ++It)
    {
        AURFPSEnemy* Enemy = *It;
        if (!IsValid(Enemy) || Enemy->IsEnemyDead()) continue;

        const float Distance = FVector::Dist2D(SourceLocation, Enemy->GetActorLocation());
        if (Distance > AudibleRange) continue;

        float EffectiveRange = AudibleRange;
        FCollisionQueryParams SoundParams(SCENE_QUERY_STAT(PlayerTacticalNoiseOcclusion), true, this);
        SoundParams.AddIgnoredActor(this);
        SoundParams.AddIgnoredActor(Enemy);
        FHitResult SoundHit;
        const FVector EnemyEar = Enemy->GetActorLocation() + FVector(0.f, 0.f, 52.f);
        if (GetWorld()->LineTraceSingleByChannel(SoundHit, SourceLocation, EnemyEar, ECC_Visibility, SoundParams))
        {
            EffectiveRange *= 0.48f;
        }

        if (Distance <= EffectiveRange)
        {
            Enemy->AlertFromNoise(SourceLocation, EffectiveRange, AlertStrength);
        }
    }
}

void AURFPSCharacter::NotifyEnemiesOfGunshot(const FVector& ShotDirection)
{
    if (!GetWorld()) return;

    const FVector Start = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
    const FVector Direction = ShotDirection.GetSafeNormal();

    for (TActorIterator<AURFPSEnemy> It(GetWorld()); It; ++It)
    {
        AURFPSEnemy* Enemy = *It;
        if (!IsValid(Enemy) || Enemy->IsEnemyDead()) continue;

        const FVector ToEnemy = Enemy->GetActorLocation() - Start;
        const float Distance = ToEnemy.Size();

        float AudibleRange = 6200.f;
        if (Distance <= AudibleRange)
        {
            FCollisionQueryParams SoundParams(SCENE_QUERY_STAT(GunshotOcclusion), true, this);
            SoundParams.AddIgnoredActor(this);
            FHitResult SoundHit;
            const FVector EnemyEar = Enemy->GetActorLocation() + FVector(0.f, 0.f, 52.f);
            if (GetWorld()->LineTraceSingleByChannel(SoundHit, Start, EnemyEar, ECC_Visibility, SoundParams))
            {
                if (SoundHit.GetActor() != Enemy)
                {
                    AudibleRange = 2700.f;
                }
            }

            if (Distance <= AudibleRange)
            {
                Enemy->AlertFromGunshot(GetActorLocation());
            }
        }

        const float ForwardDistance = FVector::DotProduct(ToEnemy, Direction);
        if (ForwardDistance > 100.f && ForwardDistance < 5000.f)
        {
            const FVector ClosestPoint = Start + Direction * ForwardDistance;
            const float MissDistance = FVector::Dist(ClosestPoint, Enemy->GetActorLocation());
            if (MissDistance < 230.f)
            {
                Enemy->ApplySuppression(GetActorLocation());
            }
        }
    }
}

void AURFPSCharacter::ApplyDamageDirection(AActor* DamageCauser)
{
    if (!DamageCauser) return;

    const FVector ToSource = DamageCauser->GetActorLocation() - GetActorLocation();
    if (ToSource.IsNearlyZero()) return;

    const float SourceYaw = ToSource.Rotation().Yaw;
    const float ViewYaw = Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw;
    DamageDirectionDegrees = FMath::FindDeltaAngleDegrees(ViewYaw, SourceYaw);
    DamageDirectionAlpha = 1.f;
}

void AURFPSCharacter::RegisterConfirmedHit(bool bHeadshot)
{
    ++ConfirmedHits;
    HitMarkerAlpha = 1.f;
    if (bHeadshot)
    {
        ++HeadshotCount;
        HeadshotMarkerAlpha = 1.f;
    }
}

void AURFPSCharacter::ApplySuppression(float Intensity, const FVector& SourceLocation)
{
    if (IsDead()) return;

    SuppressionFeedback = FMath::Clamp(SuppressionFeedback + FMath::Clamp(Intensity, 0.f, 1.f), 0.f, 1.f);
    if (!SourceLocation.IsNearlyZero())
    {
        const FVector ToSource = SourceLocation - GetActorLocation();
        if (!ToSource.IsNearlyZero())
        {
            const float SourceYaw = ToSource.Rotation().Yaw;
            const float ViewYaw = Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw;
            DamageDirectionDegrees = FMath::FindDeltaAngleDegrees(ViewYaw, SourceYaw);
            DamageDirectionAlpha = FMath::Max(DamageDirectionAlpha, 0.38f);
        }
    }
}

void AURFPSCharacter::SpawnShellCasing()
{
    if (!GetWorld() || !FirstPersonCamera) return;

    const FVector Right = FirstPersonCamera->GetRightVector();
    const FVector Up = FirstPersonCamera->GetUpVector();
    const FVector Forward = FirstPersonCamera->GetForwardVector();
    const FVector SpawnLocation = WeaponReceiver
        ? WeaponReceiver->GetComponentLocation() + Right * 7.f + Up * 2.f
        : FirstPersonCamera->GetComponentLocation() + Forward * 30.f + Right * 12.f;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AURFPSShellCasing* Casing = GetWorld()->SpawnActor<AURFPSShellCasing>(
        AURFPSShellCasing::StaticClass(), SpawnLocation, FRotator(0.f, 90.f, 0.f), SpawnParams))
    {
        const FVector LinearVelocity = Right * FMath::FRandRange(145.f, 215.f)
            + Up * FMath::FRandRange(90.f, 155.f)
            - Forward * FMath::FRandRange(10.f, 45.f)
            + GetVelocity() * 0.15f;
        const FVector AngularVelocity(
            FMath::FRandRange(-900.f, 900.f),
            FMath::FRandRange(-1100.f, 1100.f),
            FMath::FRandRange(-900.f, 900.f));
        Casing->Eject(LinearVelocity, AngularVelocity);
    }
}

void AURFPSCharacter::HandleDeath()
{
    EndFire();
    CancelTreatment();
    GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
    bReloading = false;
    bAiming = false;
    bSprinting = false;
    bWalkingSlow = false;
    bHoldingBreath = false;
    bLowReady = false;
    LeanTarget = 0.f;
    if (Flashlight)
    {
        bFlashlightOn = false;
        Flashlight->SetVisibility(false);
    }
    if (MuzzleFlashLight) MuzzleFlashLight->SetVisibility(false);
    GetCharacterMovement()->DisableMovement();
}

void AURFPSCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    const float ImpactSpeed = FMath::Clamp(FMath::Abs(PreviousVerticalVelocity), 0.f, 1200.f);
    LandingKick = FMath::Clamp(ImpactSpeed / 1200.f * 4.5f, 0.f, 4.5f);
    if (ImpactSpeed > 360.f)
    {
        FootstepDistanceAccumulator = 0.f;
        PlayFootstep(FMath::GetMappedRangeValueClamped(FVector2D(360.f, 1050.f), FVector2D(0.46f, 1.0f), ImpactSpeed));
    }
}

float AURFPSCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (IsDead() || SpawnProtectionRemaining > 0.f) return 0.f;

    float ZoneMultiplier = 1.f;
    float BleedChance = 0.18f;
    bool bTorsoHit = true;

    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent* PointEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
        const float RelativeHeight = PointEvent->HitInfo.ImpactPoint.Z - GetActorLocation().Z;

        if (RelativeHeight > 72.f)
        {
            ZoneMultiplier = 1.35f;
            BleedChance = 0.14f;
            bTorsoHit = false;
        }
        else if (RelativeHeight < 18.f)
        {
            ZoneMultiplier = 0.72f;
            BleedChance = 0.31f;
            bTorsoHit = false;
        }
    }

    const float AdjustedDamage = DamageAmount * ZoneMultiplier;
    const float RawApplied = Super::TakeDamage(AdjustedDamage, DamageEvent, EventInstigator, DamageCauser);

    float ArmorAbsorbed = 0.f;
    if (bTorsoHit && Armor > 0.f && RawApplied > 0.f)
    {
        ArmorAbsorbed = FMath::Min(RawApplied * 0.45f, Armor);
        Armor = FMath::Max(0.f, Armor - ArmorAbsorbed * 1.25f);
    }

    const float HealthDamage = FMath::Max(0.f, RawApplied - ArmorAbsorbed);
    Health = FMath::Clamp(Health - HealthDamage, 0.f, MaxHealth);
    DamageFeedback = FMath::Clamp(DamageFeedback + HealthDamage / 24.f, 0.f, 1.f);
    SuppressionFeedback = FMath::Clamp(SuppressionFeedback + 0.22f, 0.f, 1.f);
    ApplyDamageDirection(DamageCauser);

    if (HealthDamage >= 4.5f && FMath::FRand() < BleedChance)
    {
        const float NewBleed = FMath::Clamp(HealthDamage / 28.f, 0.16f, 0.55f);
        BleedSeverity = FMath::Clamp(BleedSeverity + NewBleed, 0.f, 1.f);
    }

    if (bTreating && HealthDamage > 0.f)
    {
        CancelTreatment();
    }

    if (Health <= 0.f)
    {
        HandleDeath();
    }

    return HealthDamage;
}
