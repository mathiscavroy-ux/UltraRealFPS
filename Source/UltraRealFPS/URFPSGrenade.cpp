#include "URFPSGrenade.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "URFPSAudio.h"
#include "URFPSCharacter.h"
#include "URFPSEnemy.h"
#include "URFPSImpactEffect.h"

AURFPSGrenade::AURFPSGrenade()
{
    PrimaryActorTick.bCanEverTick = true;
    SetReplicates(false);

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    SetRootComponent(CollisionSphere);
    CollisionSphere->InitSphereRadius(7.5f);
    CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
    GrenadeMesh->SetupAttachment(CollisionSphere);
    GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GrenadeMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.20f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereAsset.Succeeded()) GrenadeMesh->SetStaticMesh(SphereAsset.Object);

    ExplosionLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ExplosionLight"));
    ExplosionLight->SetupAttachment(CollisionSphere);
    ExplosionLight->SetIntensity(0.f);
    ExplosionLight->SetAttenuationRadius(1150.f);
    ExplosionLight->SetLightColor(FLinearColor(1.f, 0.38f, 0.08f));
    ExplosionLight->SetCastShadows(false);
    ExplosionLight->SetUseInverseSquaredFalloff(true);
    ExplosionLight->SetSourceRadius(18.f);
    ExplosionLight->SetSoftSourceRadius(38.f);
    ExplosionLight->SetVisibility(false);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->SetUpdatedComponent(CollisionSphere);
    ProjectileMovement->InitialSpeed = 1450.f;
    ProjectileMovement->MaxSpeed = 2100.f;
    ProjectileMovement->ProjectileGravityScale = 1.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = true;
    ProjectileMovement->Bounciness = 0.34f;
    ProjectileMovement->Friction = 0.52f;
    ProjectileMovement->BounceVelocityStopSimulatingThreshold = 55.f;
    ProjectileMovement->bForceSubStepping = true;
}

void AURFPSGrenade::BeginPlay()
{
    Super::BeginPlay();

    if (UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(ParentMaterial, this))
        {
            Material->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.08f, 0.095f, 0.07f, 1.f));
            GrenadeMesh->SetMaterial(0, Material);
        }
    }

    GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AURFPSGrenade::Explode, FuseSeconds, false);
}

void AURFPSGrenade::InitializeGrenade(const FVector& InitialVelocity, AController* InInstigatorController, AActor* InDamageCauser)
{
    InstigatorController = InInstigatorController;
    DamageCauserActor = InDamageCauser;
    if (CollisionSphere && InDamageCauser)
    {
        CollisionSphere->IgnoreActorWhenMoving(InDamageCauser, true);
    }
    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = InitialVelocity;
    }
}

void AURFPSGrenade::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bExploded)
    {
        if (ExplosionLight)
        {
            const float NewIntensity = FMath::FInterpTo(ExplosionLight->Intensity, 0.f, DeltaSeconds, 26.f);
            ExplosionLight->SetIntensity(NewIntensity);
        }
        return;
    }

    ThreatPulseCooldown -= DeltaSeconds;
    if (ThreatPulseCooldown <= 0.f)
    {
        ThreatPulseCooldown = 0.22f;
        PulseThreat();
    }
}

void AURFPSGrenade::PulseThreat()
{
    if (!GetWorld()) return;

    for (TActorIterator<AURFPSEnemy> It(GetWorld()); It; ++It)
    {
        AURFPSEnemy* Enemy = *It;
        if (!IsValid(Enemy) || Enemy->IsEnemyDead()) continue;
        if (FVector::DistSquared(Enemy->GetActorLocation(), GetActorLocation()) <= FMath::Square(1150.f))
        {
            Enemy->ReactToGrenade(GetActorLocation());
        }
    }
}

float AURFPSGrenade::ComputeOccludedDamage(AActor* Target, float Distance) const
{
    if (!Target || !GetWorld()) return 0.f;

    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 8.f);
    const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GrenadeOcclusion), true, this);
    Params.AddIgnoredActor(this);
    if (DamageCauserActor) Params.AddIgnoredActor(DamageCauserActor);

    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (Hit.GetActor() != Target)
        {
            return 0.f;
        }
    }

    const float Alpha = FMath::Clamp(1.f - Distance / ExplosionRadius, 0.f, 1.f);
    const float CurvedAlpha = Alpha * Alpha;
    const float Maximum = Cast<AURFPSCharacter>(Target) ? MaxPlayerDamage : MaxEnemyDamage;
    return FMath::Lerp(6.f, Maximum, CurvedAlpha);
}

void AURFPSGrenade::Explode()
{
    if (bExploded || !GetWorld()) return;
    bExploded = true;

    if (ProjectileMovement) ProjectileMovement->StopMovementImmediately();
    if (CollisionSphere) CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (GrenadeMesh) GrenadeMesh->SetVisibility(false);
    if (ExplosionLight)
    {
        ExplosionLight->SetVisibility(true);
        ExplosionLight->SetLightColor(FLinearColor(
            1.f,
            FMath::FRandRange(0.31f, 0.43f),
            FMath::FRandRange(0.045f, 0.10f),
            1.f));
        ExplosionLight->SetIntensity(FMath::FRandRange(76000.f, 94000.f));
        ExplosionLight->SetAttenuationRadius(FMath::FRandRange(980.f, 1280.f));
    }

    URFPSAudio::PlaySpatial(this, EURFPSAudioEvent::GrenadeExplosion, GetActorLocation(), 1.0f, FMath::FRandRange(0.97f, 1.03f));

    // Cast a small number of fragmentation probes into nearby geometry. They create silent,
    // material-aware impact scars/debris without turning the explosion into an audio spam burst.
    FCollisionQueryParams FragmentParams(SCENE_QUERY_STAT(GrenadeFragmentVisuals), false, this);
    FragmentParams.AddIgnoredActor(this);
    if (DamageCauserActor) FragmentParams.AddIgnoredActor(DamageCauserActor);
    FragmentParams.bReturnPhysicalMaterial = true;

    const FVector BlastOrigin = GetActorLocation() + FVector(0.f, 0.f, 18.f);
    for (int32 FragmentIndex = 0; FragmentIndex < 9; ++FragmentIndex)
    {
        FVector FragmentDirection = FMath::VRand();
        FragmentDirection.Z = FMath::Clamp(FragmentDirection.Z + 0.18f, -0.55f, 1.f);
        FragmentDirection = FragmentDirection.GetSafeNormal();

        FHitResult FragmentHit;
        const FVector FragmentEnd = BlastOrigin + FragmentDirection * FMath::FRandRange(260.f, 520.f);
        if (!GetWorld()->LineTraceSingleByChannel(FragmentHit, BlastOrigin, FragmentEnd, ECC_Visibility, FragmentParams))
        {
            continue;
        }

        const EPhysicalSurface SurfaceType = UGameplayStatics::GetSurfaceType(FragmentHit);
        if (AURFPSImpactEffect* Impact = GetWorld()->SpawnActor<AURFPSImpactEffect>(
            AURFPSImpactEffect::StaticClass(),
            FragmentHit.ImpactPoint + FragmentHit.ImpactNormal * 1.5f,
            FragmentHit.ImpactNormal.Rotation()))
        {
            Impact->InitializeImpact(FragmentHit.ImpactNormal, false, SurfaceType, false, false);
        }
    }

    if (AURFPSCharacter* Player = Cast<AURFPSCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        const float Distance = FVector::Dist(Player->GetActorLocation(), GetActorLocation());
        if (Distance <= ExplosionRadius)
        {
            const float Damage = ComputeOccludedDamage(Player, Distance);
            if (Damage > 0.f)
            {
                UGameplayStatics::ApplyDamage(Player, Damage, InstigatorController, DamageCauserActor ? DamageCauserActor : this, nullptr);
                Player->ApplySuppression(FMath::Clamp(1.f - Distance / ExplosionRadius, 0.25f, 1.f), GetActorLocation());
            }
        }
    }

    for (TActorIterator<AURFPSEnemy> It(GetWorld()); It; ++It)
    {
        AURFPSEnemy* Enemy = *It;
        if (!IsValid(Enemy) || Enemy->IsEnemyDead()) continue;

        const float Distance = FVector::Dist(Enemy->GetActorLocation(), GetActorLocation());
        if (Distance > ExplosionRadius) continue;

        const float Damage = ComputeOccludedDamage(Enemy, Distance);
        if (Damage > 0.f)
        {
            UGameplayStatics::ApplyDamage(Enemy, Damage, InstigatorController, DamageCauserActor ? DamageCauserActor : this, nullptr);
            Enemy->ApplySuppression(GetActorLocation());
        }
    }

    SetLifeSpan(0.72f);
}
