#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "URFPSNavGrid.h"
#include "URFPSGameMode.generated.h"

class UStaticMesh;
class AStaticMeshActor;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPhysicalMaterial;
class UInstancedStaticMeshComponent;
class AURFPSEnemy;
class AURFPSDoor;

UCLASS()
class ULTRAREALFPS_API AURFPSGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AURFPSGameMode();
    virtual void StartPlay() override;

    void NotifyEnemyKilled();
    void RequestNextWave();
    bool TryAcquireFireSlot(AURFPSEnemy* Enemy);
    void ReleaseFireSlot(AURFPSEnemy* Enemy);
    int32 GetCurrentWave() const { return CurrentWave; }
    int32 GetEnemiesAlive() const { return EnemiesAlive; }
    int32 GetTotalKills() const { return TotalKills; }
    int32 GetActiveShooters() const;
    int32 GetMaxActiveShooters() const;
    int32 GetHuntingEnemies() const;
    bool IsWaveCleared() const { return bWaveCleared; }
    float GetNextWaveTimeRemaining() const;
    float GetCurrentWaveElapsed() const;
    float GetWaveIntroAlpha() const;
    FString GetWaveName() const;

    // Enemy navigation over the procedural compound (see URFPSNavGrid).
    bool IsNavigationReady() const { return NavGrid.IsReady(); }
    bool FindNavPath(const FVector& From, const FVector& To, TArray<FVector>& OutWaypoints) const;
    bool FindNavPointNear(const FVector& Origin, const FVector& Center, float Radius, FVector& OutLocation) const;
    bool HasNavLine(const FVector& From, const FVector& To) const;

private:
    enum class EBlockStyle : uint8
    {
        Floor,
        Wall,
        Cover,
        Wood,
        Metal,
        Dark,
        Asphalt,
        ConcreteLight,
        PaintBlue,
        Accent,
        Hazard,
        SupplyAmmo,
        SupplyMedical,
        SupplyGrenade
    };

    void DestroyLegacyLighting();
    void BuildLighting();
    void CreateMaterials();
    void BuildArena();
    void BuildIndustrialArchitecturePass();
    void LoadImportedArtAssets();
    AStaticMeshActor* SpawnImportedArtMesh(UStaticMesh* MeshAsset, const FVector& Location,
        const FRotator& Rotation = FRotator::ZeroRotator, const FVector& Scale = FVector::OneVector,
        bool bCollision = true, EBlockStyle SurfaceStyle = EBlockStyle::Metal);
    void SpawnWorldLabel(const FVector& Location, const FRotator& Rotation, const FString& Label,
        const FColor& Color, float WorldSize = 42.f);
    void SpawnBlock(const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator,
        bool bCastShadow = true, EBlockStyle Style = EBlockStyle::Wall);
    AStaticMeshActor* SpawnTaggedBlock(const FVector& Location, const FVector& Scale, FName Tag,
        EBlockStyle Style = EBlockStyle::Accent);
    void SpawnCylinder(const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator,
        EBlockStyle Style = EBlockStyle::Cover);
    void SpawnDetailBlock(const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator,
        EBlockStyle Style = EBlockStyle::Dark, bool bCastShadow = false);
    void SpawnDetailCylinder(const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator,
        EBlockStyle Style = EBlockStyle::Metal, bool bCastShadow = false);
    UInstancedStaticMeshComponent* GetOrCreateDetailISM(EBlockStyle Style, bool bCylinder, bool bCastShadow);
    AURFPSDoor* SpawnDoor(const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
    UMaterialInterface* GetMaterialForStyle(EBlockStyle Style) const;
    UPhysicalMaterial* GetPhysicalMaterialForStyle(EBlockStyle Style) const;
    void SpawnSupplies();
    void SpawnWave();
    void StartNextWave();
    bool IsSpawnVisibleToPlayer(const FVector& SpawnLocation) const;
    void CleanupFireSlots();
    void RegisterNavBox(const FVector& Location, const FVector& HalfExtents, const FRotator& Rotation);
    void BuildNavigation();
    void OrderSurvivorsToHunt();

    UPROPERTY() UStaticMesh* CubeMesh = nullptr;
    UPROPERTY() UStaticMesh* CylinderMesh = nullptr;

    // Art Foundation kit. These assets are optional at compile time: the procedural blockout
    // remains a fallback until IMPORT_ART_ASSETS.bat has generated the .uasset files.
    UPROPERTY() UStaticMesh* ArtContainer20 = nullptr;
    UPROPERTY() UStaticMesh* ArtConcreteBarrier = nullptr;
    UPROPERTY() UStaticMesh* ArtIndustrialCrate = nullptr;
    UPROPERTY() UStaticMesh* ArtPallet = nullptr;
    UPROPERTY() UStaticMesh* ArtElectricalCabinet = nullptr;
    UPROPERTY() UStaticMesh* ArtHVAC = nullptr;
    UPROPERTY() UStaticMesh* ArtPipeRack = nullptr;
    UPROPERTY() UStaticMesh* ArtLoadingBay = nullptr;
    UPROPERTY() UStaticMesh* ArtLampPost = nullptr;
    UPROPERTY() UStaticMesh* ArtFuelTank = nullptr;
    UPROPERTY() UStaticMesh* ArtStorageRack = nullptr;
    UPROPERTY() UMaterialInterface* BaseShapeMaterial = nullptr;
    // Generated by Tools/build_environment_materials.py; optional (palette fallback without it).
    UPROPERTY() UMaterialInterface* EnvironmentMasterMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* FloorMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* WallMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* CoverMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* WoodMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* MetalMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* DarkMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* AsphaltMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* ConcreteLightMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* PaintBlueMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* AccentMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* HazardMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* AmmoMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* MedicalMaterial = nullptr;
    UPROPERTY() UMaterialInstanceDynamic* GrenadeMaterial = nullptr;
    UPROPERTY() UPhysicalMaterial* ConcretePhysicalMaterial = nullptr;
    UPROPERTY() UPhysicalMaterial* MetalPhysicalMaterial = nullptr;
    UPROPERTY() UPhysicalMaterial* WoodPhysicalMaterial = nullptr;
    UPROPERTY(Transient) TArray<UInstancedStaticMeshComponent*> DetailISMComponents;
    TMap<uint32, UInstancedStaticMeshComponent*> DetailISMCache;

    int32 CurrentWave = 1;
    int32 EnemiesAlive = 0;
    int32 TotalKills = 0;
    bool bWaveCleared = false;
    float TimeBetweenWaves = 12.f;
    float WaveStartWorldTime = 0.f;
    TArray<TWeakObjectPtr<AURFPSEnemy>> ActiveShooters;
    FTimerHandle NextWaveTimerHandle;
    FURFPSNavGrid NavGrid;
};
