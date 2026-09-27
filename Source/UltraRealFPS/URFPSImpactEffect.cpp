#include "URFPSImpactEffect.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "URFPSAudio.h"

AURFPSImpactEffect::AURFPSImpactEffect()
{
    PrimaryActorTick.bCanEverTick = true;
    SetLifeSpan(5.0f);

    MarkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkMesh"));
    SetRootComponent(MarkMesh);
    MarkMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MarkMesh->SetCastShadow(false);
    MarkMesh->SetRelativeScale3D(FVector(0.032f, 0.032f, 0.0025f));

    // A very flat cylinder reads as an impact scar rather than the raised sphere used by
    // the early prototype. It remains an engine primitive so the effect has no asset dependency.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderAsset.Succeeded()) MarkMesh->SetStaticMesh(CylinderAsset.Object);

    FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
    FlashLight->SetupAttachment(MarkMesh);
    FlashLight->SetIntensity(1100.f);
    FlashLight->SetAttenuationRadius(105.f);
    FlashLight->SetLightColor(FLinearColor(1.f, 0.48f, 0.14f));
    FlashLight->SetCastShadows(false);
    FlashLight->SetUseInverseSquaredFalloff(true);
    FlashLight->SetSourceRadius(1.5f);
    FlashLight->SetSoftSourceRadius(4.f);
}

void AURFPSImpactEffect::BeginPlay()
{
    Super::BeginPlay();

    DebrisMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        ImpactMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        DebrisMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        if (ImpactMaterial) MarkMesh->SetMaterial(0, ImpactMaterial);
    }

    ApplyVisuals();
}

void AURFPSImpactEffect::ApplyVisuals()
{
    FLinearColor MarkColor(0.025f, 0.025f, 0.025f, 1.f);
    FLinearColor FlashColor(1.f, 0.48f, 0.14f, 1.f);
    FVector Scale(0.032f, 0.032f, 0.0025f);
    float FlashIntensity = 1100.f;
    float FlashRadius = 105.f;
    float Life = 5.f;

    if (bSoftImpact || ImpactSurface == SurfaceType4)
    {
        MarkColor = FLinearColor(0.16f, 0.018f, 0.012f, 1.f);
        FlashColor = FLinearColor(0.38f, 0.018f, 0.012f, 1.f);
        Scale = FVector(0.034f, 0.029f, 0.0022f);
        FlashIntensity = 180.f;
        FlashRadius = 45.f;
        Life = 0.28f;
    }
    else if (ImpactSurface == SurfaceType2)
    {
        MarkColor = FLinearColor(0.055f, 0.060f, 0.064f, 1.f);
        FlashColor = FLinearColor(1.f, 0.67f, 0.24f, 1.f);
        Scale = FVector(0.024f, 0.024f, 0.0018f);
        FlashIntensity = bWasRicochet ? 7600.f : 4100.f;
        FlashRadius = bWasRicochet ? 185.f : 130.f;
        Life = 4.2f;
    }
    else if (ImpactSurface == SurfaceType3)
    {
        MarkColor = FLinearColor(0.12f, 0.052f, 0.018f, 1.f);
        FlashColor = FLinearColor(0.68f, 0.28f, 0.075f, 1.f);
        Scale = FVector(0.040f, 0.030f, 0.0028f);
        FlashIntensity = 420.f;
        FlashRadius = 72.f;
        Life = 3.4f;
    }
    else
    {
        MarkColor = FLinearColor(0.020f, 0.020f, 0.019f, 1.f);
        FlashColor = FLinearColor(0.95f, 0.58f, 0.28f, 1.f);
        Scale = FVector(0.034f, 0.034f, 0.0025f);
        FlashIntensity = bWasRicochet ? 3200.f : 1250.f;
        FlashRadius = bWasRicochet ? 145.f : 105.f;
    }

    const float MarkVariation = FMath::FRandRange(0.90f, 1.12f);
    Scale.X *= MarkVariation;
    Scale.Y *= FMath::FRandRange(0.92f, 1.08f);

    if (ImpactMaterial) ImpactMaterial->SetVectorParameterValue(FName(TEXT("Color")), MarkColor);
    if (MarkMesh) MarkMesh->SetRelativeScale3D(Scale);
    if (FlashLight)
    {
        FlashLight->SetLightColor(FlashColor);
        FlashLight->SetIntensity(FlashIntensity * FMath::FRandRange(0.90f, 1.10f));
        FlashLight->SetAttenuationRadius(FlashRadius);
    }
    SetLifeSpan(Life);
}

void AURFPSImpactEffect::SpawnSurfaceDebris()
{
    if (bDebrisSpawned || bSoftImpact || ImpactSurface == SurfaceType4 || !DebrisMesh || !DebrisMaterial)
    {
        return;
    }
    bDebrisSpawned = true;

    int32 DebrisCount = 5;
    FLinearColor DebrisColor(0.20f, 0.20f, 0.19f, 1.f);
    float MinSpeed = 135.f;
    float MaxSpeed = 310.f;
    float MinLife = 0.30f;
    float MaxLife = 0.55f;
    float GravityScale = 1.0f;

    if (ImpactSurface == SurfaceType2)
    {
        DebrisCount = bWasRicochet ? 8 : 6;
        DebrisColor = FLinearColor(1.0f, 0.43f, 0.08f, 1.f);
        MinSpeed = 420.f;
        MaxSpeed = bWasRicochet ? 920.f : 720.f;
        MinLife = 0.12f;
        MaxLife = 0.26f;
        GravityScale = 0.30f;
    }
    else if (ImpactSurface == SurfaceType3)
    {
        DebrisCount = 6;
        DebrisColor = FLinearColor(0.26f, 0.105f, 0.025f, 1.f);
        MinSpeed = 160.f;
        MaxSpeed = 390.f;
        MinLife = 0.32f;
        MaxLife = 0.62f;
        GravityScale = 0.72f;
    }
    else
    {
        DebrisCount = 5;
        DebrisColor = FLinearColor(0.23f, 0.22f, 0.20f, 1.f);
    }

    DebrisMaterial->SetVectorParameterValue(FName(TEXT("Color")), DebrisColor);

    DebrisComponents.Reserve(DebrisCount);
    DebrisVelocities.Reserve(DebrisCount);
    DebrisInitialScales.Reserve(DebrisCount);
    DebrisLifeRemaining.Reserve(DebrisCount);
    DebrisLifeInitial.Reserve(DebrisCount);
    DebrisGravityScales.Reserve(DebrisCount);

    for (int32 Index = 0; Index < DebrisCount; ++Index)
    {
        UStaticMeshComponent* Debris = NewObject<UStaticMeshComponent>(this);
        if (!Debris) continue;

        Debris->SetMobility(EComponentMobility::Movable);
        Debris->SetupAttachment(RootComponent);
        Debris->SetAbsolute(true, true, true);
        Debris->SetStaticMesh(DebrisMesh);
        Debris->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Debris->SetCastShadow(false);
        Debris->SetMaterial(0, DebrisMaterial);
        Debris->RegisterComponent();

        FVector Direction = FMath::VRandCone(CachedSurfaceNormal, FMath::DegreesToRadians(68.f));
        if (Direction.IsNearlyZero()) Direction = CachedSurfaceNormal;
        Direction = Direction.GetSafeNormal();

        const float Speed = FMath::FRandRange(MinSpeed, MaxSpeed);
        const FVector Velocity = Direction * Speed + CachedSurfaceNormal * FMath::FRandRange(25.f, 90.f);

        FVector FragmentScale;
        if (ImpactSurface == SurfaceType2)
        {
            FragmentScale = FVector(FMath::FRandRange(0.040f, 0.075f), FMath::FRandRange(0.0035f, 0.0065f), FMath::FRandRange(0.0035f, 0.0065f));
        }
        else if (ImpactSurface == SurfaceType3)
        {
            FragmentScale = FVector(FMath::FRandRange(0.028f, 0.058f), FMath::FRandRange(0.005f, 0.010f), FMath::FRandRange(0.005f, 0.012f));
        }
        else
        {
            const float Size = FMath::FRandRange(0.007f, 0.016f);
            FragmentScale = FVector(Size, Size * FMath::FRandRange(0.65f, 1.05f), Size * FMath::FRandRange(0.55f, 0.95f));
        }

        const float Life = FMath::FRandRange(MinLife, MaxLife);
        Debris->SetWorldLocation(GetActorLocation() + CachedSurfaceNormal * FMath::FRandRange(1.0f, 3.0f));
        Debris->SetWorldRotation(Velocity.Rotation());
        Debris->SetWorldScale3D(FragmentScale);

        DebrisComponents.Add(Debris);
        DebrisVelocities.Add(Velocity);
        DebrisInitialScales.Add(FragmentScale);
        DebrisLifeRemaining.Add(Life);
        DebrisLifeInitial.Add(Life);
        DebrisGravityScales.Add(GravityScale * FMath::FRandRange(0.85f, 1.15f));
    }
}

void AURFPSImpactEffect::InitializeImpact(const FVector& SurfaceNormal, bool bCharacterImpact, EPhysicalSurface SurfaceType, bool bRicochet, bool bPlayAudio)
{
    bSoftImpact = bCharacterImpact;
    bWasRicochet = bRicochet;
    ImpactSurface = SurfaceType;

    CachedSurfaceNormal = SurfaceNormal.IsNearlyZero() ? FVector::UpVector : SurfaceNormal.GetSafeNormal();
    SetActorRotation(FRotationMatrix::MakeFromZ(CachedSurfaceNormal).Rotator());

    ApplyVisuals();
    SpawnSurfaceDebris();

    EURFPSAudioEvent Event = EURFPSAudioEvent::ImpactConcrete;
    if (bCharacterImpact || ImpactSurface == SurfaceType4) Event = EURFPSAudioEvent::ImpactFlesh;
    else if (ImpactSurface == SurfaceType2) Event = EURFPSAudioEvent::ImpactMetal;
    else if (ImpactSurface == SurfaceType3) Event = EURFPSAudioEvent::ImpactWood;

    if (bPlayAudio)
    {
        const float Volume = bCharacterImpact ? 0.42f : (bWasRicochet ? 0.74f : 0.56f);
        URFPSAudio::PlaySpatial(this, Event, GetActorLocation(), Volume, FMath::FRandRange(0.92f, 1.08f));
    }
}

void AURFPSImpactEffect::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (FlashLight)
    {
        FlashLight->SetIntensity(FMath::FInterpTo(FlashLight->Intensity, 0.f, DeltaSeconds, bWasRicochet ? 58.f : 44.f));
    }

    const int32 Count = DebrisComponents.Num();
    for (int32 Index = 0; Index < Count; ++Index)
    {
        UStaticMeshComponent* Debris = DebrisComponents.IsValidIndex(Index) ? DebrisComponents[Index] : nullptr;
        if (!IsValid(Debris) || !DebrisLifeRemaining.IsValidIndex(Index) || DebrisLifeRemaining[Index] <= 0.f)
        {
            continue;
        }

        DebrisLifeRemaining[Index] = FMath::Max(0.f, DebrisLifeRemaining[Index] - DeltaSeconds);
        if (DebrisLifeRemaining[Index] <= 0.f)
        {
            Debris->SetVisibility(false);
            continue;
        }

        FVector& Velocity = DebrisVelocities[Index];
        Velocity.Z -= 980.f * DebrisGravityScales[Index] * DeltaSeconds;
        Velocity *= FMath::Clamp(1.f - DeltaSeconds * 1.35f, 0.f, 1.f);

        Debris->SetWorldLocation(Debris->GetComponentLocation() + Velocity * DeltaSeconds);
        Debris->SetWorldRotation(Velocity.Rotation());

        const float LifeAlpha = FMath::Clamp(DebrisLifeRemaining[Index] / FMath::Max(DebrisLifeInitial[Index], KINDA_SMALL_NUMBER), 0.f, 1.f);
        const float ScaleAlpha = FMath::Lerp(0.28f, 1.f, LifeAlpha);
        Debris->SetWorldScale3D(DebrisInitialScales[Index] * ScaleAlpha);
    }
}
