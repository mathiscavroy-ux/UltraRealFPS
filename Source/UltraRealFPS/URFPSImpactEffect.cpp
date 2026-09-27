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
    MarkMesh->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.012f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereAsset.Succeeded()) MarkMesh->SetStaticMesh(SphereAsset.Object);

    FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
    FlashLight->SetupAttachment(MarkMesh);
    FlashLight->SetIntensity(1500.f);
    FlashLight->SetAttenuationRadius(95.f);
    FlashLight->SetLightColor(FLinearColor(1.f, 0.48f, 0.14f));
    FlashLight->SetCastShadows(false);
}

void AURFPSImpactEffect::BeginPlay()
{
    Super::BeginPlay();

    if (UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        ImpactMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        if (ImpactMaterial) MarkMesh->SetMaterial(0, ImpactMaterial);
    }

    ApplyVisuals();
}

void AURFPSImpactEffect::ApplyVisuals()
{
    FLinearColor MarkColor(0.025f, 0.025f, 0.025f, 1.f);
    FLinearColor FlashColor(1.f, 0.48f, 0.14f, 1.f);
    FVector Scale(0.035f, 0.035f, 0.012f);
    float FlashIntensity = 1500.f;
    float Life = 5.f;

    if (bSoftImpact || ImpactSurface == SurfaceType4)
    {
        MarkColor = FLinearColor(0.18f, 0.020f, 0.015f, 1.f);
        FlashColor = FLinearColor(0.42f, 0.025f, 0.015f, 1.f);
        Scale = FVector(0.045f, 0.045f, 0.014f);
        FlashIntensity = 350.f;
        Life = 0.35f;
    }
    else if (ImpactSurface == SurfaceType2)
    {
        MarkColor = FLinearColor(0.09f, 0.095f, 0.10f, 1.f);
        FlashColor = FLinearColor(1.f, 0.72f, 0.32f, 1.f);
        Scale = FVector(0.026f, 0.026f, 0.010f);
        FlashIntensity = bWasRicochet ? 5200.f : 3200.f;
        Life = 4.0f;
    }
    else if (ImpactSurface == SurfaceType3)
    {
        MarkColor = FLinearColor(0.17f, 0.080f, 0.028f, 1.f);
        FlashColor = FLinearColor(0.72f, 0.32f, 0.10f, 1.f);
        Scale = FVector(0.050f, 0.040f, 0.018f);
        FlashIntensity = 650.f;
        Life = 3.0f;
    }
    else
    {
        MarkColor = FLinearColor(0.035f, 0.035f, 0.035f, 1.f);
        FlashColor = FLinearColor(0.92f, 0.62f, 0.35f, 1.f);
        Scale = FVector(0.042f, 0.042f, 0.014f);
        FlashIntensity = bWasRicochet ? 2800.f : 1100.f;
    }

    if (ImpactMaterial) ImpactMaterial->SetVectorParameterValue(FName(TEXT("Color")), MarkColor);
    if (MarkMesh) MarkMesh->SetRelativeScale3D(Scale);
    if (FlashLight)
    {
        FlashLight->SetLightColor(FlashColor);
        FlashLight->SetIntensity(FlashIntensity);
    }
    SetLifeSpan(Life);
}

void AURFPSImpactEffect::InitializeImpact(const FVector& SurfaceNormal, bool bCharacterImpact, EPhysicalSurface SurfaceType, bool bRicochet)
{
    bSoftImpact = bCharacterImpact;
    bWasRicochet = bRicochet;
    ImpactSurface = SurfaceType;

    if (!SurfaceNormal.IsNearlyZero())
    {
        SetActorRotation(SurfaceNormal.Rotation());
    }

    ApplyVisuals();

    EURFPSAudioEvent Event = EURFPSAudioEvent::ImpactConcrete;
    if (bCharacterImpact || ImpactSurface == SurfaceType4) Event = EURFPSAudioEvent::ImpactFlesh;
    else if (ImpactSurface == SurfaceType2) Event = EURFPSAudioEvent::ImpactMetal;
    else if (ImpactSurface == SurfaceType3) Event = EURFPSAudioEvent::ImpactWood;

    const float Volume = bCharacterImpact ? 0.42f : (bWasRicochet ? 0.74f : 0.56f);
    URFPSAudio::PlaySpatial(this, Event, GetActorLocation(), Volume, FMath::FRandRange(0.92f, 1.08f));
}

void AURFPSImpactEffect::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (FlashLight)
    {
        FlashLight->SetIntensity(FMath::FInterpTo(FlashLight->Intensity, 0.f, DeltaSeconds, bWasRicochet ? 46.f : 36.f));
    }
}
