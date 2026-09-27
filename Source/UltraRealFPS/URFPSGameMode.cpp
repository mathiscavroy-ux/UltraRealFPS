#include "URFPSGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "URFPSCharacter.h"
#include "URFPSDoor.h"
#include "URFPSEnemy.h"
#include "URFPSHUD.h"

AURFPSGameMode::AURFPSGameMode()
{
    DefaultPawnClass = AURFPSCharacter::StaticClass();
    HUDClass = AURFPSHUD::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded()) CubeMesh = CubeAsset.Object;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderAsset.Succeeded()) CylinderMesh = CylinderAsset.Object;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (MaterialAsset.Succeeded()) BaseShapeMaterial = MaterialAsset.Object;
}

void AURFPSGameMode::StartPlay()
{
    DestroyLegacyLighting();
    BuildLighting();
    CreateMaterials();
    BuildArena();
    Super::StartPlay();
    SpawnSupplies();
    SpawnWave();
}

void AURFPSGameMode::DestroyLegacyLighting()
{
    if (!GetWorld()) return;

    TArray<AActor*> ActorsToDestroy;
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
    {
        ActorsToDestroy.Add(*It);
    }
    for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
    {
        ActorsToDestroy.Add(*It);
    }

    for (AActor* Actor : ActorsToDestroy)
    {
        if (IsValid(Actor)) Actor->Destroy();
    }
}

void AURFPSGameMode::BuildLighting()
{
    if (!GetWorld()) return;

    GetWorld()->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-34.f, -32.f, 0.f));
    if (Sun)
    {
        UDirectionalLightComponent* SunComponent = Sun->GetComponent();
        if (SunComponent)
        {
            SunComponent->SetMobility(EComponentMobility::Movable);
            SunComponent->SetIntensity(8.2f);
            SunComponent->SetLightColor(FLinearColor(1.f, 0.93f, 0.82f));
            SunComponent->SetAtmosphereSunLight(true);
            SunComponent->SetVolumetricScatteringIntensity(0.72f);
        }
    }

    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(0.f, 0.f, -90.f), FRotator::ZeroRotator);
    if (Fog && Fog->GetComponent())
    {
        UExponentialHeightFogComponent* FogComponent = Fog->GetComponent();
        FogComponent->SetFogDensity(0.0025f);
        FogComponent->SetFogHeightFalloff(0.36f);
        FogComponent->SetFogInscatteringColor(FLinearColor(0.53f, 0.59f, 0.64f));
        FogComponent->SetVolumetricFog(true);
        FogComponent->SetVolumetricFogDistance(6200.f);
        FogComponent->SetVolumetricFogScatteringDistribution(0.42f);
        FogComponent->SetVolumetricFogAlbedo(FColor(215, 220, 225));
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator);
    if (Sky && Sky->GetLightComponent())
    {
        USkyLightComponent* SkyComponent = Sky->GetLightComponent();
        SkyComponent->SetMobility(EComponentMobility::Movable);
        SkyComponent->SetIntensity(1.32f);
        SkyComponent->SetRealTimeCapture(true);
        SkyComponent->SetLowerHemisphereColor(FLinearColor(0.050f, 0.060f, 0.070f));
    }
}

void AURFPSGameMode::CreateMaterials()
{
    if (!BaseShapeMaterial) return;

    auto MakeMaterial = [this](const FLinearColor& Color)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
        if (Material) Material->SetVectorParameterValue(FName(TEXT("Color")), Color);
        return Material;
    };

    FloorMaterial = MakeMaterial(FLinearColor(0.105f, 0.115f, 0.120f, 1.f));
    WallMaterial = MakeMaterial(FLinearColor(0.175f, 0.185f, 0.190f, 1.f));
    CoverMaterial = MakeMaterial(FLinearColor(0.235f, 0.245f, 0.235f, 1.f));
    WoodMaterial = MakeMaterial(FLinearColor(0.235f, 0.145f, 0.070f, 1.f));
    MetalMaterial = MakeMaterial(FLinearColor(0.085f, 0.095f, 0.105f, 1.f));
    DarkMaterial = MakeMaterial(FLinearColor(0.048f, 0.052f, 0.058f, 1.f));
    AccentMaterial = MakeMaterial(FLinearColor(0.34f, 0.115f, 0.035f, 1.f));
    HazardMaterial = MakeMaterial(FLinearColor(0.62f, 0.42f, 0.035f, 1.f));
    AmmoMaterial = MakeMaterial(FLinearColor(0.20f, 0.25f, 0.10f, 1.f));
    MedicalMaterial = MakeMaterial(FLinearColor(0.10f, 0.28f, 0.16f, 1.f));
    GrenadeMaterial = MakeMaterial(FLinearColor(0.20f, 0.18f, 0.07f, 1.f));

    ConcretePhysicalMaterial = NewObject<UPhysicalMaterial>(this, FName(TEXT("PM_Runtime_Concrete")));
    MetalPhysicalMaterial = NewObject<UPhysicalMaterial>(this, FName(TEXT("PM_Runtime_Metal")));
    WoodPhysicalMaterial = NewObject<UPhysicalMaterial>(this, FName(TEXT("PM_Runtime_Wood")));
    if (ConcretePhysicalMaterial) ConcretePhysicalMaterial->SurfaceType = SurfaceType1;
    if (MetalPhysicalMaterial) MetalPhysicalMaterial->SurfaceType = SurfaceType2;
    if (WoodPhysicalMaterial) WoodPhysicalMaterial->SurfaceType = SurfaceType3;
}

UMaterialInterface* AURFPSGameMode::GetMaterialForStyle(EBlockStyle Style) const
{
    switch (Style)
    {
    case EBlockStyle::Floor: return FloorMaterial;
    case EBlockStyle::Cover: return CoverMaterial;
    case EBlockStyle::Wood: return WoodMaterial;
    case EBlockStyle::Metal: return MetalMaterial;
    case EBlockStyle::Dark: return DarkMaterial;
    case EBlockStyle::Accent: return AccentMaterial;
    case EBlockStyle::Hazard: return HazardMaterial;
    case EBlockStyle::SupplyAmmo: return AmmoMaterial;
    case EBlockStyle::SupplyMedical: return MedicalMaterial;
    case EBlockStyle::SupplyGrenade: return GrenadeMaterial;
    default: return WallMaterial;
    }
}


UPhysicalMaterial* AURFPSGameMode::GetPhysicalMaterialForStyle(EBlockStyle Style) const
{
    switch (Style)
    {
    case EBlockStyle::Wood:
        return WoodPhysicalMaterial;
    case EBlockStyle::Metal:
    case EBlockStyle::Accent:
    case EBlockStyle::Hazard:
    case EBlockStyle::SupplyAmmo:
    case EBlockStyle::SupplyMedical:
    case EBlockStyle::SupplyGrenade:
        return MetalPhysicalMaterial;
    default:
        return ConcretePhysicalMaterial;
    }
}

void AURFPSGameMode::SpawnBlock(const FVector& Location, const FVector& Scale, const FRotator& Rotation, bool bCastShadow, EBlockStyle Style)
{
    if (!CubeMesh || !GetWorld()) return;

    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Block) return;

    UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(CubeMesh);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetCastShadow(bCastShadow);
    if (UMaterialInterface* Material = GetMaterialForStyle(Style)) Mesh->SetMaterial(0, Material);
    if (UPhysicalMaterial* PhysicalMaterial = GetPhysicalMaterialForStyle(Style)) Mesh->SetPhysMaterialOverride(PhysicalMaterial);
    Block->SetActorScale3D(Scale);
}

AStaticMeshActor* AURFPSGameMode::SpawnTaggedBlock(const FVector& Location, const FVector& Scale, FName Tag, EBlockStyle Style)
{
    if (!CubeMesh || !GetWorld()) return nullptr;

    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
    if (!Block) return nullptr;

    UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(CubeMesh);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetCastShadow(true);
    if (UMaterialInterface* Material = GetMaterialForStyle(Style)) Mesh->SetMaterial(0, Material);
    if (UPhysicalMaterial* PhysicalMaterial = GetPhysicalMaterialForStyle(Style)) Mesh->SetPhysMaterialOverride(PhysicalMaterial);
    Block->SetActorScale3D(Scale);
    Block->Tags.Add(Tag);
    return Block;
}

void AURFPSGameMode::SpawnCylinder(const FVector& Location, const FVector& Scale, const FRotator& Rotation, EBlockStyle Style)
{
    if (!CylinderMesh || !GetWorld()) return;

    AStaticMeshActor* Prop = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Prop) return;

    UStaticMeshComponent* Mesh = Prop->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(CylinderMesh);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    if (UMaterialInterface* Material = GetMaterialForStyle(Style)) Mesh->SetMaterial(0, Material);
    if (UPhysicalMaterial* PhysicalMaterial = GetPhysicalMaterialForStyle(Style)) Mesh->SetPhysMaterialOverride(PhysicalMaterial);
    Prop->SetActorScale3D(Scale);
}

AURFPSDoor* AURFPSGameMode::SpawnDoor(const FVector& Location, const FRotator& Rotation)
{
    if (!GetWorld()) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return GetWorld()->SpawnActor<AURFPSDoor>(AURFPSDoor::StaticClass(), Location, Rotation, Params);
}

void AURFPSGameMode::BuildArena()
{
    // Large dynamic tactical compound. Ground top surface is roughly Z=-100.
    SpawnBlock(FVector(0.f, 0.f, -150.f), FVector(110.f, 110.f, 1.f), FRotator::ZeroRotator, false, EBlockStyle::Floor);

    // Perimeter wall.
    SpawnBlock(FVector(5500.f, 0.f, 125.f), FVector(1.f, 55.f, 4.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(-5500.f, 0.f, 125.f), FVector(1.f, 55.f, 4.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(0.f, 5500.f, 125.f), FVector(55.f, 1.f, 4.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(0.f, -5500.f, 125.f), FVector(55.f, 1.f, 4.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);

    // Spawn pocket and operations desk.
    SpawnBlock(FVector(0.f, 620.f, -25.f), FVector(4.3f, 0.35f, 1.5f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(-620.f, 220.f, -25.f), FVector(0.35f, 3.2f, 1.5f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(620.f, 220.f, -25.f), FVector(0.35f, 3.2f, 1.5f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnTaggedBlock(FVector(0.f, 395.f, -62.f), FVector(0.50f, 0.34f, 0.42f), TEXT("WaveControl"), EBlockStyle::Accent);

    // Central checkpoint and broken road barriers.
    SpawnBlock(FVector(900.f, 500.f, -45.f), FVector(2.3f, 0.40f, 1.1f), FRotator(0.f, 14.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(1250.f, -620.f, -45.f), FVector(1.8f, 0.40f, 1.1f), FRotator(0.f, -28.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(-750.f, 1100.f, -45.f), FVector(2.0f, 0.40f, 1.1f), FRotator(0.f, 68.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(-1350.f, -900.f, -45.f), FVector(2.2f, 0.40f, 1.1f), FRotator(0.f, -16.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(2200.f, -950.f, -45.f), FVector(2.8f, 0.40f, 1.1f), FRotator(0.f, 32.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(-120.f, -1650.f, -45.f), FVector(3.2f, 0.35f, 1.1f), FRotator(0.f, 8.f, 0.f), true, EBlockStyle::Cover);

    // West shoot-house: offset rooms and cross angles.
    SpawnBlock(FVector(-3000.f, 1050.f, 50.f), FVector(16.f, 0.35f, 3.f));
    SpawnBlock(FVector(-3000.f, 3050.f, 50.f), FVector(16.f, 0.35f, 3.f));
    SpawnBlock(FVector(-4550.f, 2050.f, 50.f), FVector(0.35f, 10.f, 3.f));
    SpawnBlock(FVector(-1450.f, 1300.f, 50.f), FVector(0.35f, 2.5f, 3.f));
    SpawnBlock(FVector(-1450.f, 2800.f, 50.f), FVector(0.35f, 2.5f, 3.f));
    SpawnBlock(FVector(-3050.f, 2050.f, 50.f), FVector(0.30f, 5.5f, 3.f));
    SpawnBlock(FVector(-2450.f, 2050.f, 50.f), FVector(5.3f, 0.30f, 3.f));
    SpawnBlock(FVector(-3700.f, 1550.f, 50.f), FVector(0.30f, 4.4f, 3.f));
    SpawnBlock(FVector(-3550.f, 2550.f, 50.f), FVector(4.4f, 0.30f, 3.f));

    // East warehouse with internal fire lanes.
    SpawnBlock(FVector(3550.f, 1850.f, 80.f), FVector(0.4f, 16.f, 3.6f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(1850.f, 3400.f, 80.f), FVector(17.f, 0.4f, 3.6f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(1850.f, 300.f, 80.f), FVector(17.f, 0.4f, 3.6f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(350.f, 1150.f, 80.f), FVector(0.4f, 8.0f, 3.6f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(350.f, 2700.f, 80.f), FVector(0.4f, 6.0f, 3.6f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(1700.f, 1200.f, -15.f), FVector(0.35f, 4.8f, 1.7f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(2450.f, 2350.f, -15.f), FVector(0.35f, 4.5f, 1.7f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(1150.f, 2450.f, -15.f), FVector(5.7f, 0.35f, 1.7f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(2920.f, 1150.f, -15.f), FVector(3.6f, 0.35f, 1.7f), FRotator::ZeroRotator, true, EBlockStyle::Cover);

    // South service yard.
    SpawnBlock(FVector(-2800.f, -3000.f, 25.f), FVector(7.0f, 0.35f, 2.2f));
    SpawnBlock(FVector(-3500.f, -2200.f, 25.f), FVector(0.35f, 8.0f, 2.2f));
    SpawnBlock(FVector(2800.f, -3150.f, 25.f), FVector(7.0f, 0.35f, 2.2f));
    SpawnBlock(FVector(3500.f, -2350.f, 25.f), FVector(0.35f, 8.0f, 2.2f));
    SpawnBlock(FVector(800.f, -3000.f, -30.f), FVector(2.4f, 0.55f, 1.35f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(-900.f, -3000.f, -30.f), FVector(2.4f, 0.55f, 1.35f), FRotator::ZeroRotator, true, EBlockStyle::Cover);

    // New north elevated observation lane: player can use it, AI stays ground based.
    SpawnBlock(FVector(0.f, 4050.f, 145.f), FVector(11.f, 2.0f, 0.30f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(-1180.f, 4050.f, 60.f), FVector(0.30f, 2.3f, 2.2f));
    SpawnBlock(FVector(1180.f, 4050.f, 60.f), FVector(0.30f, 2.3f, 2.2f));
    SpawnBlock(FVector(-1450.f, 4050.f, 10.f), FVector(4.0f, 1.1f, 0.24f), FRotator(0.f, 0.f, 18.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(1450.f, 4050.f, 10.f), FVector(4.0f, 1.1f, 0.24f), FRotator(0.f, 180.f, -18.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(0.f, 4050.f, 260.f), FVector(1.6f, 0.35f, 1.2f), FRotator::ZeroRotator, true, EBlockStyle::Metal);

    // Crate clusters.
    const TArray<FVector> Crates =
    {
        FVector(1650.f, 620.f, -58.f), FVector(1760.f, 620.f, -58.f), FVector(1705.f, 620.f, 24.f),
        FVector(-2000.f, 800.f, -58.f), FVector(-1890.f, 800.f, -58.f),
        FVector(2820.f, 850.f, -58.f), FVector(2930.f, 850.f, -58.f),
        FVector(-2700.f, -1450.f, -58.f), FVector(-2590.f, -1450.f, -58.f),
        FVector(650.f, -2200.f, -58.f), FVector(760.f, -2200.f, -58.f),
        FVector(-650.f, -2350.f, -58.f), FVector(-540.f, -2350.f, -58.f),
        FVector(2150.f, 3900.f, -58.f), FVector(-2150.f, 3900.f, -58.f)
    };
    for (const FVector& Crate : Crates)
    {
        SpawnBlock(Crate, FVector(0.78f, 0.78f, 0.78f), FRotator::ZeroRotator, true, EBlockStyle::Wood);
    }

    // Barrels / compact cover.
    SpawnCylinder(FVector(980.f, -1450.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);
    SpawnCylinder(FVector(1060.f, -1370.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);
    SpawnCylinder(FVector(-900.f, -1750.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);
    SpawnCylinder(FVector(-980.f, -1660.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);
    SpawnCylinder(FVector(3220.f, 2600.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);
    SpawnCylinder(FVector(-4100.f, 2400.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);

    // Long-range cover markers.
    SpawnBlock(FVector(3900.f, -1900.f, -35.f), FVector(1.3f, 1.3f, 1.3f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(4350.f, -1300.f, -35.f), FVector(1.2f, 1.2f, 1.3f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(-4000.f, -2100.f, -35.f), FVector(1.3f, 1.3f, 1.3f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(-4200.f, -900.f, -35.f), FVector(1.2f, 1.2f, 1.3f), FRotator::ZeroRotator, true, EBlockStyle::Cover);


    // Immersion update: east service annex with layered cover and a maintenance corridor.
    SpawnBlock(FVector(4450.f, 2550.f, 70.f), FVector(0.30f, 8.5f, 3.4f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4050.f, 3350.f, 70.f), FVector(4.2f, 0.30f, 3.4f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4050.f, 1750.f, 70.f), FVector(4.2f, 0.30f, 3.4f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4000.f, 2550.f, -35.f), FVector(0.35f, 2.1f, 1.25f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
    SpawnBlock(FVector(4250.f, 2200.f, -35.f), FVector(1.55f, 0.35f, 1.25f), FRotator(0.f, 12.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(4250.f, 2900.f, -35.f), FVector(1.55f, 0.35f, 1.25f), FRotator(0.f, -12.f, 0.f), true, EBlockStyle::Cover);

    // West loading lane and staggered concrete dividers.
    SpawnBlock(FVector(-4400.f, -2750.f, 20.f), FVector(0.35f, 9.0f, 2.1f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(-3550.f, -3650.f, 20.f), FVector(8.8f, 0.35f, 2.1f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(-3600.f, -2450.f, -42.f), FVector(1.8f, 0.32f, 1.0f), FRotator(0.f, 22.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(-3900.f, -3100.f, -42.f), FVector(1.8f, 0.32f, 1.0f), FRotator(0.f, -18.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(-3250.f, -3200.f, -42.f), FVector(1.8f, 0.32f, 1.0f), FRotator(0.f, 28.f, 0.f), true, EBlockStyle::Cover);

    // Hazard-marked center lane. These give the compound more visual structure without external assets.
    for (int32 MarkerIndex = -4; MarkerIndex <= 4; ++MarkerIndex)
    {
        SpawnBlock(FVector(static_cast<float>(MarkerIndex) * 520.f, -430.f, -92.f), FVector(1.5f, 0.055f, 0.035f),
            FRotator(0.f, MarkerIndex % 2 == 0 ? 18.f : -18.f, 0.f), false, EBlockStyle::Hazard);
    }

    // Small elevated firing shelves and ramps create vertical choices without requiring a nav mesh.
    SpawnBlock(FVector(-2450.f, -4050.f, 120.f), FVector(4.2f, 2.2f, 0.28f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(-1850.f, -4050.f, 5.f), FVector(4.2f, 1.05f, 0.22f), FRotator(0.f, 0.f, 17.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(2450.f, -4050.f, 120.f), FVector(4.2f, 2.2f, 0.28f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(1850.f, -4050.f, 5.f), FVector(4.2f, 1.05f, 0.22f), FRotator(0.f, 180.f, -17.f), true, EBlockStyle::Cover);

    // Additional prop clusters to break long sight lines.
    const TArray<FVector> ExtraCrates =
    {
        FVector(3650.f, -2650.f, -58.f), FVector(3760.f, -2650.f, -58.f), FVector(3705.f, -2650.f, 24.f),
        FVector(-3650.f, 3600.f, -58.f), FVector(-3540.f, 3600.f, -58.f),
        FVector(650.f, 3500.f, -58.f), FVector(760.f, 3500.f, -58.f), FVector(705.f, 3500.f, 24.f),
        FVector(-650.f, 3450.f, -58.f), FVector(-760.f, 3450.f, -58.f)
    };
    for (const FVector& Crate : ExtraCrates)
    {
        SpawnBlock(Crate, FVector(0.78f, 0.78f, 0.78f), FRotator::ZeroRotator, true, EBlockStyle::Wood);
    }

    // Audio / interaction update: enclosed security office with a real interactive door.
    // The doorway remains wide enough to avoid trapping the capsule while the door swings.
    SpawnBlock(FVector(4740.f, -4060.f, 35.f), FVector(0.30f, 4.6f, 2.7f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(3660.f, -4060.f, 35.f), FVector(0.30f, 4.6f, 2.7f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4200.f, -4510.f, 35.f), FVector(5.7f, 0.30f, 2.7f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(3880.f, -3610.f, 35.f), FVector(2.45f, 0.30f, 2.7f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4520.f, -3610.f, 35.f), FVector(2.45f, 0.30f, 2.7f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4200.f, -3610.f, 215.f), FVector(0.95f, 0.30f, 0.65f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnDoor(FVector(4200.f, -3610.f, -100.f), FRotator(0.f, 90.f, 0.f));
    if (APointLight* OfficeLight = GetWorld()->SpawnActor<APointLight>(FVector(4200.f, -4060.f, 190.f), FRotator::ZeroRotator))
    {
        if (UPointLightComponent* LightComponent = Cast<UPointLightComponent>(OfficeLight->GetLightComponent()))
        {
            LightComponent->SetMobility(EComponentMobility::Movable);
            LightComponent->SetIntensity(2600.f);
            LightComponent->SetAttenuationRadius(820.f);
            LightComponent->SetLightColor(FLinearColor(1.f, 0.72f, 0.48f));
            LightComponent->SetCastShadows(true);
        }
    }
    SpawnBlock(FVector(4300.f, -4170.f, -52.f), FVector(1.4f, 0.62f, 0.92f), FRotator(0.f, -8.f, 0.f), true, EBlockStyle::Cover);
    SpawnBlock(FVector(3970.f, -4100.f, -58.f), FVector(0.82f, 0.82f, 0.82f), FRotator::ZeroRotator, true, EBlockStyle::Wood);

    // Secondary maintenance room on the north-east side. It creates a second close-quarter
    // interaction point without making doors mandatory for the wave AI to reach the player.
    SpawnBlock(FVector(4860.f, 4100.f, 25.f), FVector(0.30f, 3.9f, 2.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4060.f, 4100.f, 25.f), FVector(0.30f, 3.9f, 2.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4460.f, 4480.f, 25.f), FVector(4.3f, 0.30f, 2.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4235.f, 3720.f, 25.f), FVector(1.75f, 0.30f, 2.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnBlock(FVector(4685.f, 3720.f, 25.f), FVector(1.75f, 0.30f, 2.5f), FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnDoor(FVector(4460.f, 3720.f, -100.f), FRotator(0.f, 90.f, 0.f));
    if (APointLight* MaintenanceLight = GetWorld()->SpawnActor<APointLight>(FVector(4460.f, 4100.f, 180.f), FRotator::ZeroRotator))
    {
        if (UPointLightComponent* LightComponent = Cast<UPointLightComponent>(MaintenanceLight->GetLightComponent()))
        {
            LightComponent->SetMobility(EComponentMobility::Movable);
            LightComponent->SetIntensity(1900.f);
            LightComponent->SetAttenuationRadius(720.f);
            LightComponent->SetLightColor(FLinearColor(0.62f, 0.76f, 1.f));
            LightComponent->SetCastShadows(true);
        }
    }
    SpawnCylinder(FVector(4630.f, 4220.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);
    SpawnCylinder(FVector(4550.f, 4300.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);

    // Ballistic validation lane in the south perimeter. The three panels intentionally
    // use different physical surfaces so penetration / ricochet behavior can be checked
    // in-game after a single build: wood (left), metal (center), concrete (right).
    SpawnBlock(FVector(-700.f, -4820.f, 20.f), FVector(2.1f, 0.20f, 2.0f), FRotator::ZeroRotator, true, EBlockStyle::Wood);
    SpawnBlock(FVector(0.f, -4820.f, 20.f), FVector(2.1f, 0.16f, 2.0f), FRotator::ZeroRotator, true, EBlockStyle::Metal);
    SpawnBlock(FVector(700.f, -4820.f, 20.f), FVector(2.1f, 0.45f, 2.0f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
}

void AURFPSGameMode::SpawnSupplies()
{
    if (!GetWorld()) return;

    TArray<AActor*> SuppliesToDestroy;
    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
    {
        AStaticMeshActor* Actor = *It;
        if (Actor && (Actor->ActorHasTag(FName(TEXT("SupplyAmmo"))) ||
            Actor->ActorHasTag(FName(TEXT("SupplyMedical"))) ||
            Actor->ActorHasTag(FName(TEXT("SupplyGrenade")))))
        {
            SuppliesToDestroy.Add(Actor);
        }
    }
    for (AActor* Supply : SuppliesToDestroy)
    {
        if (IsValid(Supply)) Supply->Destroy();
    }

    SpawnTaggedBlock(FVector(-390.f, 300.f, -65.f), FVector(0.55f, 0.55f, 0.45f), TEXT("SupplyAmmo"), EBlockStyle::SupplyAmmo);
    SpawnTaggedBlock(FVector(0.f, 300.f, -65.f), FVector(0.55f, 0.55f, 0.45f), TEXT("SupplyMedical"), EBlockStyle::SupplyMedical);
    SpawnTaggedBlock(FVector(390.f, 300.f, -65.f), FVector(0.55f, 0.55f, 0.45f), TEXT("SupplyGrenade"), EBlockStyle::SupplyGrenade);
}

bool AURFPSGameMode::IsSpawnVisibleToPlayer(const FVector& SpawnLocation) const
{
    if (!GetWorld()) return false;

    APawn* Player = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
    if (!Player) return false;

    const FVector Start = Player->GetActorLocation() + FVector(0.f, 0.f, 58.f);
    const FVector End = SpawnLocation + FVector(0.f, 0.f, 58.f);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(SpawnVisibility), true, Player);
    Params.AddIgnoredActor(Player);

    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    return !bHit;
}

void AURFPSGameMode::SpawnWave()
{
    if (!GetWorld()) return;

    bWaveCleared = false;
    EnemiesAlive = 0;
    WaveStartWorldTime = GetWorld()->GetTimeSeconds();

    const TArray<FVector> EnemyPositions =
    {
        FVector(1750.f, 950.f, 0.f), FVector(2850.f, -900.f, 0.f), FVector(3300.f, 1400.f, 0.f),
        FVector(2650.f, 2700.f, 0.f), FVector(-1850.f, 1550.f, 0.f), FVector(-2850.f, 1550.f, 0.f),
        FVector(-3450.f, -1200.f, 0.f), FVector(4050.f, -2250.f, 0.f), FVector(-3900.f, 2600.f, 0.f),
        FVector(3300.f, 2850.f, 0.f), FVector(-2600.f, -3000.f, 0.f), FVector(2450.f, -3250.f, 0.f),
        FVector(-4300.f, 4100.f, 0.f), FVector(4300.f, 4100.f, 0.f), FVector(-4650.f, 500.f, 0.f),
        FVector(4650.f, 350.f, 0.f), FVector(4100.f, 2550.f, 0.f), FVector(-3850.f, -3000.f, 0.f),
        FVector(-2350.f, -4100.f, 0.f), FVector(2350.f, -4100.f, 0.f)
    };

    TArray<int32> SpawnOrder;
    const int32 RotationOffset = (CurrentWave * 3) % EnemyPositions.Num();

    // First prefer positions that are not directly visible from the player spawn/current position.
    for (int32 Pass = 0; Pass < 2; ++Pass)
    {
        for (int32 Offset = 0; Offset < EnemyPositions.Num(); ++Offset)
        {
            const int32 Index = (RotationOffset + Offset * 5) % EnemyPositions.Num();
            if (SpawnOrder.Contains(Index)) continue;

            const bool bVisible = IsSpawnVisibleToPlayer(EnemyPositions[Index]);
            if ((Pass == 0 && !bVisible) || (Pass == 1 && bVisible))
            {
                SpawnOrder.Add(Index);
            }
        }
    }

    const int32 EnemyCount = FMath::Min(6 + CurrentWave, 10);
    for (int32 Index = 0; Index < EnemyCount && Index < SpawnOrder.Num(); ++Index)
    {
        const FVector Position = EnemyPositions[SpawnOrder[Index]];
        FVector FaceDirection = -Position;
        FaceDirection.Z = 0.f;
        const FRotator Rotation = FaceDirection.IsNearlyZero() ? FRotator::ZeroRotator : FaceDirection.Rotation();

        if (AURFPSEnemy* Enemy = GetWorld()->SpawnActor<AURFPSEnemy>(AURFPSEnemy::StaticClass(), Position, Rotation))
        {
            Enemy->ConfigureForWave(CurrentWave, Index + CurrentWave * 7);
            ++EnemiesAlive;
        }
    }
}

void AURFPSGameMode::NotifyEnemyKilled()
{
    CleanupFireSlots();
    if (EnemiesAlive <= 0) return;

    --EnemiesAlive;
    ++TotalKills;

    if (EnemiesAlive <= 0 && !bWaveCleared)
    {
        EnemiesAlive = 0;
        bWaveCleared = true;
        ActiveShooters.Reset();
        SpawnSupplies();
        GetWorldTimerManager().SetTimer(NextWaveTimerHandle, this, &AURFPSGameMode::StartNextWave, TimeBetweenWaves, false);
    }
}

void AURFPSGameMode::RequestNextWave()
{
    if (!bWaveCleared || !GetWorld()) return;
    GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
    StartNextWave();
}

void AURFPSGameMode::StartNextWave()
{
    ActiveShooters.Reset();
    ++CurrentWave;
    SpawnWave();
}

void AURFPSGameMode::CleanupFireSlots()
{
    ActiveShooters.RemoveAll([](const TWeakObjectPtr<AURFPSEnemy>& Entry)
    {
        return !Entry.IsValid() || Entry->IsEnemyDead();
    });
}

int32 AURFPSGameMode::GetMaxActiveShooters() const
{
    int32 MaxShooters = CurrentWave >= 5 ? 3 : 2;

    if (GetWorld())
    {
        if (const AURFPSCharacter* Player = Cast<AURFPSCharacter>(GetWorld()->GetFirstPlayerController()
            ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr))
        {
            if (Player->GetHealth() < 42.f) MaxShooters = FMath::Min(MaxShooters, 2);
            if (Player->GetHealth() < 22.f) MaxShooters = 1;
        }
    }

    return MaxShooters;
}

int32 AURFPSGameMode::GetActiveShooters() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AURFPSEnemy>& Entry : ActiveShooters)
    {
        if (Entry.IsValid() && !Entry->IsEnemyDead()) ++Count;
    }
    return Count;
}

bool AURFPSGameMode::TryAcquireFireSlot(AURFPSEnemy* Enemy)
{
    if (!Enemy || Enemy->IsEnemyDead()) return false;
    CleanupFireSlots();

    for (const TWeakObjectPtr<AURFPSEnemy>& Entry : ActiveShooters)
    {
        if (Entry.Get() == Enemy) return true;
    }

    if (ActiveShooters.Num() >= GetMaxActiveShooters()) return false;
    ActiveShooters.Add(Enemy);
    return true;
}

void AURFPSGameMode::ReleaseFireSlot(AURFPSEnemy* Enemy)
{
    if (!Enemy) return;
    ActiveShooters.RemoveAll([Enemy](const TWeakObjectPtr<AURFPSEnemy>& Entry)
    {
        return !Entry.IsValid() || Entry.Get() == Enemy;
    });
}

float AURFPSGameMode::GetNextWaveTimeRemaining() const
{
    if (!bWaveCleared || !GetWorld()) return 0.f;
    return FMath::Max(0.f, GetWorldTimerManager().GetTimerRemaining(NextWaveTimerHandle));
}

float AURFPSGameMode::GetCurrentWaveElapsed() const
{
    if (!GetWorld() || bWaveCleared) return 0.f;
    return FMath::Max(0.f, GetWorld()->GetTimeSeconds() - WaveStartWorldTime);
}

float AURFPSGameMode::GetWaveIntroAlpha() const
{
    const float Elapsed = GetCurrentWaveElapsed();
    if (Elapsed <= 0.f || Elapsed >= 3.2f) return 0.f;
    if (Elapsed < 0.35f) return Elapsed / 0.35f;
    if (Elapsed > 2.55f) return 1.f - (Elapsed - 2.55f) / 0.65f;
    return 1.f;
}

FString AURFPSGameMode::GetWaveName() const
{
    switch (CurrentWave % 6)
    {
    case 1: return TEXT("CONTACT INITIAL");
    case 2: return TEXT("PRESSION LATERALE");
    case 3: return TEXT("TIREURS SPECIALISES");
    case 4: return TEXT("CONTRE-ATTAQUE");
    case 5: return TEXT("ASSAUT COORDONNE");
    default: return TEXT("TENIR LE COMPOUND");
    }
}
