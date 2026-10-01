#include "URFPSEnemy.h"

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
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "URFPSAudio.h"
#include "URFPSCharacter.h"
#include "URFPSDoor.h"
#include "URFPSGameMode.h"
#include "URFPSProjectile.h"

AURFPSEnemy::AURFPSEnemy()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 88.f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetCapsuleComponent());
    BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -5.f));
    BodyMesh->SetRelativeScale3D(FVector(0.42f, 0.30f, 0.82f));
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(GetCapsuleComponent());
    HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 57.f));
    HeadMesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.28f));
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HeadMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    HeadMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    HelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HelmetMesh"));
    HelmetMesh->SetupAttachment(GetCapsuleComponent());
    HelmetMesh->SetRelativeLocation(FVector(0.f, 0.f, 63.f));
    HelmetMesh->SetRelativeScale3D(FVector(0.31f, 0.31f, 0.13f));
    HelmetMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HelmetMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    HelmetMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    VestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VestMesh"));
    VestMesh->SetupAttachment(GetCapsuleComponent());
    VestMesh->SetRelativeLocation(FVector(2.f, 0.f, 4.f));
    VestMesh->SetRelativeScale3D(FVector(0.46f, 0.34f, 0.44f));
    VestMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    VestMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    VestMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackpackMesh"));
    BackpackMesh->SetupAttachment(GetCapsuleComponent());
    BackpackMesh->SetRelativeLocation(FVector(-22.f, 0.f, 8.f));
    BackpackMesh->SetRelativeScale3D(FVector(0.15f, 0.27f, 0.32f));
    BackpackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Legs. The torso box stops 42 cm above the capsule bottom, so without them every enemy
    // floated above the ground and shots at knee height passed through nothing. Each leg hangs
    // from a hip pivot so a simple walk cycle can swing it from the hip instead of its centre.
    auto CreateLeg = [this](const TCHAR* PivotName, const TCHAR* LegName, float SideOffset, USceneComponent*& OutPivot)
    {
        OutPivot = CreateDefaultSubobject<USceneComponent>(FName(PivotName));
        OutPivot->SetupAttachment(GetCapsuleComponent());
        OutPivot->SetRelativeLocation(FVector(0.f, SideOffset, -44.f));

        UStaticMeshComponent* Leg = CreateDefaultSubobject<UStaticMeshComponent>(FName(LegName));
        Leg->SetupAttachment(OutPivot);
        Leg->SetRelativeLocation(FVector(0.f, 0.f, -22.f));
        Leg->SetRelativeScale3D(FVector(0.16f, 0.13f, 0.46f));
        Leg->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Leg->SetCollisionResponseToAllChannels(ECR_Ignore);
        Leg->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        return Leg;
    };
    LeftLegMesh = CreateLeg(TEXT("LeftHipPivot"), TEXT("LeftLegMesh"), -9.f, LeftHipPivot);
    RightLegMesh = CreateLeg(TEXT("RightHipPivot"), TEXT("RightLegMesh"), 9.f, RightHipPivot);

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyWeapon"));
    WeaponMesh->SetupAttachment(GetCapsuleComponent());
    WeaponMesh->SetRelativeLocation(FVector(31.f, 12.f, 14.f));
    WeaponMesh->SetRelativeScale3D(FVector(0.38f, 0.055f, 0.055f));
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    WeaponBarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyWeaponBarrel"));
    WeaponBarrelMesh->SetupAttachment(GetCapsuleComponent());
    WeaponBarrelMesh->SetRelativeLocation(FVector(59.f, 12.f, 14.f));
    // Pitch 90 lays the Z-aligned engine cylinder along the firing axis (yaw kept it upright).
    WeaponBarrelMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
    WeaponBarrelMesh->SetRelativeScale3D(FVector(0.012f, 0.012f, 0.24f));
    WeaponBarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MuzzleFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EnemyMuzzleFlash"));
    MuzzleFlashLight->SetupAttachment(GetCapsuleComponent());
    MuzzleFlashLight->SetRelativeLocation(FVector(72.f, 12.f, 16.f));
    MuzzleFlashLight->SetIntensity(3000.f);
    MuzzleFlashLight->SetAttenuationRadius(235.f);
    MuzzleFlashLight->SetLightColor(FLinearColor(1.f, 0.40f, 0.10f));
    MuzzleFlashLight->SetCastShadows(false);
    MuzzleFlashLight->SetUseInverseSquaredFalloff(true);
    MuzzleFlashLight->SetSourceRadius(1.5f);
    MuzzleFlashLight->SetSoftSourceRadius(3.5f);
    MuzzleFlashLight->SetVisibility(false);

    MuzzleFlashCone = CreateDefaultSubobject<USpotLightComponent>(TEXT("EnemyMuzzleFlashCone"));
    MuzzleFlashCone->SetupAttachment(GetCapsuleComponent());
    MuzzleFlashCone->SetRelativeLocation(FVector(74.f, 12.f, 16.f));
    MuzzleFlashCone->SetRelativeRotation(FRotator::ZeroRotator);
    MuzzleFlashCone->SetIntensity(4400.f);
    MuzzleFlashCone->SetAttenuationRadius(360.f);
    MuzzleFlashCone->SetInnerConeAngle(10.f);
    MuzzleFlashCone->SetOuterConeAngle(30.f);
    MuzzleFlashCone->SetLightColor(FLinearColor(1.f, 0.46f, 0.14f));
    MuzzleFlashCone->SetCastShadows(false);
    MuzzleFlashCone->SetUseInverseSquaredFalloff(true);
    MuzzleFlashCone->SetSourceRadius(1.0f);
    MuzzleFlashCone->SetSoftSourceRadius(2.5f);
    MuzzleFlashCone->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CubeMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CubeMesh.Object);
        HelmetMesh->SetStaticMesh(CubeMesh.Object);
        VestMesh->SetStaticMesh(CubeMesh.Object);
        BackpackMesh->SetStaticMesh(CubeMesh.Object);
        LeftLegMesh->SetStaticMesh(CubeMesh.Object);
        RightLegMesh->SetStaticMesh(CubeMesh.Object);
        WeaponMesh->SetStaticMesh(CubeMesh.Object);
    }
    if (SphereMesh.Succeeded()) HeadMesh->SetStaticMesh(SphereMesh.Object);
    else if (CubeMesh.Succeeded()) HeadMesh->SetStaticMesh(CubeMesh.Object);

    if (CylinderMesh.Succeeded()) WeaponBarrelMesh->SetStaticMesh(CylinderMesh.Object);
    else if (CubeMesh.Succeeded()) WeaponBarrelMesh->SetStaticMesh(CubeMesh.Object);

    UCharacterMovementComponent* Move = GetCharacterMovement();
    Move->MaxWalkSpeed = 265.f;
    Move->MaxAcceleration = 820.f;
    Move->BrakingDecelerationWalking = 900.f;
    Move->GroundFriction = 7.f;
    Move->bRunPhysicsWithNoController = true;
    Move->bOrientRotationToMovement = false;
}

void AURFPSEnemy::BeginPlay()
{
    Super::BeginPlay();
    StrafeDirection = FMath::RandBool() ? 1 : -1;
    StrafeChangeTimer = FMath::FRandRange(0.8f, 2.4f);
    FireCooldown = FMath::FRandRange(1.1f, 2.55f);
    GuardLocation = GetActorLocation();
    ProgressAnchor = GuardLocation;
    PatrolWaitTimer = FMath::FRandRange(3.f, 9.f);

    if (UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        UniformMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        GearMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        WeaponMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        FaceMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);

        if (UniformMaterial)
        {
            UniformMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.10f, 0.12f, 0.11f, 1.f));
            BodyMesh->SetMaterial(0, UniformMaterial);
            LeftLegMesh->SetMaterial(0, UniformMaterial);
            RightLegMesh->SetMaterial(0, UniformMaterial);
        }
        if (FaceMaterial)
        {
            // Balaclava tone: the head previously kept the bright default shape colour.
            FaceMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.032f, 0.032f, 0.030f, 1.f));
            HeadMesh->SetMaterial(0, FaceMaterial);
        }
        if (GearMaterial)
        {
            GearMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.055f, 0.060f, 0.055f, 1.f));
            HelmetMesh->SetMaterial(0, GearMaterial);
            VestMesh->SetMaterial(0, GearMaterial);
            BackpackMesh->SetMaterial(0, GearMaterial);
        }
        if (WeaponMaterial)
        {
            WeaponMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.025f, 0.028f, 0.030f, 1.f));
            WeaponMesh->SetMaterial(0, WeaponMaterial);
            WeaponBarrelMesh->SetMaterial(0, WeaponMaterial);
        }
    }

    ApplyRoleVisuals();
}

void AURFPSEnemy::SetRoleFromSeed(int32 RoleSeed)
{
    const int32 Pattern = FMath::Abs(RoleSeed + ConfiguredWave * 3) % 10;
    if (ConfiguredWave >= 2 && Pattern == 3)
    {
        Role = EURFPSEnemyRole::Marksman;
    }
    else if (Pattern == 1 || Pattern == 7)
    {
        Role = EURFPSEnemyRole::Suppressor;
    }
    else if (Pattern == 4 || Pattern == 8)
    {
        Role = EURFPSEnemyRole::Breacher;
    }
    else
    {
        Role = EURFPSEnemyRole::Rifleman;
    }
}

void AURFPSEnemy::ConfigureForWave(int32 WaveNumber, int32 RoleSeed)
{
    ConfiguredWave = FMath::Max(1, WaveNumber);
    SetRoleFromSeed(RoleSeed);

    const float WaveHealthBonus = FMath::Min(static_cast<float>(ConfiguredWave - 1) * 2.f, 12.f);
    const float WaveDamageBonus = FMath::Min(static_cast<float>(ConfiguredWave - 1) * 0.08f, 0.65f);

    switch (Role)
    {
    case EURFPSEnemyRole::Suppressor:
        MaxHealth = 105.f + WaveHealthBonus;
        DamagePerShot = 5.1f + WaveDamageBonus;
        BaseAccuracyDegrees = 3.20f;
        PreferredDistance = 1450.f;
        ReactionMin = 0.66f;
        ReactionMax = 1.12f;
        MinBurstShots = 4;
        MaxBurstShots = 6;
        BurstIntervalMin = 0.11f;
        BurstIntervalMax = 0.16f;
        BurstPauseMin = 1.05f;
        BurstPauseMax = 1.55f;
        ProjectileSpeed = 69000.f;
        BaseConfiguredWalkSpeed = 245.f;
        GetCharacterMovement()->MaxWalkSpeed = BaseConfiguredWalkSpeed;
        HelmetDurability = 40.f;
        VestDurability = 52.f;
        break;

    case EURFPSEnemyRole::Marksman:
        MaxHealth = 88.f + WaveHealthBonus * 0.7f;
        DamagePerShot = 10.0f + WaveDamageBonus;
        BaseAccuracyDegrees = 1.18f;
        PreferredDistance = 2350.f;
        ReactionMin = 0.82f;
        ReactionMax = 1.28f;
        MinBurstShots = 1;
        MaxBurstShots = 1;
        BurstIntervalMin = 0.1f;
        BurstIntervalMax = 0.1f;
        BurstPauseMin = 1.75f;
        BurstPauseMax = 2.55f;
        ProjectileSpeed = 86000.f;
        BaseConfiguredWalkSpeed = 225.f;
        GetCharacterMovement()->MaxWalkSpeed = BaseConfiguredWalkSpeed;
        HelmetDurability = 22.f;
        VestDurability = 26.f;
        break;

    case EURFPSEnemyRole::Breacher:
        MaxHealth = 108.f + WaveHealthBonus;
        DamagePerShot = 6.8f + WaveDamageBonus;
        BaseAccuracyDegrees = 3.75f;
        PreferredDistance = 720.f;
        ReactionMin = 0.50f;
        ReactionMax = 0.88f;
        MinBurstShots = 2;
        MaxBurstShots = 3;
        BurstIntervalMin = 0.14f;
        BurstIntervalMax = 0.19f;
        BurstPauseMin = 1.10f;
        BurstPauseMax = 1.65f;
        ProjectileSpeed = 65000.f;
        BaseConfiguredWalkSpeed = 305.f;
        GetCharacterMovement()->MaxWalkSpeed = BaseConfiguredWalkSpeed;
        HelmetDurability = 44.f;
        VestDurability = 56.f;
        break;

    default:
        MaxHealth = 95.f + WaveHealthBonus;
        DamagePerShot = 6.3f + WaveDamageBonus;
        BaseAccuracyDegrees = 2.48f;
        PreferredDistance = 1250.f;
        ReactionMin = 0.62f;
        ReactionMax = 1.10f;
        MinBurstShots = 2;
        MaxBurstShots = 3;
        BurstIntervalMin = 0.15f;
        BurstIntervalMax = 0.22f;
        BurstPauseMin = 1.05f;
        BurstPauseMax = 1.70f;
        ProjectileSpeed = 72000.f;
        BaseConfiguredWalkSpeed = 265.f;
        GetCharacterMovement()->MaxWalkSpeed = BaseConfiguredWalkSpeed;
        HelmetDurability = 28.f;
        VestDurability = 38.f;
        break;
    }

    Health = MaxHealth;
    LegInjury = 0.f;
    AimInjury = 0.f;
    BaseAccuracyDegrees = FMath::Max(0.95f, BaseAccuracyDegrees - static_cast<float>(ConfiguredWave - 1) * 0.012f);
    ReactionMin = FMath::Max(0.46f, ReactionMin - static_cast<float>(ConfiguredWave - 1) * 0.008f);
    ReactionMax = FMath::Max(0.80f, ReactionMax - static_cast<float>(ConfiguredWave - 1) * 0.010f);
    ApplyRoleVisuals();
}

void AURFPSEnemy::ApplyRoleVisuals()
{
    if (!VestMesh || !HelmetMesh || !BackpackMesh) return;

    FVector VestScale(0.46f, 0.34f, 0.44f);
    FVector BackpackScale(0.15f, 0.27f, 0.32f);
    FVector WeaponScale(0.38f, 0.055f, 0.055f);
    FLinearColor UniformColor(0.10f, 0.12f, 0.11f, 1.f);
    FLinearColor GearColor(0.055f, 0.060f, 0.055f, 1.f);

    switch (Role)
    {
    case EURFPSEnemyRole::Suppressor:
        VestScale = FVector(0.50f, 0.37f, 0.48f);
        BackpackScale = FVector(0.19f, 0.31f, 0.38f);
        WeaponScale = FVector(0.43f, 0.060f, 0.060f);
        UniformColor = FLinearColor(0.10f, 0.13f, 0.085f, 1.f);
        GearColor = FLinearColor(0.065f, 0.080f, 0.050f, 1.f);
        break;
    case EURFPSEnemyRole::Marksman:
        VestScale = FVector(0.42f, 0.31f, 0.39f);
        BackpackScale = FVector(0.12f, 0.23f, 0.27f);
        WeaponScale = FVector(0.52f, 0.050f, 0.050f);
        UniformColor = FLinearColor(0.12f, 0.10f, 0.085f, 1.f);
        GearColor = FLinearColor(0.055f, 0.045f, 0.040f, 1.f);
        break;
    case EURFPSEnemyRole::Breacher:
        VestScale = FVector(0.51f, 0.38f, 0.50f);
        BackpackScale = FVector(0.11f, 0.23f, 0.25f);
        WeaponScale = FVector(0.31f, 0.070f, 0.070f);
        UniformColor = FLinearColor(0.075f, 0.095f, 0.12f, 1.f);
        GearColor = FLinearColor(0.045f, 0.055f, 0.070f, 1.f);
        break;
    default:
        break;
    }

    VestMesh->SetRelativeScale3D(VestScale);
    BackpackMesh->SetRelativeScale3D(BackpackScale);
    WeaponMesh->SetRelativeScale3D(WeaponScale);
    if (UniformMaterial) UniformMaterial->SetVectorParameterValue(FName(TEXT("Color")), UniformColor);
    if (GearMaterial) GearMaterial->SetVectorParameterValue(FName(TEXT("Color")), GearColor);
}

void AURFPSEnemy::AlertFromNoise(const FVector& SourceLocation, float AudibleRange, float AlertStrength)
{
    if (bDead || AudibleRange <= 0.f) return;

    const float Distance = FVector::Dist2D(GetActorLocation(), SourceLocation);
    if (Distance > AudibleRange) return;

    bAlerted = true;
    LastSeenLocation = SourceLocation;
    TimeSinceSeen = 0.f;

    const float DistanceAlpha = 1.f - FMath::Clamp(Distance / AudibleRange, 0.f, 1.f);
    const float Strength = FMath::Clamp(AlertStrength * (0.55f + DistanceAlpha * 0.65f), 0.15f, 1.25f);
    const float ReactionMinLocal = FMath::Lerp(1.15f, 0.36f, Strength);
    const float ReactionMaxLocal = FMath::Lerp(1.65f, 0.72f, Strength);
    ReactionTimer = FMath::Max(ReactionTimer, FMath::FRandRange(ReactionMinLocal, ReactionMaxLocal));
}

void AURFPSEnemy::AlertFromGunshot(const FVector& SourceLocation)
{
    AlertFromNoise(SourceLocation, 6200.f, 1.10f);
}

void AURFPSEnemy::ApplySuppression(const FVector& SourceLocation)
{
    if (bDead) return;

    AlertFromGunshot(SourceLocation);
    SuppressionTimer = FMath::Max(SuppressionTimer, FMath::FRandRange(0.95f, 1.60f));
    RepositionTimer = FMath::Max(RepositionTimer, FMath::FRandRange(0.80f, 1.45f));
    StrafeDirection = FMath::RandBool() ? 1 : -1;
    StrafeChangeTimer = FMath::FRandRange(0.35f, 0.85f);
}

void AURFPSEnemy::ReactToGrenade(const FVector& GrenadeLocation)
{
    if (bDead) return;
    if (TimeSinceSeen >= SearchLingerTime)
    {
        // An enemy without recent contact never reached the evasion code and stood next to the
        // grenade. It now enters the contact phase: flee first, then check where it landed.
        LastSeenLocation = GrenadeLocation;
        TimeSinceSeen = SearchLingerTime * 0.5f;
    }
    GrenadeThreatLocation = GrenadeLocation;
    GrenadeAvoidTimer = FMath::Max(GrenadeAvoidTimer, FMath::FRandRange(0.85f, 1.35f));
    RepositionTimer = FMath::Max(RepositionTimer, 1.1f);
    SuppressionTimer = FMath::Max(SuppressionTimer, 0.45f);
    bAlerted = true;
    ReleaseFireSlot();
    BurstShotsRemaining = 0;
}

void AURFPSEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDead)
    {
        UpdateDeathFall(DeltaSeconds);
        return;
    }

    const float LegSpeedScale = FMath::Lerp(1.f, 0.62f, FMath::Clamp(LegInjury, 0.f, 1.f));
    const float CriticalHealthScale = Health < MaxHealth * 0.28f ? 0.90f : 1.f;
    GetCharacterMovement()->MaxWalkSpeed = BaseConfiguredWalkSpeed * LegSpeedScale * CriticalHealthScale;

    if (MuzzleFlashTimer > 0.f)
    {
        MuzzleFlashTimer -= DeltaSeconds;
        if (MuzzleFlashTimer <= 0.f)
        {
            if (MuzzleFlashLight) MuzzleFlashLight->SetVisibility(false);
            if (MuzzleFlashCone) MuzzleFlashCone->SetVisibility(false);
        }
    }

    WeaponKick = FMath::FInterpTo(WeaponKick, 0.f, DeltaSeconds, 16.f);
    if (WeaponMesh) WeaponMesh->SetRelativeRotation(FRotator(-5.5f * WeaponKick, 0.f, 0.f));
    UpdateFootsteps(DeltaSeconds);
    UpdateLegSwing(DeltaSeconds);

    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Player) return;

    // A downed player is no longer a target. Previously the squad kept converging on the body
    // and emptying magazines into it until the player pressed R to respawn.
    if (IsPlayerDown(Player))
    {
        DisengageFromDownedPlayer();
        return;
    }

    const float Distance = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
    bool bHasLOS = false;
    if (Distance <= AggroRange)
    {
        bHasLOS = HasLineOfSightToPlayer();
    }

    if (bHasLOS)
    {
        if (!bAlerted)
        {
            bAlerted = true;
            ReactionTimer = FMath::FRandRange(ReactionMin, ReactionMax);
            AlertNearbySquad(Player->GetActorLocation());
        }
        TimeSinceSeen = 0.f;
        LastSeenLocation = Player->GetActorLocation();
    }
    else
    {
        TimeSinceSeen += DeltaSeconds;
        if (bHasFireSlot)
        {
            ReleaseFireSlot();
            BurstShotsRemaining = 0;
        }
    }

    ReactionTimer = FMath::Max(0.f, ReactionTimer - DeltaSeconds);
    SuppressionTimer = FMath::Max(0.f, SuppressionTimer - DeltaSeconds);
    RepositionTimer = FMath::Max(0.f, RepositionTimer - DeltaSeconds);
    CoverSeekTimer = FMath::Max(0.f, CoverSeekTimer - DeltaSeconds);
    GrenadeAvoidTimer = FMath::Max(0.f, GrenadeAvoidTimer - DeltaSeconds);

    DoorCooldown = FMath::Max(0.f, DoorCooldown - DeltaSeconds);

    if (bAlerted && TimeSinceSeen < SearchLingerTime)
    {
        UpdateCombatMovement(Player, DeltaSeconds, bHasLOS);
        TryFire(DeltaSeconds, Player, bHasLOS);
        return;
    }

    // The contact went cold. Enemies used to freeze on the spot from here on, so a player who
    // stayed out of sight stalled the wave: they now search where the player was last seen or
    // heard, then patrol, and hunt once the GameMode pacing says so.
    if (bAlerted)
    {
        bAlerted = false;
        BeginSearch(LastSeenLocation, 3);
    }
    UpdateNavigation(Player, DeltaSeconds);
}

bool AURFPSEnemy::IsPlayerDown(const APawn* Player) const
{
    const AURFPSCharacter* PlayerCharacter = Cast<AURFPSCharacter>(Player);
    return PlayerCharacter && PlayerCharacter->IsDead();
}

void AURFPSEnemy::DisengageFromDownedPlayer()
{
    ReleaseFireSlot();
    BurstShotsRemaining = 0;
    ReactionTimer = 0.f;
    // Forget the contact: after the respawn the squad must see or hear the player again and
    // goes through its normal reaction delay instead of firing instantly at the spawn point.
    bAlerted = false;
    TimeSinceSeen = SearchLingerTime;

    // Hunters fall back to a patrol and give the respawned player a moment before closing in.
    if (Intent == EURFPSEnemyIntent::Hunt || Intent == EURFPSEnemyIntent::Search)
    {
        GuardLocation = GetActorLocation();
        SetIntent(EURFPSEnemyIntent::Patrol);
    }
    if (HuntStartTime >= 0.f)
    {
        HuntStartTime = FMath::Max(HuntStartTime, GetWorldTimeSeconds() + 12.f);
    }
    HuntRefreshCount = 0;
}

void AURFPSEnemy::UpdateLegSwing(float DeltaSeconds)
{
    if (!LeftHipPivot || !RightHipPivot) return;

    // Stride cycle driven by the distance actually covered (one full cycle per two steps).
    const float PlanarSpeed = GetVelocity().Size2D();
    const bool bGrounded = GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround();
    const float TargetAmplitude = bGrounded ? FMath::Clamp(PlanarSpeed / 300.f, 0.f, 1.f) * 24.f : 0.f;
    LegSwingAmplitude = FMath::FInterpTo(LegSwingAmplitude, TargetAmplitude, DeltaSeconds, 8.f);
    LegSwingPhase = FMath::Fmod(LegSwingPhase + DeltaSeconds * PI * PlanarSpeed / 170.f, 2.f * PI);

    const float Swing = FMath::Sin(LegSwingPhase) * LegSwingAmplitude;
    LeftHipPivot->SetRelativeRotation(FRotator(Swing, 0.f, 0.f));
    RightHipPivot->SetRelativeRotation(FRotator(-Swing, 0.f, 0.f));
}

void AURFPSEnemy::BeginDeathFall()
{
    // The corpse used to be rolled 82 degrees around the capsule centre and stayed ~65 cm above
    // the ground. It now collapses onto the floor the capsule was standing on.
    DeathStartLocation = GetActorLocation();
    DeathStartRotation = GetActorRotation();
    DeathRestRoll = FMath::RandBool() ? 82.f : -82.f;
    DeathFallElapsed = 0.f;

    const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
    // Lying on its side the widest gear (vest) extends ~20 cm from the actor origin.
    DeathRestLocation = DeathStartLocation - FVector(0.f, 0.f, HalfHeight - 21.f);

    if (LeftHipPivot) LeftHipPivot->SetRelativeRotation(FRotator(-8.f, 0.f, 0.f));
    if (RightHipPivot) RightHipPivot->SetRelativeRotation(FRotator(12.f, 0.f, 0.f));
}

void AURFPSEnemy::UpdateDeathFall(float DeltaSeconds)
{
    if (DeathFallElapsed >= DeathFallDuration) return;

    DeathFallElapsed = FMath::Min(DeathFallDuration, DeathFallElapsed + DeltaSeconds);
    const float Alpha = FMath::Clamp(DeathFallElapsed / FMath::Max(DeathFallDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
    const float Eased = Alpha * Alpha; // accelerates like a body giving way under gravity

    FRotator Rotation = DeathStartRotation;
    Rotation.Roll = DeathStartRotation.Roll + (DeathRestRoll - DeathStartRotation.Roll) * Eased;
    const FVector Location = DeathStartLocation + (DeathRestLocation - DeathStartLocation) * Eased;
    SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
}

bool AURFPSEnemy::HasLineOfSightToPlayer() const
{
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Player) return false;

    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 60.f);
    const FVector End = Player->GetActorLocation() + FVector(0.f, 0.f, 52.f);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemySight), true, this);
    Params.AddIgnoredActor(this);

    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) return true;
    return Hit.GetActor() == Player;
}

FVector AURFPSEnemy::GetAvoidanceDirection(const FVector& DesiredDirection) const
{
    if (DesiredDirection.IsNearlyZero()) return DesiredDirection;

    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 35.f);
    const FVector End = Start + DesiredDirection.GetSafeNormal2D() * 165.f;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyAvoidance), false, this);
    Params.AddIgnoredActor(this);

    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        const FVector Side = FVector::CrossProduct(FVector::UpVector, DesiredDirection.GetSafeNormal2D()) * static_cast<float>(StrafeDirection);
        return (DesiredDirection.GetSafeNormal2D() * 0.2f + Side * 0.9f).GetSafeNormal2D();
    }

    return DesiredDirection.GetSafeNormal2D();
}

FVector AURFPSEnemy::FindCoverBiasedDirection(APawn* Player) const
{
    if (!Player || !GetWorld()) return FVector::ZeroVector;

    FVector Away = GetActorLocation() - Player->GetActorLocation();
    Away.Z = 0.f;
    Away = Away.GetSafeNormal2D();
    if (Away.IsNearlyZero()) Away = -GetActorForwardVector().GetSafeNormal2D();

    const FVector Side = FVector::CrossProduct(FVector::UpVector, Away).GetSafeNormal2D();
    const FVector Directions[5] =
    {
        Away,
        (Away + Side * 0.9f).GetSafeNormal2D(),
        (Away - Side * 0.9f).GetSafeNormal2D(),
        Side,
        -Side
    };

    const FVector PlayerEye = Player->GetActorLocation() + FVector(0.f, 0.f, 52.f);
    float BestScore = -BIG_NUMBER;
    FVector BestDirection = Away;

    for (const FVector& Direction : Directions)
    {
        const FVector Candidate = GetActorLocation() + Direction * 360.f;
        const FVector CandidateEye = Candidate + FVector(0.f, 0.f, 54.f);
        FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyCoverProbe), true, this);
        Params.AddIgnoredActor(this);
        Params.AddIgnoredActor(Player);

        FHitResult CoverHit;
        const bool bBlockedFromPlayer = GetWorld()->LineTraceSingleByChannel(CoverHit, PlayerEye, CandidateEye, ECC_Visibility, Params);

        FHitResult PathHit;
        const FVector PathStart = GetActorLocation() + FVector(0.f, 0.f, 35.f);
        const bool bPathBlocked = GetWorld()->LineTraceSingleByChannel(PathHit, PathStart, CandidateEye, ECC_Visibility, Params);

        float Score = bBlockedFromPlayer ? 2.2f : 0.f;
        Score += FVector::DotProduct(Direction, Away) * 0.45f;
        if (bPathBlocked) Score -= 1.2f;

        if (Score > BestScore)
        {
            BestScore = Score;
            BestDirection = Direction;
        }
    }

    return BestDirection.GetSafeNormal2D();
}

bool AURFPSEnemy::AcquireFireSlot()
{
    if (bHasFireSlot) return true;
    if (!GetWorld()) return false;

    if (AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        bHasFireSlot = GameMode->TryAcquireFireSlot(this);
        return bHasFireSlot;
    }

    bHasFireSlot = true;
    return true;
}

void AURFPSEnemy::ReleaseFireSlot()
{
    if (!bHasFireSlot) return;
    bHasFireSlot = false;

    if (GetWorld())
    {
        if (AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->ReleaseFireSlot(this);
        }
    }
}


void AURFPSEnemy::UpdateCombatMovement(APawn* Player, float DeltaSeconds, bool bHasLOS)
{
    // Out of sight: close in on the last known position along a planned route. The straight
    // line used before walked enemies into walls (one followed the security office walls for
    // minutes without finding the door).
    if (!bHasLOS && GrenadeAvoidTimer <= 0.f)
    {
        if (MoveTowards(LastSeenLocation, 0.85f, 110.f, DeltaSeconds))
        {
            // Nobody there: the contact is over, the search starts on the next tick.
            TimeSinceSeen = SearchLingerTime;
        }
        return;
    }

    const FVector TargetLocation = bHasLOS ? Player->GetActorLocation() : LastSeenLocation;
    FVector ToTarget = TargetLocation - GetActorLocation();
    ToTarget.Z = 0.f;

    const float Distance = ToTarget.Size();
    const FVector ForwardToTarget = ToTarget.GetSafeNormal();

    if (!ForwardToTarget.IsNearlyZero())
    {
        const FRotator TargetRotation = ForwardToTarget.Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, bHasLOS ? 7.f : 4.f));
    }

    StrafeChangeTimer -= DeltaSeconds;
    if (StrafeChangeTimer <= 0.f)
    {
        StrafeDirection = FMath::RandBool() ? 1 : -1;
        StrafeChangeTimer = FMath::FRandRange(0.9f, 2.5f);
    }

    FVector DesiredMove = FVector::ZeroVector;

    if (GrenadeAvoidTimer > 0.f)
    {
        FVector AwayFromGrenade = GetActorLocation() - GrenadeThreatLocation;
        AwayFromGrenade.Z = 0.f;
        DesiredMove = AwayFromGrenade.GetSafeNormal2D();
        DesiredMove += FVector::CrossProduct(FVector::UpVector, DesiredMove) * static_cast<float>(StrafeDirection) * 0.45f;
    }
    else if (!bHasLOS)
    {
        if (Distance > 90.f) DesiredMove = ForwardToTarget;
    }
    else
    {
        float AdvanceStrength = 0.72f;
        float RetreatStrength = 0.62f;
        float StrafeStrength = 0.38f;

        if (Role == EURFPSEnemyRole::Breacher)
        {
            AdvanceStrength = 0.95f;
            RetreatStrength = 0.35f;
            StrafeStrength = 0.55f;
        }
        else if (Role == EURFPSEnemyRole::Marksman)
        {
            AdvanceStrength = 0.42f;
            RetreatStrength = 0.92f;
            StrafeStrength = 0.28f;
        }
        else if (Role == EURFPSEnemyRole::Suppressor)
        {
            StrafeStrength = 0.50f;
        }

        const bool bNeedsCover = SuppressionTimer > 0.45f || Health < MaxHealth * 0.42f || LegInjury > 0.45f;
        if (bNeedsCover && CoverSeekTimer <= 0.f)
        {
            DesiredMove += FindCoverBiasedDirection(Player) * 1.05f;
            CoverSeekTimer = FMath::FRandRange(0.45f, 0.85f);
        }
        else
        {
            if (Distance > PreferredDistance + 220.f)
            {
                DesiredMove += ForwardToTarget * AdvanceStrength;
            }
            else if (Distance < PreferredDistance - 260.f)
            {
                DesiredMove -= ForwardToTarget * RetreatStrength;
            }

            const FVector Strafe = FVector::CrossProduct(FVector::UpVector, ForwardToTarget) * static_cast<float>(StrafeDirection);
            const float TacticalStrafe = (SuppressionTimer > 0.f || RepositionTimer > 0.f) ? 0.88f : StrafeStrength;
            DesiredMove += Strafe * TacticalStrafe;
        }

        if (Health < MaxHealth * 0.35f || LegInjury > 0.62f)
        {
            DesiredMove -= ForwardToTarget * (LegInjury > 0.62f ? 0.40f : 0.28f);
        }
    }

    DesiredMove = GetAvoidanceDirection(DesiredMove);
    if (!DesiredMove.IsNearlyZero())
    {
        const float MovementScale = (GrenadeAvoidTimer > 0.f || SuppressionTimer > 0.f || RepositionTimer > 0.f) ? 1.0f : 0.72f;
        AddMovementInput(DesiredMove, MovementScale);
    }
}

void AURFPSEnemy::TryFire(float DeltaSeconds, APawn* Player, bool bHasLOS)
{
    FireCooldown -= DeltaSeconds;

    if (!bHasLOS || ReactionTimer > 0.f || GrenadeAvoidTimer > 0.f)
    {
        if (bHasFireSlot)
        {
            ReleaseFireSlot();
            BurstShotsRemaining = 0;
        }
        return;
    }

    if (FireCooldown > 0.f) return;

    if (BurstShotsRemaining <= 0)
    {
        if (!AcquireFireSlot())
        {
            FireCooldown = FMath::FRandRange(0.22f, 0.50f);
            RepositionTimer = FMath::Max(RepositionTimer, FMath::FRandRange(0.35f, 0.75f));
            return;
        }
        BurstShotsRemaining = FMath::RandRange(MinBurstShots, MaxBurstShots);
    }

    FireOneShot(Player);
    --BurstShotsRemaining;

    if (BurstShotsRemaining > 0)
    {
        FireCooldown = FMath::FRandRange(BurstIntervalMin, BurstIntervalMax);
    }
    else
    {
        ReleaseFireSlot();
        const float SuppressedDelay = SuppressionTimer > 0.f ? 0.42f : 0.f;
        const float InjuryDelay = (AimInjury + LegInjury) * 0.22f;
        FireCooldown = FMath::FRandRange(BurstPauseMin, BurstPauseMax) + SuppressedDelay + InjuryDelay;
        RepositionTimer = FMath::Max(RepositionTimer, FMath::FRandRange(0.55f, 1.25f));
        if (FMath::FRand() < 0.55f) StrafeDirection *= -1;
    }
}

void AURFPSEnemy::FireOneShot(APawn* Player)
{
    if (!Player || !GetWorld()) return;

    const FVector Start = WeaponBarrelMesh
        ? WeaponBarrelMesh->GetComponentLocation() + GetActorForwardVector() * 12.f
        : GetActorLocation() + FVector(0.f, 0.f, 58.f);
    const float Distance = FVector::Dist(Start, Player->GetActorLocation());
    const float DistancePenalty = FMath::Clamp(Distance / 3200.f, 0.f, 1.65f);
    const float PlayerMovementPenalty = FMath::Clamp(Player->GetVelocity().Size2D() / 600.f, 0.f, 1.f) * 0.58f;
    const float EnemyMovementPenalty = FMath::Clamp(GetVelocity().Size2D() / 310.f, 0.f, 1.f) * 0.78f;
    const float SuppressionPenalty = SuppressionTimer > 0.f ? 1.20f : 0.f;
    const float InjuryPenalty = AimInjury * 1.35f + LegInjury * 0.35f;
    const float AccuracyDegrees = BaseAccuracyDegrees + DistancePenalty + PlayerMovementPenalty + EnemyMovementPenalty + SuppressionPenalty + InjuryPenalty;

    const float TravelTime = Distance / FMath::Max(ProjectileSpeed, 1.f);
    const FVector Lead = Player->GetVelocity() * TravelTime * 0.24f;
    FVector AimDirection = (Player->GetActorLocation() + FVector(0.f, 0.f, 48.f) + Lead - Start).GetSafeNormal();
    AimDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(AccuracyDegrees));

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ++EnemyShotCounter;
    const bool bTracerRound = Role == EURFPSEnemyRole::Marksman || (EnemyShotCounter % 3) == 0;
    if (AURFPSProjectile* Projectile = GetWorld()->SpawnActor<AURFPSProjectile>(
        AURFPSProjectile::StaticClass(), Start, AimDirection.Rotation(), SpawnParams))
    {
        Projectile->Initialize(AimDirection * ProjectileSpeed, DamagePerShot, nullptr, this, bTracerRound);
    }

    const float RoleVolume = Role == EURFPSEnemyRole::Marksman ? 1.05f : (Role == EURFPSEnemyRole::Breacher ? 0.92f : 0.86f);
    URFPSAudio::PlayGunshot(this, Start, true, RoleVolume, FMath::FRandRange(0.94f, 1.05f));

    WeaponKick = 1.f;
    if (MuzzleFlashLight || MuzzleFlashCone)
    {
        const FLinearColor FlashColor(
            1.f,
            FMath::FRandRange(0.36f, 0.49f),
            FMath::FRandRange(0.08f, 0.16f),
            1.f);

        if (MuzzleFlashLight)
        {
            MuzzleFlashLight->SetLightColor(FlashColor);
            MuzzleFlashLight->SetIntensity(FMath::FRandRange(2500.f, 4300.f));
            MuzzleFlashLight->SetAttenuationRadius(FMath::FRandRange(190.f, 265.f));
            MuzzleFlashLight->SetVisibility(true);
        }
        if (MuzzleFlashCone)
        {
            MuzzleFlashCone->SetLightColor(FlashColor);
            MuzzleFlashCone->SetIntensity(FMath::FRandRange(3600.f, 5900.f));
            MuzzleFlashCone->SetAttenuationRadius(FMath::FRandRange(300.f, 410.f));
            MuzzleFlashCone->SetOuterConeAngle(FMath::FRandRange(25.f, 33.f));
            MuzzleFlashCone->SetVisibility(true);
        }
        MuzzleFlashTimer = FMath::FRandRange(0.020f, 0.034f);
    }
}

float AURFPSEnemy::GetWorldTimeSeconds() const
{
    return GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.f;
}

void AURFPSEnemy::ScheduleHunt(float DelaySeconds)
{
    HuntStartTime = GetWorldTimeSeconds() + FMath::Max(0.f, DelaySeconds);
}

void AURFPSEnemy::BeginHunt()
{
    if (bDead) return;

    const float Now = GetWorldTimeSeconds();
    if (HuntStartTime < 0.f || HuntStartTime > Now)
    {
        HuntStartTime = Now;
    }
    if (Intent != EURFPSEnemyIntent::Hunt)
    {
        SetIntent(EURFPSEnemyIntent::Hunt);
        HuntRefreshTimer = 0.f;
    }
}

void AURFPSEnemy::SetIntent(EURFPSEnemyIntent NewIntent)
{
    Intent = NewIntent;
    bHasMoveTarget = false;
    LookAroundTimer = 0.f;
    ClearPath();
}

void AURFPSEnemy::ClearPath()
{
    bHasPath = false;
    PathPoints.Reset();
    PathIndex = 0;
    StuckCount = 0;
    ProgressTimer = 0.f;
    ProgressAnchor = GetActorLocation();
}

void AURFPSEnemy::BeginSearch(const FVector& Center, int32 Points)
{
    SetIntent(EURFPSEnemyIntent::Search);
    SearchCenter = Center;
    SearchPointsLeft = FMath::Max(1, Points);
}

void AURFPSEnemy::PickHuntTarget(const APawn* Player)
{
    HuntRefreshTimer = FMath::FRandRange(6.5f, 9.5f);
    if (!Player) return;

    // The squad knows the player's sector, not the exact spot: the estimate tightens with every
    // refresh, so a hidden player is found within a minute or two rather than instantly.
    const float Uncertainty = FMath::Max(450.f, 1500.f - 220.f * static_cast<float>(HuntRefreshCount));
    ++HuntRefreshCount;
    const float Angle = FMath::FRand() * 2.f * PI;
    const float Distance = Uncertainty * FMath::Sqrt(FMath::FRand());
    const FVector Estimate = Player->GetActorLocation() + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.f);

    FVector Target = Estimate;
    if (AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        FVector Reachable;
        if (GameMode->FindNavPointNear(GetActorLocation(), Estimate, 400.f, Reachable))
        {
            Target = Reachable;
        }
    }

    // Keep the current route unless the estimate moved noticeably.
    if (!bHasMoveTarget || FVector::Dist2D(Target, MoveTarget) > 500.f)
    {
        MoveTarget = Target;
        bHasMoveTarget = true;
    }
}

bool AURFPSEnemy::PickPatrolPoint()
{
    PatrolWaitTimer = FMath::FRandRange(6.f, 12.f);

    AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GameMode || !GameMode->IsNavigationReady()) return false;

    FVector Point;
    if (!GameMode->FindNavPointNear(GetActorLocation(), GuardLocation, 700.f, Point)) return false;

    if (Intent != EURFPSEnemyIntent::Patrol)
    {
        SetIntent(EURFPSEnemyIntent::Patrol);
    }
    MoveTarget = Point;
    bHasMoveTarget = true;
    return true;
}

void AURFPSEnemy::UpdateLookAround(float DeltaSeconds)
{
    LookAroundTimer -= DeltaSeconds;
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, LookAroundYaw, 0.f), DeltaSeconds, 2.6f));
}

void AURFPSEnemy::UpdateNavigation(APawn* Player, float DeltaSeconds)
{
    const float Now = GetWorldTimeSeconds();

    // Wave pacing: guards and patrols move out once their hunt delay has passed.
    if (HuntStartTime >= 0.f && Now >= HuntStartTime &&
        (Intent == EURFPSEnemyIntent::Guard || Intent == EURFPSEnemyIntent::Patrol))
    {
        BeginHunt();
    }

    switch (Intent)
    {
    case EURFPSEnemyIntent::Search:
        if (LookAroundTimer > 0.f)
        {
            UpdateLookAround(DeltaSeconds);
            break;
        }
        if (!bHasMoveTarget)
        {
            if (SearchPointsLeft <= 0)
            {
                if (HuntStartTime >= 0.f && Now >= HuntStartTime)
                {
                    SetIntent(EURFPSEnemyIntent::Hunt);
                    HuntRefreshTimer = 0.f;
                }
                else
                {
                    GuardLocation = GetActorLocation();
                    SetIntent(EURFPSEnemyIntent::Patrol);
                    PatrolWaitTimer = FMath::FRandRange(2.f, 5.f);
                }
                break;
            }
            --SearchPointsLeft;
            AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(UGameplayStatics::GetGameMode(this));
            bHasMoveTarget = GameMode && GameMode->FindNavPointNear(GetActorLocation(), SearchCenter, 650.f, MoveTarget);
            if (!bHasMoveTarget)
            {
                SearchPointsLeft = 0;
                break;
            }
        }
        if (MoveTowards(MoveTarget, 0.62f, 90.f, DeltaSeconds))
        {
            bHasMoveTarget = false;
            LookAroundTimer = FMath::FRandRange(1.0f, 2.0f);
            LookAroundYaw = GetActorRotation().Yaw + FMath::FRandRange(70.f, 140.f) * (FMath::RandBool() ? 1.f : -1.f);
        }
        break;

    case EURFPSEnemyIntent::Hunt:
        HuntRefreshTimer -= DeltaSeconds;
        if (HuntRefreshTimer <= 0.f || !bHasMoveTarget)
        {
            PickHuntTarget(Player);
        }
        if (bHasMoveTarget && MoveTowards(MoveTarget, Role == EURFPSEnemyRole::Breacher ? 0.92f : 0.78f, 150.f, DeltaSeconds))
        {
            // Sweep the estimated area, then the hunt resumes from a fresher estimate.
            BeginSearch(MoveTarget, 2);
        }
        break;

    case EURFPSEnemyIntent::Patrol:
    case EURFPSEnemyIntent::Guard:
    default:
        if (bHasMoveTarget)
        {
            if (MoveTowards(MoveTarget, 0.42f, 80.f, DeltaSeconds))
            {
                bHasMoveTarget = false;
                PatrolWaitTimer = FMath::FRandRange(5.f, 11.f);
                LookAroundTimer = FMath::FRandRange(1.2f, 2.2f);
                LookAroundYaw = GetActorRotation().Yaw + FMath::FRandRange(60.f, 150.f) * (FMath::RandBool() ? 1.f : -1.f);
            }
        }
        else
        {
            if (LookAroundTimer > 0.f)
            {
                UpdateLookAround(DeltaSeconds);
            }
            PatrolWaitTimer -= DeltaSeconds;
            if (PatrolWaitTimer <= 0.f)
            {
                PickPatrolPoint();
            }
        }
        break;
    }
}

bool AURFPSEnemy::MoveTowards(const FVector& Goal, float SpeedScale, float AcceptRadius, float DeltaSeconds)
{
    const FVector Location = GetActorLocation();
    if (FVector::Dist2D(Location, Goal) <= AcceptRadius)
    {
        ClearPath();
        return true;
    }

    AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(UGameplayStatics::GetGameMode(this));
    const bool bNavigation = GameMode && GameMode->IsNavigationReady();

    RepathTimer -= DeltaSeconds;
    if (bNavigation && (!bHasPath || FVector::Dist2D(PathGoal, Goal) > 150.f || RepathTimer <= 0.f))
    {
        bHasPath = GameMode->FindNavPath(Location, Goal, PathPoints);
        PathIndex = 0;
        PathGoal = Goal;
        RepathTimer = FMath::FRandRange(3.5f, 5.5f);
        if (!bHasPath)
        {
            // Cut off from the goal: report it as done so the behaviour picks another one.
            ClearPath();
            return true;
        }
    }

    FVector Waypoint = Goal;
    if (bNavigation && bHasPath && PathPoints.Num() > 0)
    {
        const int32 LastIndex = PathPoints.Num() - 1;
        while (PathIndex < LastIndex && FVector::Dist2D(Location, PathPoints[PathIndex]) < 60.f)
        {
            ++PathIndex;
        }
        // Cut the next corner when the straight line is free (the route was planned from an
        // older position).
        if (PathIndex < LastIndex && GameMode->HasNavLine(Location, PathPoints[PathIndex + 1]))
        {
            ++PathIndex;
        }
        Waypoint = PathPoints[PathIndex];
        if (PathIndex == LastIndex && FVector::Dist2D(Location, Waypoint) <= FMath::Max(AcceptRadius, 60.f))
        {
            // End of the reachable route (the goal itself may stand on a platform or deck).
            ClearPath();
            return true;
        }
    }

    FVector Direction = Waypoint - Location;
    Direction.Z = 0.f;
    Direction = Direction.GetSafeNormal();
    if (!bNavigation)
    {
        Direction = GetAvoidanceDirection(Direction);
    }

    TryOpenDoorAhead(Direction);
    if (DoorPauseTimer > 0.f)
    {
        DoorPauseTimer -= DeltaSeconds;
        return false;
    }

    // Stuck on another enemy, a door leaf or an art mesh wider than its bounds: sidestep and
    // replan; give the goal up after a few failed attempts.
    ProgressTimer += DeltaSeconds;
    if (ProgressTimer >= 1.6f)
    {
        if (FVector::Dist2D(Location, ProgressAnchor) < 35.f)
        {
            ++StuckCount;
            bHasPath = false;
            SidestepDirection = FVector::CrossProduct(FVector::UpVector, Direction) * (FMath::RandBool() ? 1.f : -1.f);
            SidestepTimer = 0.55f;
        }
        else
        {
            StuckCount = 0;
        }
        ProgressTimer = 0.f;
        ProgressAnchor = Location;
        if (StuckCount >= 4)
        {
            ClearPath();
            return true;
        }
    }

    FVector Move = Direction + GetSquadSeparation() * 0.6f;
    if (SidestepTimer > 0.f)
    {
        SidestepTimer -= DeltaSeconds;
        Move = Move * 0.35f + SidestepDirection;
    }
    Move.Z = 0.f;
    Move = Move.GetSafeNormal();

    if (!Direction.IsNearlyZero())
    {
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), DeltaSeconds, 5.f));
    }
    if (!Move.IsNearlyZero())
    {
        AddMovementInput(Move, SpeedScale);
    }
    return false;
}

void AURFPSEnemy::TryOpenDoorAhead(const FVector& Direction)
{
    if (DoorCooldown > 0.f || Direction.IsNearlyZero() || !GetWorld()) return;

    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 30.f);
    const FVector End = Start + Direction * 150.f;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyDoorProbe), false, this);
    Params.AddIgnoredActor(this);

    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) return;

    if (AURFPSDoor* Door = Cast<AURFPSDoor>(Hit.GetActor()))
    {
        if (!Door->IsOpen())
        {
            // Doors are on the routes now: the enemy opens them (audible to the player) instead
            // of pushing against the leaf. The leaf swings away from it.
            Door->Interact(this);
            DoorPauseTimer = 0.45f;
        }
        DoorCooldown = 1.2f;
    }
}

FVector AURFPSEnemy::GetSquadSeparation() const
{
    FVector Push = FVector::ZeroVector;
    if (!GetWorld()) return Push;

    for (TActorIterator<AURFPSEnemy> It(GetWorld()); It; ++It)
    {
        const AURFPSEnemy* Other = *It;
        if (Other == this || !IsValid(Other) || Other->bDead) continue;

        FVector Away = GetActorLocation() - Other->GetActorLocation();
        Away.Z = 0.f;
        const float Distance = static_cast<float>(Away.Size());
        if (Distance > 1.f && Distance < 130.f)
        {
            Push += Away / Distance * (1.f - Distance / 130.f);
        }
    }
    return Push;
}

void AURFPSEnemy::UpdateFootsteps(float DeltaSeconds)
{
    if (bDead || !GetWorld() || !GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
    {
        FootstepDistanceAccumulator = 0.f;
        return;
    }

    const float Speed = GetVelocity().Size2D();
    if (Speed < 70.f)
    {
        FootstepDistanceAccumulator = FMath::Max(0.f, FootstepDistanceAccumulator - DeltaSeconds * 85.f);
        return;
    }

    FootstepDistanceAccumulator += Speed * DeltaSeconds;
    const float StepDistance = FMath::Lerp(188.f, 158.f, FMath::Clamp(Speed / 310.f, 0.f, 1.f));
    if (FootstepDistanceAccumulator < StepDistance) return;
    FootstepDistanceAccumulator = FMath::Fmod(FootstepDistanceAccumulator, StepDistance);

    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 24.f);
    const FVector End = GetActorLocation() - FVector(0.f, 0.f, 135.f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyFootstepSurface), false, this);
    Params.AddIgnoredActor(this);
    Params.bReturnPhysicalMaterial = true;

    FHitResult Hit;
    EURFPSAudioEvent Event = EURFPSAudioEvent::FootstepConcrete;
    FVector SoundLocation = GetActorLocation() - FVector(0.f, 0.f, 75.f);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        SoundLocation = Hit.ImpactPoint + Hit.ImpactNormal * 2.f;
        const EPhysicalSurface Surface = UGameplayStatics::GetSurfaceType(Hit);
        if (Surface == SurfaceType2) Event = EURFPSAudioEvent::FootstepMetal;
        else if (Surface == SurfaceType3) Event = EURFPSAudioEvent::FootstepWood;
    }

    float Volume = 0.36f;
    if (Role == EURFPSEnemyRole::Breacher) Volume = 0.48f;
    else if (Role == EURFPSEnemyRole::Marksman) Volume = 0.31f;
    URFPSAudio::PlaySpatial(this, Event, SoundLocation, Volume, FMath::FRandRange(0.91f, 1.08f));
}

void AURFPSEnemy::AlertNearbySquad(const FVector& AlertLocation)
{
    if (!GetWorld()) return;

    for (TActorIterator<AURFPSEnemy> It(GetWorld()); It; ++It)
    {
        AURFPSEnemy* Other = *It;
        if (!IsValid(Other) || Other == this || Other->bDead) continue;

        if (FVector::Dist2D(GetActorLocation(), Other->GetActorLocation()) <= 1750.f)
        {
            Other->AlertFromGunshot(AlertLocation);
        }
    }
}

float AURFPSEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead) return 0.f;

    bool bHeadHit = false;
    bool bLegHit = false;
    bool bTorsoHit = true;
    float ZoneMultiplier = 1.f;

    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent* PointEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
        if (PointEvent->HitInfo.Component.IsValid())
        {
            bHeadHit = PointEvent->HitInfo.Component->GetFName() == FName(TEXT("HeadMesh"));
        }

        const float RelativeHeight = PointEvent->HitInfo.ImpactPoint.Z - GetActorLocation().Z;
        if (bHeadHit || RelativeHeight > 48.f)
        {
            bHeadHit = true;
            bTorsoHit = false;
            ZoneMultiplier = 1.85f;
        }
        else if (RelativeHeight < -18.f)
        {
            bLegHit = true;
            bTorsoHit = false;
            ZoneMultiplier = 0.72f;
        }
    }

    const float AdjustedDamage = DamageAmount * ZoneMultiplier;
    const float RawApplied = Super::TakeDamage(AdjustedDamage, DamageEvent, EventInstigator, DamageCauser);

    float ArmorAbsorbed = 0.f;
    if (bHeadHit && HelmetDurability > 0.f && RawApplied > 0.f)
    {
        ArmorAbsorbed = FMath::Min(RawApplied * 0.36f, HelmetDurability);
        HelmetDurability = FMath::Max(0.f, HelmetDurability - ArmorAbsorbed * 1.45f);
        if (HelmetDurability <= 0.1f && HelmetMesh)
        {
            // A destroyed helmet must stop behaving like invisible ballistic geometry.
            HelmetMesh->SetVisibility(false);
            HelmetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
    else if (bTorsoHit && VestDurability > 0.f && RawApplied > 0.f)
    {
        ArmorAbsorbed = FMath::Min(RawApplied * 0.34f, VestDurability);
        VestDurability = FMath::Max(0.f, VestDurability - ArmorAbsorbed * 1.25f);
    }

    const float HealthDamage = FMath::Max(0.f, RawApplied - ArmorAbsorbed);
    Health = FMath::Clamp(Health - HealthDamage, 0.f, MaxHealth);

    if (bLegHit && HealthDamage > 0.f)
    {
        LegInjury = FMath::Clamp(LegInjury + HealthDamage / 48.f, 0.f, 1.f);
    }
    else if (!bHeadHit && HealthDamage > 0.f)
    {
        AimInjury = FMath::Clamp(AimInjury + HealthDamage / 105.f, 0.f, 0.85f);
    }

    if (RawApplied > 0.f)
    {
        ReleaseFireSlot();
        BurstShotsRemaining = 0;
        bAlerted = true;
        ReactionTimer = 0.f;
        SuppressionTimer = FMath::FRandRange(0.62f, 1.05f) + FMath::Clamp(HealthDamage / 45.f, 0.f, 0.55f);
        RepositionTimer = FMath::FRandRange(0.85f, 1.55f);
        CoverSeekTimer = 0.f;
        StrafeDirection = FMath::RandBool() ? 1 : -1;
        StrafeChangeTimer = FMath::FRandRange(0.45f, 1.1f);

        if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            LastSeenLocation = Player->GetActorLocation();
            TimeSinceSeen = 0.f;
            AlertNearbySquad(LastSeenLocation);
        }
    }

    if (Health <= 0.f && !bDead)
    {
        bDead = true;
        ReleaseFireSlot();
        GetCharacterMovement()->DisableMovement();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        // Every hit volume goes: the vest and helmet used to keep blocking Visibility, so a
        // corpse soaked up rounds, blocked sight lines and shielded others from grenades for 6 s.
        UStaticMeshComponent* const HitVolumes[] = { BodyMesh, HeadMesh, HelmetMesh, VestMesh, LeftLegMesh, RightLegMesh };
        for (UStaticMeshComponent* HitVolume : HitVolumes)
        {
            if (HitVolume) HitVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        if (MuzzleFlashLight) MuzzleFlashLight->SetVisibility(false);
        if (MuzzleFlashCone) MuzzleFlashCone->SetVisibility(false);
        BeginDeathFall();
        SetLifeSpan(6.f);

        if (AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->NotifyEnemyKilled();
        }
    }

    return HealthDamage;
}
