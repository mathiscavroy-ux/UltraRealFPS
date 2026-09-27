#include "URFPSProjectile.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"
#include "URFPSAudio.h"
#include "URFPSCharacter.h"
#include "URFPSEnemy.h"
#include "URFPSImpactEffect.h"

AURFPSProjectile::AURFPSProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    SetLifeSpan(3.0f);
    SetActorEnableCollision(false);

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    TracerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TracerMesh"));
    TracerMesh->SetupAttachment(Root);
    TracerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TracerMesh->SetCastShadow(false);
    TracerMesh->SetRelativeScale3D(FVector(0.045f, 0.010f, 0.010f));
    TracerMesh->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        TracerMesh->SetStaticMesh(CubeMesh.Object);
    }

    TracerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TracerLight"));
    TracerLight->SetupAttachment(Root);
    TracerLight->SetIntensity(900.f);
    TracerLight->SetAttenuationRadius(115.f);
    TracerLight->SetLightColor(FLinearColor(1.f, 0.42f, 0.08f));
    TracerLight->SetCastShadows(false);
    TracerLight->SetVisibility(false);
}

void AURFPSProjectile::Initialize(const FVector& InVelocity, float InDamage, AController* InInstigatorController,
    AActor* InDamageCauser, bool bInTracerRound)
{
    Velocity = InVelocity;
    InitialSpeed = FMath::Max(1.f, Velocity.Size());
    Damage = InDamage;
    InstigatorController = InInstigatorController;
    DamageCauserActor = InDamageCauser;
    bTracerRound = bInTracerRound;
    RemainingPenetrations = 2;
    RemainingRicochets = 1;

    if (TracerMesh) TracerMesh->SetVisibility(bTracerRound);
    if (TracerLight) TracerLight->SetVisibility(bTracerRound);
}

void AURFPSProjectile::SpawnImpact(const FHitResult& Hit, EPhysicalSurface SurfaceType, bool bCharacterImpact, bool bRicochet)
{
    if (!GetWorld()) return;

    if (AURFPSImpactEffect* Impact = GetWorld()->SpawnActor<AURFPSImpactEffect>(
        AURFPSImpactEffect::StaticClass(), Hit.ImpactPoint + Hit.ImpactNormal * 1.5f, Hit.ImpactNormal.Rotation()))
    {
        Impact->InitializeImpact(Hit.ImpactNormal, bCharacterImpact, SurfaceType, bRicochet);
    }
}

bool AURFPSProjectile::TryHandleSurfaceInteraction(const FHitResult& Hit, EPhysicalSurface SurfaceType, const FVector& ImpactDirection)
{
    if (!GetWorld()) return false;

    const float Speed = Velocity.Size();
    const FVector SurfaceNormal = Hit.ImpactNormal.GetSafeNormal();
    const float HeadOnFactor = FMath::Clamp(FVector::DotProduct(-ImpactDirection, SurfaceNormal), 0.f, 1.f);

    // Wood penetration uses an approximate thickness test instead of a fixed teleport.
    // A reverse trace from beyond the panel estimates where the projectile would leave
    // the same component. Thick structures therefore remain cover while crates/panels
    // can be defeated by a rifle round with a visible loss of energy.
    if (SurfaceType == SurfaceType3 && RemainingPenetrations > 0 && Speed > 36000.f)
    {
        constexpr float MaxWoodPenetrationCm = 115.f;
        const FVector ProbeStart = Hit.ImpactPoint + ImpactDirection * MaxWoodPenetrationCm;
        const FVector ProbeEnd = Hit.ImpactPoint + ImpactDirection * 2.f;

        FCollisionQueryParams ExitParams(SCENE_QUERY_STAT(BallisticExitTrace), true, this);
        ExitParams.AddIgnoredActor(this);
        ExitParams.bReturnPhysicalMaterial = true;
        if (AActor* OwnerActor = GetOwner()) ExitParams.AddIgnoredActor(OwnerActor);

        FHitResult ExitHit;
        if (GetWorld()->LineTraceSingleByChannel(ExitHit, ProbeStart, ProbeEnd, ECC_Visibility, ExitParams))
        {
            const bool bSameActor = ExitHit.GetActor() == Hit.GetActor();
            const bool bSameComponent = !Hit.Component.IsValid() || ExitHit.Component == Hit.Component;
            if (bSameActor && bSameComponent)
            {
                const float Thickness = FVector::Distance(Hit.ImpactPoint, ExitHit.ImpactPoint);
                if (Thickness > 2.f && Thickness <= MaxWoodPenetrationCm)
                {
                    --RemainingPenetrations;
                    SpawnImpact(Hit, SurfaceType, false, false);
                    SpawnImpact(ExitHit, SurfaceType, false, false);

                    const float ThicknessAlpha = FMath::Clamp(Thickness / MaxWoodPenetrationCm, 0.f, 1.f);
                    const float SpeedRetention = FMath::Lerp(0.76f, 0.48f, ThicknessAlpha);
                    const float DamageRetention = FMath::Lerp(0.82f, 0.58f, ThicknessAlpha);
                    Damage *= DamageRetention;
                    Velocity *= SpeedRetention;
                    SetActorLocation(ExitHit.ImpactPoint + ImpactDirection * 4.f, false);
                    return true;
                }
            }
        }
    }

    float RicochetThreshold = 0.f;
    float MaximumChance = 0.f;
    float SpeedRetention = 0.f;
    float DamageRetention = 0.f;

    if (SurfaceType == SurfaceType2)
    {
        RicochetThreshold = 0.38f;
        MaximumChance = 0.78f;
        SpeedRetention = 0.64f;
        DamageRetention = 0.56f;
    }
    else if (SurfaceType == SurfaceType1)
    {
        RicochetThreshold = 0.20f;
        MaximumChance = 0.32f;
        SpeedRetention = 0.46f;
        DamageRetention = 0.42f;
    }

    if (RemainingRicochets > 0 && RicochetThreshold > 0.f && Speed > 30000.f && HeadOnFactor < RicochetThreshold)
    {
        const float GrazingAlpha = 1.f - FMath::Clamp(HeadOnFactor / RicochetThreshold, 0.f, 1.f);
        const float RicochetChance = MaximumChance * GrazingAlpha;
        if (FMath::FRand() <= RicochetChance)
        {
            --RemainingRicochets;
            SpawnImpact(Hit, SurfaceType, false, true);

            FVector Reflected = FMath::GetReflectionVector(ImpactDirection, SurfaceNormal).GetSafeNormal();
            Reflected = FMath::VRandCone(Reflected, FMath::DegreesToRadians(1.8f)).GetSafeNormal();
            Velocity = Reflected * Speed * SpeedRetention;
            Damage *= DamageRetention;
            SetActorLocation(Hit.ImpactPoint + SurfaceNormal * 2.5f + Reflected * 2.f, false);
            return true;
        }
    }

    return false;
}

void AURFPSProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!GetWorld() || Velocity.IsNearlyZero())
    {
        Destroy();
        return;
    }

    SetActorRotation(Velocity.Rotation());

    float RemainingTime = DeltaSeconds;
    while (RemainingTime > KINDA_SMALL_NUMBER)
    {
        const float Step = FMath::Min(MaxSubstep, RemainingTime);
        const FVector Start = GetActorLocation();

        const FVector Gravity(0.f, 0.f, GravityZ);
        const FVector NewVelocity = Velocity + Gravity * Step;
        const FVector End = Start + Velocity * Step + Gravity * (0.5f * Step * Step);

        FCollisionQueryParams Params(SCENE_QUERY_STAT(BallisticTrace), true, this);
        Params.AddIgnoredActor(this);
        Params.bReturnPhysicalMaterial = true;
        if (AActor* OwnerActor = GetOwner()) Params.AddIgnoredActor(OwnerActor);

        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            AActor* HitActor = Hit.GetActor();
            const FVector ImpactDirection = Velocity.GetSafeNormal();
            const bool bPlayerImpact = Cast<AURFPSCharacter>(HitActor) != nullptr;
            AURFPSEnemy* HitEnemy = Cast<AURFPSEnemy>(HitActor);
            const bool bEnemyImpact = HitEnemy != nullptr;
            const bool bCharacterActor = bPlayerImpact || bEnemyImpact;

            EPhysicalSurface SurfaceType = bCharacterActor ? SurfaceType4 : UGameplayStatics::GetSurfaceType(Hit);
            if (bEnemyImpact && Hit.Component.IsValid())
            {
                const FName ComponentName = Hit.Component->GetFName();
                if (ComponentName == FName(TEXT("HelmetMesh")) || ComponentName == FName(TEXT("VestMesh")))
                {
                    // Armor is still resolved by AURFPSEnemy::TakeDamage, but the impact feedback
                    // should read as hard plate/helmet contact instead of a flesh hit.
                    SurfaceType = SurfaceType2;
                }
            }
            const bool bSoftCharacterImpact = bCharacterActor && SurfaceType == SurfaceType4;

            if (!bCharacterActor && TryHandleSurfaceInteraction(Hit, SurfaceType, ImpactDirection))
            {
                RemainingTime -= Step;
                continue;
            }

            bool bHeadshot = false;
            if (HitActor)
            {
                // Enemy-on-enemy friendly fire is intentionally disabled for the current single-player combat prototype.
                if (Cast<AURFPSEnemy>(DamageCauserActor) && Cast<AURFPSEnemy>(HitActor))
                {
                    SpawnImpact(Hit, SurfaceType2, false, false);
                    SetActorLocation(Hit.ImpactPoint);
                    Destroy();
                    return;
                }

                if (AURFPSEnemy* Enemy = Cast<AURFPSEnemy>(HitActor))
                {
                    if (Hit.Component.IsValid())
                    {
                        bHeadshot = Hit.Component->GetFName() == FName(TEXT("HeadMesh"));
                    }
                    if (!bHeadshot)
                    {
                        const float RelativeHeight = Hit.ImpactPoint.Z - Enemy->GetActorLocation().Z;
                        bHeadshot = RelativeHeight > 48.f;
                    }
                }
                else if (Cast<AURFPSCharacter>(HitActor))
                {
                    const float RelativeHeight = Hit.ImpactPoint.Z - HitActor->GetActorLocation().Z;
                    bHeadshot = RelativeHeight > 72.f;
                }

                const float VelocityScale = FMath::Clamp(Velocity.Size() / InitialSpeed, 0.50f, 1.f);
                const float FinalDamage = Damage * VelocityScale;
                const float AppliedDamage = UGameplayStatics::ApplyPointDamage(
                    HitActor,
                    FinalDamage,
                    ImpactDirection,
                    Hit,
                    InstigatorController,
                    DamageCauserActor ? DamageCauserActor : this,
                    nullptr);

                if (AppliedDamage > 0.f)
                {
                    if (AURFPSCharacter* Shooter = Cast<AURFPSCharacter>(DamageCauserActor))
                    {
                        Shooter->RegisterConfirmedHit(bHeadshot);
                    }
                }
            }

            SpawnImpact(Hit, SurfaceType, bSoftCharacterImpact, false);
            SetActorLocation(Hit.ImpactPoint);
            Destroy();
            return;
        }

        // Enemy rounds can suppress the player on a genuine near miss, but only after
        // this segment has been confirmed unobstructed by the ballistic trace above.
        if (!bSuppressionApplied && DamageCauserActor && Cast<AURFPSEnemy>(DamageCauserActor))
        {
            if (AURFPSCharacter* Player = Cast<AURFPSCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
            {
                const FVector Segment = End - Start;
                const float SegmentLengthSq = Segment.SizeSquared();
                if (SegmentLengthSq > KINDA_SMALL_NUMBER)
                {
                    const FVector TargetPoint = Player->GetActorLocation() + FVector(0.f, 0.f, 35.f);
                    const float T = FMath::Clamp(FVector::DotProduct(TargetPoint - Start, Segment) / SegmentLengthSq, 0.f, 1.f);
                    const FVector ClosestPoint = Start + Segment * T;
                    const float MissDistance = FVector::Dist(ClosestPoint, TargetPoint);
                    if (MissDistance < 165.f)
                    {
                        const float Intensity = FMath::GetMappedRangeValueClamped(FVector2D(165.f, 35.f), FVector2D(0.16f, 0.52f), MissDistance);
                        Player->ApplySuppression(Intensity, DamageCauserActor->GetActorLocation());
                        URFPSAudio::PlaySpatial(this, EURFPSAudioEvent::BulletCrack, ClosestPoint,
                            FMath::Lerp(0.52f, 0.95f, Intensity), FMath::FRandRange(0.94f, 1.08f));
                        bSuppressionApplied = true;
                    }
                }
            }
        }

        TraveledDistance += FVector::Distance(Start, End);
        SetActorLocation(End, false);
        Velocity = NewVelocity * FMath::Exp(-DragPerSecond * Step);
        RemainingTime -= Step;
    }
}
