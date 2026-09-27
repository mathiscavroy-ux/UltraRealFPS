#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "URFPSShellCasing.generated.h"

class UStaticMeshComponent;
class UPrimitiveComponent;

UCLASS()
class ULTRAREALFPS_API AURFPSShellCasing : public AActor
{
    GENERATED_BODY()

public:
    AURFPSShellCasing();
    void Eject(const FVector& LinearVelocity, const FVector& AngularVelocity);

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnCasingHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse, const FHitResult& Hit);

    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CasingMesh;
    bool bPlayedImpact = false;
};
