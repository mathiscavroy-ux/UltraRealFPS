#include "URFPSShellCasing.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "URFPSAudio.h"

AURFPSShellCasing::AURFPSShellCasing()
{
    PrimaryActorTick.bCanEverTick = false;
    SetLifeSpan(2.8f);

    CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
    SetRootComponent(CasingMesh);
    CasingMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    CasingMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    CasingMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    CasingMesh->SetSimulatePhysics(true);
    CasingMesh->SetEnableGravity(true);
    CasingMesh->SetLinearDamping(0.08f);
    CasingMesh->SetAngularDamping(0.12f);
    CasingMesh->SetRelativeScale3D(FVector(0.024f, 0.024f, 0.070f));
    CasingMesh->SetCastShadow(false);
    CasingMesh->SetNotifyRigidBodyCollision(true);
    CasingMesh->OnComponentHit.AddDynamic(this, &AURFPSShellCasing::OnCasingHit);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderAsset.Succeeded()) CasingMesh->SetStaticMesh(CylinderAsset.Object);
}

void AURFPSShellCasing::BeginPlay()
{
    Super::BeginPlay();

    if (UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(ParentMaterial, this))
        {
            Material->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.55f, 0.37f, 0.10f, 1.f));
            CasingMesh->SetMaterial(0, Material);
        }
    }
}

void AURFPSShellCasing::Eject(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
    if (!CasingMesh) return;
    CasingMesh->SetPhysicsLinearVelocity(LinearVelocity, false, NAME_None);
    CasingMesh->SetPhysicsAngularVelocityInDegrees(AngularVelocity, false, NAME_None);
}
void AURFPSShellCasing::OnCasingHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bPlayedImpact || NormalImpulse.Size() < 25.f) return;
    bPlayedImpact = true;

    const float Volume = FMath::GetMappedRangeValueClamped(FVector2D(25.f, 260.f), FVector2D(0.18f, 0.48f), NormalImpulse.Size());

    // FHitResult::ImpactPoint is FVector_NetQuantize in UE 5.8.
    // Avoid mixing it with FVector in the conditional operator: MSVC 14.51
    // cannot choose a common type for those two operands. Assign explicitly instead.
    FVector Location = GetActorLocation();
    if (!Hit.ImpactPoint.IsNearlyZero())
    {
        Location.X = Hit.ImpactPoint.X;
        Location.Y = Hit.ImpactPoint.Y;
        Location.Z = Hit.ImpactPoint.Z;
    }

    URFPSAudio::PlaySpatial(this, EURFPSAudioEvent::ShellClink, Location, Volume, FMath::FRandRange(0.90f, 1.13f));
}

