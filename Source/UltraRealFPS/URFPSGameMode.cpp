#include "URFPSGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
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
    LoadImportedArtAssets();
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

    // Lower sun angle + reduced skylight flattening gives the compound stronger readable shapes.
    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-24.f, -36.f, 0.f));
    if (Sun)
    {
        UDirectionalLightComponent* SunComponent = Sun->GetComponent();
        if (SunComponent)
        {
            SunComponent->SetMobility(EComponentMobility::Movable);
            SunComponent->SetIntensity(7.35f);
            SunComponent->SetLightColor(FLinearColor(1.f, 0.88f, 0.74f));
            SunComponent->SetAtmosphereSunLight(true);
            SunComponent->SetVolumetricScatteringIntensity(0.62f);
        }
    }

    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(0.f, 0.f, -90.f), FRotator::ZeroRotator);
    if (Fog && Fog->GetComponent())
    {
        UExponentialHeightFogComponent* FogComponent = Fog->GetComponent();
        FogComponent->SetFogDensity(0.0018f);
        FogComponent->SetFogHeightFalloff(0.34f);
        FogComponent->SetFogInscatteringColor(FLinearColor(0.40f, 0.48f, 0.56f));
        FogComponent->SetVolumetricFog(true);
        FogComponent->SetVolumetricFogDistance(7000.f);
        FogComponent->SetVolumetricFogScatteringDistribution(0.36f);
        FogComponent->SetVolumetricFogAlbedo(FColor(198, 207, 214));
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator);
    if (Sky && Sky->GetLightComponent())
    {
        USkyLightComponent* SkyComponent = Sky->GetLightComponent();
        SkyComponent->SetMobility(EComponentMobility::Movable);
        SkyComponent->SetIntensity(1.05f);
        SkyComponent->SetRealTimeCapture(true);
        SkyComponent->SetLowerHemisphereColor(FLinearColor(0.025f, 0.032f, 0.040f));
    }

    if (APostProcessVolume* PostProcess = GetWorld()->SpawnActor<APostProcessVolume>(
        APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator))
    {
        PostProcess->bUnbound = true;
        PostProcess->BlendWeight = 1.f;
        PostProcess->Settings.bOverride_AutoExposureBias = true;
        PostProcess->Settings.AutoExposureBias = -0.32f;
        PostProcess->Settings.bOverride_AutoExposureMinBrightness = true;
        PostProcess->Settings.AutoExposureMinBrightness = -2.2f;
        PostProcess->Settings.bOverride_AutoExposureMaxBrightness = true;
        PostProcess->Settings.AutoExposureMaxBrightness = 1.8f;
        PostProcess->Settings.bOverride_AutoExposureSpeedUp = true;
        PostProcess->Settings.AutoExposureSpeedUp = 3.2f;
        PostProcess->Settings.bOverride_AutoExposureSpeedDown = true;
        PostProcess->Settings.AutoExposureSpeedDown = 1.4f;
        PostProcess->Settings.bOverride_LocalExposureHighlightContrastScale = true;
        PostProcess->Settings.LocalExposureHighlightContrastScale = 0.88f;
        PostProcess->Settings.bOverride_LocalExposureShadowContrastScale = true;
        PostProcess->Settings.LocalExposureShadowContrastScale = 0.90f;
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

    FloorMaterial = MakeMaterial(FLinearColor(0.060f, 0.066f, 0.070f, 1.f));
    WallMaterial = MakeMaterial(FLinearColor(0.125f, 0.132f, 0.136f, 1.f));
    CoverMaterial = MakeMaterial(FLinearColor(0.185f, 0.192f, 0.188f, 1.f));
    WoodMaterial = MakeMaterial(FLinearColor(0.205f, 0.118f, 0.050f, 1.f));
    MetalMaterial = MakeMaterial(FLinearColor(0.060f, 0.070f, 0.078f, 1.f));
    DarkMaterial = MakeMaterial(FLinearColor(0.024f, 0.028f, 0.034f, 1.f));
    AsphaltMaterial = MakeMaterial(FLinearColor(0.018f, 0.022f, 0.026f, 1.f));
    ConcreteLightMaterial = MakeMaterial(FLinearColor(0.235f, 0.235f, 0.220f, 1.f));
    PaintBlueMaterial = MakeMaterial(FLinearColor(0.018f, 0.090f, 0.160f, 1.f));
    AccentMaterial = MakeMaterial(FLinearColor(0.42f, 0.105f, 0.025f, 1.f));
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

void AURFPSGameMode::LoadImportedArtAssets()
{
    ArtContainer20 = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_Container20_A.SM_Container20_A"));
    ArtConcreteBarrier = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_ConcreteBarrier_A.SM_ConcreteBarrier_A"));
    ArtIndustrialCrate = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_IndustrialCrate_A.SM_IndustrialCrate_A"));
    ArtPallet = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_Pallet_A.SM_Pallet_A"));
    ArtElectricalCabinet = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_ElectricalCabinet_A.SM_ElectricalCabinet_A"));
    ArtHVAC = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_HVAC_Rooftop_A.SM_HVAC_Rooftop_A"));
    ArtPipeRack = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_PipeRack_Module_A.SM_PipeRack_Module_A"));
    ArtLoadingBay = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_LoadingBay_A.SM_LoadingBay_A"));
    ArtLampPost = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_LampPost_A.SM_LampPost_A"));
    ArtFuelTank = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_FuelTank_A.SM_FuelTank_A"));
    ArtStorageRack = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Industrial/SM_StorageRack_A.SM_StorageRack_A"));
}

AStaticMeshActor* AURFPSGameMode::SpawnImportedArtMesh(UStaticMesh* MeshAsset, const FVector& Location,
    const FRotator& Rotation, const FVector& Scale, bool bCollision, EBlockStyle SurfaceStyle)
{
    if (!MeshAsset || !GetWorld()) return nullptr;

    AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Actor) return nullptr;

    UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(MeshAsset);
    Mesh->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->SetCastShadow(true);
    if (UPhysicalMaterial* PhysicalMaterial = GetPhysicalMaterialForStyle(SurfaceStyle))
    {
        Mesh->SetPhysMaterialOverride(PhysicalMaterial);
    }
    Actor->SetActorScale3D(Scale);
    return Actor;
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
    case EBlockStyle::Asphalt: return AsphaltMaterial;
    case EBlockStyle::ConcreteLight: return ConcreteLightMaterial;
    case EBlockStyle::PaintBlue: return PaintBlueMaterial;
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
    case EBlockStyle::PaintBlue:
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

UInstancedStaticMeshComponent* AURFPSGameMode::GetOrCreateDetailISM(EBlockStyle Style, bool bCylinder, bool bCastShadow)
{
    const uint32 StyleBits = static_cast<uint32>(Style);
    const uint32 Key = StyleBits | (bCylinder ? (1u << 8) : 0u) | (bCastShadow ? (1u << 9) : 0u);

    if (UInstancedStaticMeshComponent** Existing = DetailISMCache.Find(Key))
    {
        return *Existing;
    }

    UStaticMesh* MeshAsset = bCylinder ? CylinderMesh : CubeMesh;
    if (!MeshAsset) return nullptr;

    const FName ComponentName(*FString::Printf(TEXT("DetailISM_%u"), Key));
    UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
    if (!ISM) return nullptr;

    ISM->SetMobility(EComponentMobility::Movable);
    ISM->SetStaticMesh(MeshAsset);
    ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ISM->SetGenerateOverlapEvents(false);
    ISM->SetCastShadow(bCastShadow);
    if (UMaterialInterface* Material = GetMaterialForStyle(Style))
    {
        ISM->SetMaterial(0, Material);
    }
    ISM->RegisterComponent();

    DetailISMComponents.Add(ISM);
    DetailISMCache.Add(Key, ISM);
    return ISM;
}

void AURFPSGameMode::SpawnDetailBlock(const FVector& Location, const FVector& Scale, const FRotator& Rotation,
    EBlockStyle Style, bool bCastShadow)
{
    if (UInstancedStaticMeshComponent* ISM = GetOrCreateDetailISM(Style, false, bCastShadow))
    {
        ISM->AddInstance(FTransform(Rotation, Location, Scale), true);
    }
}

void AURFPSGameMode::SpawnDetailCylinder(const FVector& Location, const FVector& Scale, const FRotator& Rotation,
    EBlockStyle Style, bool bCastShadow)
{
    if (UInstancedStaticMeshComponent* ISM = GetOrCreateDetailISM(Style, true, bCastShadow))
    {
        ISM->AddInstance(FTransform(Rotation, Location, Scale), true);
    }
}

AURFPSDoor* AURFPSGameMode::SpawnDoor(const FVector& Location, const FRotator& Rotation)
{
    if (!GetWorld()) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return GetWorld()->SpawnActor<AURFPSDoor>(AURFPSDoor::StaticClass(), Location, Rotation, Params);
}

void AURFPSGameMode::SpawnWorldLabel(const FVector& Location, const FRotator& Rotation, const FString& Label,
    const FColor& Color, float WorldSize)
{
    if (!GetWorld()) return;

    if (ATextRenderActor* TextActor = GetWorld()->SpawnActor<ATextRenderActor>(Location, Rotation))
    {
        if (UTextRenderComponent* Text = TextActor->GetTextRender())
        {
            Text->SetText(FText::FromString(Label));
            Text->SetTextRenderColor(Color);
            Text->SetWorldSize(WorldSize);
            Text->SetHorizontalAlignment(EHTA_Center);
            Text->SetVerticalAlignment(EVRTA_TextCenter);
            Text->SetHorizSpacingAdjust(1.5f);
            Text->SetCastShadow(false);
        }
    }
}

void AURFPSGameMode::BuildArena()
{
    // Large dynamic tactical compound. Ground top surface is roughly Z=-100.
    SpawnBlock(FVector(0.f, 0.f, -150.f), FVector(110.f, 110.f, 1.f), FRotator::ZeroRotator, false, EBlockStyle::Floor);

    // High-impact visual layout pass: broad road/asphalt zones and service aprons are visible
    // immediately from the spawn, unlike the previous small-detail-only pass.
    SpawnDetailBlock(FVector(0.f, -350.f, -96.f), FVector(50.f, 4.6f, 0.025f),
        FRotator::ZeroRotator, EBlockStyle::Asphalt, false);
    SpawnDetailBlock(FVector(1650.f, 1850.f, -95.f), FVector(13.5f, 12.5f, 0.030f),
        FRotator::ZeroRotator, EBlockStyle::ConcreteLight, false);
    SpawnDetailBlock(FVector(-3000.f, 2050.f, -95.f), FVector(14.5f, 8.8f, 0.030f),
        FRotator::ZeroRotator, EBlockStyle::ConcreteLight, false);
    SpawnDetailBlock(FVector(0.f, -3000.f, -95.f), FVector(34.f, 5.0f, 0.030f),
        FRotator::ZeroRotator, EBlockStyle::Asphalt, false);

    for (int32 LaneIndex = -7; LaneIndex <= 7; ++LaneIndex)
    {
        const float LaneX = static_cast<float>(LaneIndex) * 620.f;
        SpawnDetailBlock(FVector(LaneX, -350.f, -92.5f), FVector(1.8f, 0.055f, 0.015f),
            FRotator::ZeroRotator, EBlockStyle::Hazard, false);
    }
    for (int32 BayIndex = -4; BayIndex <= 4; ++BayIndex)
    {
        const float BayX = static_cast<float>(BayIndex) * 760.f;
        SpawnDetailBlock(FVector(BayX, -2650.f, -92.5f), FVector(2.3f, 0.045f, 0.015f),
            FRotator(0.f, BayIndex % 2 == 0 ? 5.f : -5.f, 0.f), EBlockStyle::ConcreteLight, false);
    }

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
            LightComponent->SetIntensity(2800.f);
            LightComponent->SetAttenuationRadius(860.f);
            LightComponent->SetLightColor(FLinearColor(1.f, 0.72f, 0.48f));
            LightComponent->SetUseInverseSquaredFalloff(true);
            LightComponent->SetSourceRadius(18.f);
            LightComponent->SetSoftSourceRadius(52.f);
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
            LightComponent->SetIntensity(2100.f);
            LightComponent->SetAttenuationRadius(760.f);
            LightComponent->SetLightColor(FLinearColor(0.62f, 0.76f, 1.f));
            LightComponent->SetUseInverseSquaredFalloff(true);
            LightComponent->SetSourceRadius(15.f);
            LightComponent->SetSoftSourceRadius(44.f);
            LightComponent->SetCastShadows(true);
        }
    }
    SpawnCylinder(FVector(4630.f, 4220.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);
    SpawnCylinder(FVector(4550.f, 4300.f, -50.f), FVector(0.42f, 0.42f, 1.0f), FRotator::ZeroRotator, EBlockStyle::Metal);

    // Visual-impact pass: low-cost structural detail. These pieces have no collision, so
    // they break up the "large grey boxes" silhouette without creating invisible gameplay snags.
    // East warehouse roof ribs and wall braces.
    for (int32 RibIndex = 0; RibIndex < 5; ++RibIndex)
    {
        const float RibY = 700.f + static_cast<float>(RibIndex) * 640.f;
        SpawnDetailBlock(FVector(1950.f, RibY, 245.f), FVector(15.2f, 0.055f, 0.055f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
    }
    SpawnDetailBlock(FVector(520.f, 1820.f, 150.f), FVector(0.07f, 14.2f, 0.07f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);
    SpawnDetailBlock(FVector(3420.f, 1820.f, 150.f), FVector(0.07f, 14.2f, 0.07f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);

    // Security office frame, skirting and a simple ceiling fixture.
    SpawnDetailBlock(FVector(4138.f, -3612.f, 35.f), FVector(0.055f, 0.09f, 2.55f),
        FRotator::ZeroRotator, EBlockStyle::Metal, true);
    SpawnDetailBlock(FVector(4262.f, -3612.f, 35.f), FVector(0.055f, 0.09f, 2.55f),
        FRotator::ZeroRotator, EBlockStyle::Metal, true);
    SpawnDetailBlock(FVector(4200.f, -3612.f, 273.f), FVector(0.68f, 0.09f, 0.055f),
        FRotator::ZeroRotator, EBlockStyle::Metal, true);
    SpawnDetailBlock(FVector(4200.f, -4060.f, 235.f), FVector(1.35f, 0.16f, 0.055f),
        FRotator::ZeroRotator, EBlockStyle::Cover, false);
    SpawnDetailBlock(FVector(4200.f, -4500.f, -88.f), FVector(5.1f, 0.045f, 0.045f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);
    SpawnDetailBlock(FVector(3668.f, -4060.f, -88.f), FVector(0.045f, 4.1f, 0.045f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);

    // Maintenance room service pipes and cable tray.
    SpawnDetailCylinder(FVector(4240.f, 4240.f, 205.f), FVector(0.050f, 0.050f, 2.7f),
        FRotator(0.f, 90.f, 0.f), EBlockStyle::Metal, true);
    SpawnDetailCylinder(FVector(4240.f, 4320.f, 185.f), FVector(0.032f, 0.032f, 2.7f),
        FRotator(0.f, 90.f, 0.f), EBlockStyle::Accent, false);
    SpawnDetailBlock(FVector(4460.f, 4435.f, 218.f), FVector(3.3f, 0.10f, 0.055f),
        FRotator::ZeroRotator, EBlockStyle::Dark, true);

    // Repeated floor edge markers make the central route read as an industrial service lane.
    for (int32 EdgeIndex = -4; EdgeIndex <= 4; ++EdgeIndex)
    {
        const float EdgeX = static_cast<float>(EdgeIndex) * 520.f;
        SpawnDetailBlock(FVector(EdgeX, -600.f, -94.f), FVector(1.7f, 0.035f, 0.018f),
            FRotator(0.f, 12.f, 0.f), EBlockStyle::Hazard, false);
        SpawnDetailBlock(FVector(EdgeX, -260.f, -94.f), FVector(1.7f, 0.035f, 0.018f),
            FRotator(0.f, -12.f, 0.f), EBlockStyle::Hazard, false);
    }

    // Visible-overhaul pass: large-scale silhouettes and color breaks.
    // East warehouse roof cap and blue facade bands.
    SpawnDetailBlock(FVector(1950.f, 1850.f, 365.f), FVector(16.8f, 16.2f, 0.16f),
        FRotator::ZeroRotator, EBlockStyle::Dark, true);
    SpawnDetailBlock(FVector(1850.f, 3388.f, 150.f), FVector(15.8f, 0.055f, 0.28f),
        FRotator::ZeroRotator, EBlockStyle::PaintBlue, false);
    SpawnDetailBlock(FVector(1850.f, 312.f, 150.f), FVector(15.8f, 0.055f, 0.28f),
        FRotator::ZeroRotator, EBlockStyle::PaintBlue, false);

    // West shoot-house gets a contrasting top band so it no longer reads as one giant dark wall.
    SpawnDetailBlock(FVector(-3000.f, 1058.f, 310.f), FVector(15.5f, 0.05f, 0.18f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);
    SpawnDetailBlock(FVector(-3000.f, 3042.f, 310.f), FVector(15.5f, 0.05f, 0.18f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);

    // Two industrial storage tanks make the skyline visibly different from spawn.
    SpawnCylinder(FVector(4100.f, 650.f, 45.f), FVector(1.25f, 1.25f, 2.6f),
        FRotator::ZeroRotator, EBlockStyle::Metal);
    SpawnCylinder(FVector(4400.f, 650.f, 45.f), FVector(1.25f, 1.25f, 2.6f),
        FRotator::ZeroRotator, EBlockStyle::Metal);
    for (int32 RingIndex = 0; RingIndex < 3; ++RingIndex)
    {
        const float RingZ = -25.f + static_cast<float>(RingIndex) * 120.f;
        SpawnDetailCylinder(FVector(4100.f, 650.f, RingZ), FVector(1.34f, 1.34f, 0.050f),
            FRotator::ZeroRotator, EBlockStyle::Accent, false);
        SpawnDetailCylinder(FVector(4400.f, 650.f, RingZ), FVector(1.34f, 1.34f, 0.050f),
            FRotator::ZeroRotator, EBlockStyle::Accent, false);
    }
    SpawnDetailCylinder(FVector(4250.f, 650.f, 250.f), FVector(0.08f, 0.08f, 2.0f),
        FRotator(0.f, 90.f, 0.f), EBlockStyle::Metal, true);

    // Main entry gantry creates an obvious landmark across the central lane.
    SpawnBlock(FVector(-900.f, -900.f, 120.f), FVector(0.28f, 0.28f, 3.2f),
        FRotator::ZeroRotator, true, EBlockStyle::Metal);
    SpawnBlock(FVector(900.f, -900.f, 120.f), FVector(0.28f, 0.28f, 3.2f),
        FRotator::ZeroRotator, true, EBlockStyle::Metal);
    SpawnDetailBlock(FVector(0.f, -900.f, 390.f), FVector(9.4f, 0.22f, 0.22f),
        FRotator::ZeroRotator, EBlockStyle::PaintBlue, true);
    SpawnDetailBlock(FVector(0.f, -900.f, 355.f), FVector(5.2f, 0.24f, 0.06f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);

    // Environment graphics pass: modular industrial dressing. Repeated pieces now use ISMs,
    // so the compound can carry more silhouette detail without one Actor per decorative mesh.

    // East warehouse: roof purlins, wall columns and high clerestory panels.
    for (int32 BeamIndex = 0; BeamIndex < 8; ++BeamIndex)
    {
        const float BeamX = 650.f + static_cast<float>(BeamIndex) * 390.f;
        SpawnDetailBlock(FVector(BeamX, 1850.f, 315.f), FVector(0.055f, 14.6f, 0.07f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
    }
    for (int32 ColumnIndex = 0; ColumnIndex < 6; ++ColumnIndex)
    {
        const float ColumnY = 520.f + static_cast<float>(ColumnIndex) * 540.f;
        SpawnDetailBlock(FVector(3485.f, ColumnY, 70.f), FVector(0.10f, 0.10f, 3.2f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
        SpawnDetailBlock(FVector(415.f, ColumnY, 70.f), FVector(0.10f, 0.10f, 3.2f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
    }
    for (int32 WindowIndex = 0; WindowIndex < 7; ++WindowIndex)
    {
        const float WindowX = 720.f + static_cast<float>(WindowIndex) * 410.f;
        SpawnDetailBlock(FVector(WindowX, 3358.f, 210.f), FVector(1.35f, 0.045f, 0.46f),
            FRotator::ZeroRotator, EBlockStyle::Accent, false);
    }

    // West shoot-house: door frames and horizontal wall trims make rooms readable at a glance.
    const TArray<FVector> ShootHouseFrameCenters =
    {
        FVector(-1452.f, 1680.f, 45.f), FVector(-1452.f, 2420.f, 45.f),
        FVector(-3052.f, 1720.f, 45.f), FVector(-3052.f, 2380.f, 45.f),
        FVector(-3702.f, 2050.f, 45.f)
    };
    for (const FVector& FrameCenter : ShootHouseFrameCenters)
    {
        SpawnDetailBlock(FrameCenter + FVector(0.f, -68.f, 0.f), FVector(0.055f, 0.055f, 2.45f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
        SpawnDetailBlock(FrameCenter + FVector(0.f, 68.f, 0.f), FVector(0.055f, 0.055f, 2.45f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
        SpawnDetailBlock(FrameCenter + FVector(0.f, 0.f, 225.f), FVector(0.055f, 0.72f, 0.055f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
    }
    SpawnDetailBlock(FVector(-3000.f, 1065.f, -82.f), FVector(14.8f, 0.055f, 0.045f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);
    SpawnDetailBlock(FVector(-3000.f, 3035.f, -82.f), FVector(14.8f, 0.055f, 0.045f),
        FRotator::ZeroRotator, EBlockStyle::Accent, false);

    // South service yard: two container-like masses with ribs and door seams.
    SpawnBlock(FVector(3000.f, -4200.f, -10.f), FVector(3.0f, 1.18f, 1.45f),
        FRotator(0.f, 5.f, 0.f), true, EBlockStyle::Dark);
    SpawnBlock(FVector(-3150.f, -4150.f, -10.f), FVector(3.2f, 1.18f, 1.45f),
        FRotator(0.f, -7.f, 0.f), true, EBlockStyle::Dark);
    for (int32 RibIndex = -3; RibIndex <= 3; ++RibIndex)
    {
        const float OffsetX = static_cast<float>(RibIndex) * 78.f;
        SpawnDetailBlock(FVector(3000.f + OffsetX, -4083.f, -5.f), FVector(0.035f, 0.045f, 1.28f),
            FRotator(0.f, 5.f, 0.f), EBlockStyle::Accent, false);
        SpawnDetailBlock(FVector(-3150.f + OffsetX, -4033.f, -5.f), FVector(0.035f, 0.045f, 1.28f),
            FRotator(0.f, -7.f, 0.f), EBlockStyle::Metal, false);
    }

    // Perimeter service lamps. Geometry is decorative/non-blocking; point lights provide
    // readable landmarks without changing combat collision.
    const TArray<FVector> LampPositions =
    {
        FVector(-4700.f, -3500.f, 260.f), FVector(-4700.f, 3300.f, 260.f),
        FVector(4700.f, -3200.f, 260.f), FVector(4700.f, 3200.f, 260.f),
        FVector(0.f, 4950.f, 260.f)
    };
    for (int32 LampIndex = 0; LampIndex < LampPositions.Num(); ++LampIndex)
    {
        const FVector Lamp = LampPositions[LampIndex];
        SpawnDetailCylinder(Lamp - FVector(0.f, 0.f, 165.f), FVector(0.045f, 0.045f, 2.2f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
        SpawnDetailBlock(Lamp, FVector(0.48f, 0.18f, 0.08f),
            FRotator::ZeroRotator, EBlockStyle::Cover, true);

        if (APointLight* YardLight = GetWorld()->SpawnActor<APointLight>(Lamp - FVector(0.f, 0.f, 18.f), FRotator::ZeroRotator))
        {
            if (UPointLightComponent* LightComponent = Cast<UPointLightComponent>(YardLight->GetLightComponent()))
            {
                LightComponent->SetMobility(EComponentMobility::Movable);
                LightComponent->SetIntensity(LampIndex % 2 == 0 ? 1750.f : 1450.f);
                LightComponent->SetAttenuationRadius(920.f);
                LightComponent->SetLightColor(LampIndex % 2 == 0
                    ? FLinearColor(0.78f, 0.86f, 1.0f)
                    : FLinearColor(1.0f, 0.79f, 0.56f));
                LightComponent->SetUseInverseSquaredFalloff(true);
                LightComponent->SetSourceRadius(12.f);
                LightComponent->SetSoftSourceRadius(36.f);
                LightComponent->SetCastShadows(false);
            }
        }
    }

    BuildIndustrialArchitecturePass();

    // Ballistic validation lane in the south perimeter. The three panels intentionally
    // use different physical surfaces so penetration / ricochet behavior can be checked
    // in-game after a single build: wood (left), metal (center), concrete (right).
    SpawnBlock(FVector(-700.f, -4820.f, 20.f), FVector(2.1f, 0.20f, 2.0f), FRotator::ZeroRotator, true, EBlockStyle::Wood);
    SpawnBlock(FVector(0.f, -4820.f, 20.f), FVector(2.1f, 0.16f, 2.0f), FRotator::ZeroRotator, true, EBlockStyle::Metal);
    SpawnBlock(FVector(700.f, -4820.f, 20.f), FVector(2.1f, 0.45f, 2.0f), FRotator::ZeroRotator, true, EBlockStyle::Cover);
}

void AURFPSGameMode::BuildIndustrialArchitecturePass()
{
    // This pass deliberately works at building scale. It should remain readable from spawn and
    // at medium combat distance instead of relying on tiny dressing pieces.

    // ---------------------------------------------------------------------
    // EAST WAREHOUSE — real loading frontage instead of one dark rectangle.
    // ---------------------------------------------------------------------
    SpawnBlock(FVector(1850.f, 205.f, -72.f), FVector(13.8f, 1.25f, 0.28f),
        FRotator::ZeroRotator, true, EBlockStyle::ConcreteLight);
    SpawnDetailBlock(FVector(1850.f, 120.f, 222.f), FVector(14.1f, 1.15f, 0.13f),
        FRotator::ZeroRotator, EBlockStyle::Dark, true);

    const TArray<float> WarehouseBayX = { 760.f, 1450.f, 2140.f, 2830.f };
    for (int32 BayIndex = 0; BayIndex < WarehouseBayX.Num(); ++BayIndex)
    {
        const float BayX = WarehouseBayX[BayIndex];

        // Door slab and colored header.
        SpawnDetailBlock(FVector(BayX, 294.f, 65.f), FVector(2.55f, 0.055f, 1.55f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
        SpawnDetailBlock(FVector(BayX, 286.f, 232.f), FVector(2.72f, 0.065f, 0.18f),
            FRotator::ZeroRotator, BayIndex == 1 ? EBlockStyle::Accent : EBlockStyle::PaintBlue, false);

        // Door ribs.
        for (int32 Rib = -2; Rib <= 2; ++Rib)
        {
            SpawnDetailBlock(FVector(BayX, 279.f, 65.f + static_cast<float>(Rib) * 52.f),
                FVector(2.35f, 0.025f, 0.022f), FRotator::ZeroRotator, EBlockStyle::ConcreteLight, false);
        }

        // Protective bollards.
        SpawnCylinder(FVector(BayX - 185.f, 115.f, -52.f), FVector(0.10f, 0.10f, 0.82f),
            FRotator::ZeroRotator, EBlockStyle::Hazard);
        SpawnCylinder(FVector(BayX + 185.f, 115.f, -52.f), FVector(0.10f, 0.10f, 0.82f),
            FRotator::ZeroRotator, EBlockStyle::Hazard);
    }

    for (int32 PostIndex = 0; PostIndex < 7; ++PostIndex)
    {
        const float PostX = 520.f + static_cast<float>(PostIndex) * 470.f;
        SpawnBlock(FVector(PostX, 118.f, 62.f), FVector(0.09f, 0.09f, 2.85f),
            FRotator::ZeroRotator, true, EBlockStyle::Metal);
    }

    // Roof ventilation / service units.
    for (int32 VentIndex = 0; VentIndex < 5; ++VentIndex)
    {
        const float VentX = 720.f + static_cast<float>(VentIndex) * 620.f;
        SpawnDetailBlock(FVector(VentX, 1850.f, 430.f), FVector(0.62f, 0.78f, 0.34f),
            FRotator(0.f, VentIndex % 2 == 0 ? 8.f : -8.f, 0.f), EBlockStyle::Metal, true);
        SpawnDetailCylinder(FVector(VentX, 1850.f, 495.f), FVector(0.26f, 0.26f, 0.38f),
            FRotator::ZeroRotator, EBlockStyle::Dark, true);
    }

    SpawnWorldLabel(FVector(1850.f, 276.f, 300.f), FRotator(0.f, -90.f, 0.f),
        TEXT("WAREHOUSE 01"), FColor(220, 232, 242), 46.f);

    // ---------------------------------------------------------------------
    // WEST CQB BLOCK — stepped roofline, canopy and readable entrance.
    // ---------------------------------------------------------------------
    SpawnDetailBlock(FVector(-3000.f, 2050.f, 342.f), FVector(15.7f, 9.6f, 0.12f),
        FRotator::ZeroRotator, EBlockStyle::Dark, true);
    SpawnDetailBlock(FVector(-3650.f, 2050.f, 402.f), FVector(4.8f, 4.0f, 0.52f),
        FRotator::ZeroRotator, EBlockStyle::Wall, true);
    SpawnDetailBlock(FVector(-2180.f, 2050.f, 375.f), FVector(4.1f, 3.3f, 0.34f),
        FRotator::ZeroRotator, EBlockStyle::ConcreteLight, true);

    // Main entrance canopy on the east face.
    SpawnBlock(FVector(-1320.f, 2050.f, 120.f), FVector(0.13f, 2.2f, 3.0f),
        FRotator::ZeroRotator, true, EBlockStyle::Metal);
    SpawnDetailBlock(FVector(-1180.f, 2050.f, 330.f), FVector(1.6f, 2.55f, 0.14f),
        FRotator::ZeroRotator, EBlockStyle::PaintBlue, true);
    SpawnDetailBlock(FVector(-1310.f, 2050.f, 110.f), FVector(0.055f, 1.5f, 1.75f),
        FRotator::ZeroRotator, EBlockStyle::Dark, false);

    // Visual window modules high on the facade, intentionally non-colliding.
    for (int32 WindowIndex = 0; WindowIndex < 5; ++WindowIndex)
    {
        const float WindowY = 1300.f + static_cast<float>(WindowIndex) * 360.f;
        SpawnDetailBlock(FVector(-1440.f, WindowY, 195.f), FVector(0.045f, 1.25f, 0.50f),
            FRotator::ZeroRotator, EBlockStyle::PaintBlue, false);
    }

    SpawnWorldLabel(FVector(-1412.f, 2050.f, 365.f), FRotator(0.f, 0.f, 0.f),
        TEXT("CQB WEST"), FColor(238, 191, 84), 44.f);

    // ---------------------------------------------------------------------
    // CENTRAL OPERATIONS GANTRY — truss + control booth.
    // ---------------------------------------------------------------------
    SpawnDetailBlock(FVector(0.f, -900.f, 420.f), FVector(9.3f, 0.13f, 0.13f),
        FRotator::ZeroRotator, EBlockStyle::Metal, true);
    SpawnDetailBlock(FVector(-470.f, -900.f, 390.f), FVector(4.2f, 0.08f, 0.08f),
        FRotator(18.f, 0.f, 0.f), EBlockStyle::Accent, true);
    SpawnDetailBlock(FVector(470.f, -900.f, 390.f), FVector(4.2f, 0.08f, 0.08f),
        FRotator(-18.f, 0.f, 0.f), EBlockStyle::Accent, true);

    // Small suspended control booth to stop the gantry reading as only two poles.
    SpawnBlock(FVector(0.f, -1015.f, 310.f), FVector(1.55f, 0.90f, 0.72f),
        FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnDetailBlock(FVector(0.f, -1102.f, 330.f), FVector(1.32f, 0.04f, 0.36f),
        FRotator::ZeroRotator, EBlockStyle::PaintBlue, false);
    SpawnWorldLabel(FVector(0.f, -1108.f, 308.f), FRotator(0.f, -90.f, 0.f),
        TEXT("OPS"), FColor(210, 225, 236), 36.f);

    // ---------------------------------------------------------------------
    // SOUTH SERVICE YARD — pipe rack and heavier infrastructure.
    // ---------------------------------------------------------------------
    const TArray<float> PipeRackX = { -1600.f, -800.f, 0.f, 800.f, 1600.f };
    for (const float RackX : PipeRackX)
    {
        SpawnBlock(FVector(RackX, -3360.f, 90.f), FVector(0.10f, 0.10f, 2.5f),
            FRotator::ZeroRotator, true, EBlockStyle::Metal);
        SpawnBlock(FVector(RackX, -2760.f, 90.f), FVector(0.10f, 0.10f, 2.5f),
            FRotator::ZeroRotator, true, EBlockStyle::Metal);
        SpawnDetailBlock(FVector(RackX, -3060.f, 300.f), FVector(0.10f, 3.1f, 0.10f),
            FRotator::ZeroRotator, EBlockStyle::Metal, true);
    }

    const TArray<float> PipeZ = { 245.f, 285.f, 325.f };
    for (int32 PipeIndex = 0; PipeIndex < PipeZ.Num(); ++PipeIndex)
    {
        SpawnDetailCylinder(FVector(0.f, -3060.f + static_cast<float>(PipeIndex) * 46.f, PipeZ[PipeIndex]),
            FVector(0.065f, 0.065f, 17.0f), FRotator(0.f, 90.f, 0.f),
            PipeIndex == 1 ? EBlockStyle::Accent : EBlockStyle::PaintBlue, true);
    }

    // Transformer / utility cluster.
    for (int32 UnitIndex = 0; UnitIndex < 3; ++UnitIndex)
    {
        const float UnitX = 2500.f + static_cast<float>(UnitIndex) * 280.f;
        SpawnBlock(FVector(UnitX, -2700.f, -25.f), FVector(0.82f, 0.62f, 1.05f),
            FRotator(0.f, 4.f * UnitIndex, 0.f), true, EBlockStyle::Metal);
        SpawnDetailBlock(FVector(UnitX, -2635.f, 15.f), FVector(0.56f, 0.035f, 0.58f),
            FRotator::ZeroRotator, EBlockStyle::Hazard, false);
    }

    SpawnWorldLabel(FVector(2750.f, -2630.f, 140.f), FRotator(0.f, 90.f, 0.f),
        TEXT("HIGH VOLTAGE"), FColor(246, 188, 52), 30.f);

    // ---------------------------------------------------------------------
    // NORTH WATCH TOWER — a new skyline landmark.
    // ---------------------------------------------------------------------
    const FVector TowerBase(0.f, 4650.f, 0.f);
    const TArray<FVector> TowerPosts =
    {
        FVector(-180.f, -180.f, 215.f), FVector(180.f, -180.f, 215.f),
        FVector(-180.f, 180.f, 215.f), FVector(180.f, 180.f, 215.f)
    };
    for (const FVector& Offset : TowerPosts)
    {
        SpawnBlock(TowerBase + Offset, FVector(0.10f, 0.10f, 3.9f),
            FRotator::ZeroRotator, true, EBlockStyle::Metal);
    }
    SpawnBlock(TowerBase + FVector(0.f, 0.f, 420.f), FVector(2.45f, 2.45f, 0.20f),
        FRotator::ZeroRotator, true, EBlockStyle::ConcreteLight);
    SpawnBlock(TowerBase + FVector(0.f, 0.f, 545.f), FVector(2.0f, 2.0f, 1.15f),
        FRotator::ZeroRotator, true, EBlockStyle::Dark);
    SpawnDetailBlock(TowerBase + FVector(0.f, -198.f, 545.f), FVector(1.55f, 0.045f, 0.52f),
        FRotator::ZeroRotator, EBlockStyle::PaintBlue, false);
    SpawnDetailBlock(TowerBase + FVector(0.f, 0.f, 685.f), FVector(2.35f, 2.35f, 0.13f),
        FRotator::ZeroRotator, EBlockStyle::Metal, true);

    // Ladder rungs are decorative and use ISM.
    for (int32 RungIndex = 0; RungIndex < 9; ++RungIndex)
    {
        SpawnDetailBlock(TowerBase + FVector(205.f, 205.f, 60.f + RungIndex * 42.f),
            FVector(0.045f, 0.38f, 0.025f), FRotator::ZeroRotator, EBlockStyle::Hazard, false);
    }
    SpawnWorldLabel(TowerBase + FVector(0.f, -205.f, 610.f), FRotator(0.f, -90.f, 0.f),
        TEXT("TOWER 02"), FColor(210, 228, 238), 32.f);

    // ---------------------------------------------------------------------
    // PERIMETER WALL — break long featureless strips into structural bays.
    // ---------------------------------------------------------------------
    for (int32 Bay = -5; Bay <= 5; ++Bay)
    {
        const float Offset = static_cast<float>(Bay) * 900.f;
        SpawnDetailBlock(FVector(Offset, 5488.f, 155.f), FVector(0.11f, 0.08f, 3.7f),
            FRotator::ZeroRotator, EBlockStyle::ConcreteLight, true);
        SpawnDetailBlock(FVector(Offset, -5488.f, 155.f), FVector(0.11f, 0.08f, 3.7f),
            FRotator::ZeroRotator, EBlockStyle::ConcreteLight, true);
        SpawnDetailBlock(FVector(5488.f, Offset, 155.f), FVector(0.08f, 0.11f, 3.7f),
            FRotator::ZeroRotator, EBlockStyle::ConcreteLight, true);
        SpawnDetailBlock(FVector(-5488.f, Offset, 155.f), FVector(0.08f, 0.11f, 3.7f),
            FRotator::ZeroRotator, EBlockStyle::ConcreteLight, true);
    }

    // ---------------------------------------------------------------------
    // ART FOUNDATION KIT — real imported meshes generated in ArtSource/Industrial.
    // The branch remains playable without them, but once imported they replace the strongest
    // "grey cube" read with distinct silhouettes and proper mesh detail.
    // ---------------------------------------------------------------------
    if (ArtLoadingBay)
    {
        for (int32 BayIndex = 0; BayIndex < WarehouseBayX.Num(); ++BayIndex)
        {
            SpawnImportedArtMesh(ArtLoadingBay, FVector(WarehouseBayX[BayIndex], 250.f, -100.f),
                FRotator::ZeroRotator, FVector::OneVector, false, EBlockStyle::Metal);
        }
    }

    if (ArtContainer20)
    {
        SpawnImportedArtMesh(ArtContainer20, FVector(3150.f, -4300.f, -100.f),
            FRotator(0.f, 7.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
        SpawnImportedArtMesh(ArtContainer20, FVector(-3300.f, -4250.f, -100.f),
            FRotator(0.f, -9.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
        SpawnImportedArtMesh(ArtContainer20, FVector(-3300.f, -4250.f, 159.f),
            FRotator(0.f, -9.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
    }

    if (ArtConcreteBarrier)
    {
        const TArray<FTransform> BarrierTransforms =
        {
            FTransform(FRotator(0.f, 16.f, 0.f), FVector(-720.f, -620.f, -100.f)),
            FTransform(FRotator(0.f, -18.f, 0.f), FVector(740.f, -620.f, -100.f)),
            FTransform(FRotator(0.f, 90.f, 0.f), FVector(1800.f, -1750.f, -100.f)),
            FTransform(FRotator(0.f, 90.f, 0.f), FVector(-1850.f, -1750.f, -100.f))
        };
        for (const FTransform& Transform : BarrierTransforms)
        {
            SpawnImportedArtMesh(ArtConcreteBarrier, Transform.GetLocation(), Transform.Rotator(),
                FVector::OneVector, true, EBlockStyle::Cover);
        }
    }

    if (ArtPipeRack)
    {
        SpawnImportedArtMesh(ArtPipeRack, FVector(-1000.f, -3250.f, -100.f),
            FRotator::ZeroRotator, FVector::OneVector, true, EBlockStyle::Metal);
        SpawnImportedArtMesh(ArtPipeRack, FVector(1000.f, -3250.f, -100.f),
            FRotator::ZeroRotator, FVector::OneVector, true, EBlockStyle::Metal);
    }

    if (ArtFuelTank)
    {
        SpawnImportedArtMesh(ArtFuelTank, FVector(4050.f, 700.f, -100.f),
            FRotator(0.f, 90.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
        SpawnImportedArtMesh(ArtFuelTank, FVector(4500.f, 700.f, -100.f),
            FRotator(0.f, 90.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
    }

    if (ArtHVAC)
    {
        SpawnImportedArtMesh(ArtHVAC, FVector(900.f, 1600.f, 370.f),
            FRotator(0.f, 12.f, 0.f), FVector::OneVector, false, EBlockStyle::Metal);
        SpawnImportedArtMesh(ArtHVAC, FVector(2100.f, 2100.f, 370.f),
            FRotator(0.f, -9.f, 0.f), FVector::OneVector, false, EBlockStyle::Metal);
        SpawnImportedArtMesh(ArtHVAC, FVector(3000.f, 1400.f, 370.f),
            FRotator(0.f, 6.f, 0.f), FVector::OneVector, false, EBlockStyle::Metal);
    }

    if (ArtElectricalCabinet)
    {
        SpawnImportedArtMesh(ArtElectricalCabinet, FVector(4750.f, 4200.f, -100.f),
            FRotator(0.f, -90.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
        SpawnImportedArtMesh(ArtElectricalCabinet, FVector(2500.f, -2650.f, -100.f),
            FRotator(0.f, 180.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
    }

    if (ArtStorageRack)
    {
        SpawnImportedArtMesh(ArtStorageRack, FVector(1750.f, 1000.f, -100.f),
            FRotator(0.f, 90.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
        SpawnImportedArtMesh(ArtStorageRack, FVector(2550.f, 2650.f, -100.f),
            FRotator(0.f, 0.f, 0.f), FVector::OneVector, true, EBlockStyle::Metal);
    }

    if (ArtPallet && ArtIndustrialCrate)
    {
        const TArray<FVector> PalletSpots =
        {
            FVector(1100.f, -2050.f, -100.f), FVector(1260.f, -2050.f, -100.f),
            FVector(-1150.f, 650.f, -100.f), FVector(3050.f, 2900.f, -100.f)
        };
        for (int32 Index = 0; Index < PalletSpots.Num(); ++Index)
        {
            SpawnImportedArtMesh(ArtPallet, PalletSpots[Index], FRotator(0.f, Index * 17.f, 0.f),
                FVector::OneVector, true, EBlockStyle::Wood);
            SpawnImportedArtMesh(ArtIndustrialCrate, PalletSpots[Index] + FVector(0.f, 0.f, 15.f),
                FRotator(0.f, Index * 17.f, 0.f), FVector::OneVector, true, EBlockStyle::Wood);
        }
    }

    if (ArtLampPost)
    {
        const TArray<FVector> ArtLampPositions =
        {
            FVector(-4400.f, -3400.f, -100.f), FVector(4400.f, -3400.f, -100.f),
            FVector(-4400.f, 3300.f, -100.f), FVector(4400.f, 3300.f, -100.f)
        };
        for (const FVector& Position : ArtLampPositions)
        {
            SpawnImportedArtMesh(ArtLampPost, Position, FRotator::ZeroRotator,
                FVector::OneVector, false, EBlockStyle::Metal);
        }
    }
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
