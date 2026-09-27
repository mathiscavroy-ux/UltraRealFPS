#pragma once

#include "CoreMinimal.h"

enum class EURFPSAudioEvent : uint8
{
    RifleShot,
    EnemyRifleShot,
    RifleIndoorTail,
    RifleOutdoorTail,
    BulletCrack,
    ReloadStart,
    ReloadEnd,
    DryFire,
    FootstepConcrete,
    FootstepMetal,
    FootstepWood,
    ImpactConcrete,
    ImpactMetal,
    ImpactWood,
    ImpactFlesh,
    GrenadeExplosion,
    DoorOpen,
    DoorClose,
    ShellClink
};

namespace URFPSAudio
{
    /**
     * Generates a short procedural mono sound and plays it as a spatialized source.
     * No external audio assets are required. This is deliberately a prototype-quality
     * acoustic layer that can later be replaced by recorded weapon/foley assets without
     * changing gameplay code.
     */
    void PlaySpatial(UObject* WorldContextObject, EURFPSAudioEvent Event, const FVector& Location,
        float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f);

    /** Plays the direct shot plus an environment-dependent reflection tail. */
    void PlayGunshot(UObject* WorldContextObject, const FVector& Location, bool bEnemyShot,
        float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f);

    /** Plays a procedural sound without world attenuation (used sparingly for local weapon handling). */
    void PlayLocal(UObject* WorldContextObject, EURFPSAudioEvent Event,
        float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f);
}
