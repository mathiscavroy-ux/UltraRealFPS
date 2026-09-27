#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "URFPSDoor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPhysicalMaterial;

UCLASS()
class ULTRAREALFPS_API AURFPSDoor : public AActor
{
    GENERATED_BODY()

public:
    AURFPSDoor();

    virtual void Tick(float DeltaSeconds) override;

    void Interact(AActor* Interactor);
    FString GetInteractionText() const;
    bool IsOpen() const { return bOpen; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere) USceneComponent* HingeRoot;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* DoorMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* HandleMesh;
    UPROPERTY() UMaterialInstanceDynamic* DoorMaterial;
    UPROPERTY() UMaterialInstanceDynamic* HandleMaterial;
    UPROPERTY() UPhysicalMaterial* DoorPhysicalMaterial;

    bool bOpen = false;
    bool bMoving = false;
    float ClosedYaw = 0.f;
    float TargetYaw = 0.f;
    float OpenAngle = 96.f;
    float OpenDirection = 1.f;
    float AngularSpeed = 220.f;
};
