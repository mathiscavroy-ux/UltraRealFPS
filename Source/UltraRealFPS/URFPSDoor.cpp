#include "URFPSDoor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"
#include "URFPSAudio.h"

AURFPSDoor::AURFPSDoor()
{
    PrimaryActorTick.bCanEverTick = true;

    HingeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HingeRoot"));
    SetRootComponent(HingeRoot);

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(HingeRoot);
    DoorMesh->SetRelativeLocation(FVector(0.f, 55.f, 105.f));
    DoorMesh->SetRelativeScale3D(FVector(0.075f, 1.10f, 2.10f));
    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    DoorMesh->SetGenerateOverlapEvents(false);
    DoorMesh->SetMobility(EComponentMobility::Movable);

    HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
    HandleMesh->SetupAttachment(HingeRoot);
    HandleMesh->SetRelativeLocation(FVector(-9.f, 94.f, 104.f));
    HandleMesh->SetRelativeScale3D(FVector(0.08f, 0.14f, 0.06f));
    HandleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HandleMesh->SetMobility(EComponentMobility::Movable);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        DoorMesh->SetStaticMesh(CubeAsset.Object);
        HandleMesh->SetStaticMesh(CubeAsset.Object);
    }

    Tags.Add(FName(TEXT("Door")));
}

void AURFPSDoor::BeginPlay()
{
    Super::BeginPlay();

    ClosedYaw = GetActorRotation().Yaw;
    TargetYaw = ClosedYaw;

    DoorPhysicalMaterial = NewObject<UPhysicalMaterial>(this, FName(TEXT("PM_Runtime_DoorMetal")));
    if (DoorPhysicalMaterial)
    {
        DoorPhysicalMaterial->SurfaceType = SurfaceType2;
        DoorMesh->SetPhysMaterialOverride(DoorPhysicalMaterial);
    }

    if (UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        DoorMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        HandleMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);

        if (DoorMaterial)
        {
            DoorMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.095f, 0.11f, 0.105f, 1.f));
            DoorMesh->SetMaterial(0, DoorMaterial);
        }
        if (HandleMaterial)
        {
            HandleMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.18f, 0.19f, 0.18f, 1.f));
            HandleMesh->SetMaterial(0, HandleMaterial);
        }
    }
}

void AURFPSDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bMoving) return;

    FRotator Rotation = GetActorRotation();
    Rotation.Yaw = FMath::FInterpConstantTo(Rotation.Yaw, TargetYaw, DeltaSeconds, AngularSpeed);
    SetActorRotation(Rotation);

    if (FMath::Abs(FMath::FindDeltaAngleDegrees(Rotation.Yaw, TargetYaw)) < 0.35f)
    {
        Rotation.Yaw = TargetYaw;
        SetActorRotation(Rotation);
        bMoving = false;
    }
}

void AURFPSDoor::Interact(AActor* Interactor)
{
    if (!Interactor) return;

    if (!bOpen)
    {
        FVector ToInteractor = Interactor->GetActorLocation() - GetActorLocation();
        ToInteractor.Z = 0.f;
        const float Side = FVector::DotProduct(ToInteractor.GetSafeNormal(), GetActorForwardVector());
        // The leaf extends along local +Y from the hinge. A positive yaw swings it toward the
        // door's back side, so a player standing in front (Side >= 0) needs +1 for the leaf to
        // move away from them. The sign was inverted and every door opened into the player.
        OpenDirection = Side >= 0.f ? 1.f : -1.f;
        bOpen = true;
        TargetYaw = ClosedYaw + OpenDirection * OpenAngle;
        URFPSAudio::PlaySpatial(this, EURFPSAudioEvent::DoorOpen, GetActorLocation() + FVector(0.f, 0.f, 90.f), 0.72f, FMath::FRandRange(0.95f, 1.05f));
    }
    else
    {
        bOpen = false;
        TargetYaw = ClosedYaw;
        URFPSAudio::PlaySpatial(this, EURFPSAudioEvent::DoorClose, GetActorLocation() + FVector(0.f, 0.f, 90.f), 0.82f, FMath::FRandRange(0.96f, 1.04f));
    }

    bMoving = true;
}

FString AURFPSDoor::GetInteractionText() const
{
    return bOpen ? TEXT("E  FERMER LA PORTE") : TEXT("E  OUVRIR LA PORTE");
}
