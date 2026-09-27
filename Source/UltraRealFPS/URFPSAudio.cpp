#include "URFPSAudio.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundGroups.h"

namespace
{
    constexpr int32 SampleRate = 48000;
    constexpr float TwoPi = 2.f * PI;

    struct FGeneratedSound
    {
        TArray<int16> Samples;
        float Duration = 0.1f;
        float InnerRadius = 120.f;
        float MaxDistance = 2200.f;
        bool bOcclusion = true;
    };

    float NoiseSample()
    {
        return FMath::FRandRange(-1.f, 1.f);
    }

    float SoftClip(float Value)
    {
        // Fast saturation that preserves transients without hard digital clipping.
        return Value / (1.f + FMath::Abs(Value));
    }

    void AddSample(FGeneratedSound& Out, int32 Index, float Value)
    {
        if (!Out.Samples.IsValidIndex(Index)) return;
        const float Existing = static_cast<float>(Out.Samples[Index]) / 32767.f;
        const float Mixed = SoftClip(Existing + Value);
        Out.Samples[Index] = static_cast<int16>(FMath::Clamp(Mixed, -1.f, 1.f) * 32767.f);
    }

    FGeneratedSound Generate(EURFPSAudioEvent Event)
    {
        FGeneratedSound Result;

        switch (Event)
        {
        case EURFPSAudioEvent::RifleShot:
        case EURFPSAudioEvent::EnemyRifleShot:
            Result.Duration = 0.34f;
            Result.InnerRadius = 180.f;
            Result.MaxDistance = 16000.f;
            break;
        case EURFPSAudioEvent::RifleIndoorTail:
            Result.Duration = 0.62f;
            Result.InnerRadius = 180.f;
            Result.MaxDistance = 9500.f;
            break;
        case EURFPSAudioEvent::RifleOutdoorTail:
            Result.Duration = 0.78f;
            Result.InnerRadius = 220.f;
            Result.MaxDistance = 17000.f;
            break;
        case EURFPSAudioEvent::BulletCrack:
            Result.Duration = 0.10f;
            Result.InnerRadius = 75.f;
            Result.MaxDistance = 1800.f;
            break;
        case EURFPSAudioEvent::GrenadeExplosion:
            Result.Duration = 0.90f;
            Result.InnerRadius = 260.f;
            Result.MaxDistance = 19000.f;
            break;
        case EURFPSAudioEvent::DoorOpen:
        case EURFPSAudioEvent::DoorClose:
            Result.Duration = 0.36f;
            Result.InnerRadius = 90.f;
            Result.MaxDistance = 1700.f;
            break;
        case EURFPSAudioEvent::ShellClink:
            Result.Duration = 0.11f;
            Result.InnerRadius = 35.f;
            Result.MaxDistance = 650.f;
            break;
        case EURFPSAudioEvent::ReloadStart:
        case EURFPSAudioEvent::ReloadEnd:
        case EURFPSAudioEvent::DryFire:
            Result.Duration = 0.16f;
            Result.InnerRadius = 55.f;
            Result.MaxDistance = 700.f;
            Result.bOcclusion = false;
            break;
        case EURFPSAudioEvent::FootstepConcrete:
        case EURFPSAudioEvent::FootstepMetal:
        case EURFPSAudioEvent::FootstepWood:
            Result.Duration = 0.15f;
            Result.InnerRadius = 70.f;
            Result.MaxDistance = 1250.f;
            break;
        default:
            Result.Duration = 0.12f;
            Result.InnerRadius = 45.f;
            Result.MaxDistance = 1300.f;
            break;
        }

        const int32 NumSamples = FMath::Max(1, FMath::CeilToInt(Result.Duration * static_cast<float>(SampleRate)));
        Result.Samples.Init(0, NumSamples);

        // Two simple noise integrators give the procedural placeholders low/mid-frequency
        // energy without the obvious "sine beep" character of the first prototype.
        float LowNoise = 0.f;
        float MidNoise = 0.f;

        for (int32 Index = 0; Index < NumSamples; ++Index)
        {
            const float T = static_cast<float>(Index) / static_cast<float>(SampleRate);
            float Value = 0.f;

            switch (Event)
            {
            case EURFPSAudioEvent::RifleShot:
            case EURFPSAudioEvent::EnemyRifleShot:
            {
                const float RawNoise = NoiseSample();
                LowNoise = FMath::Lerp(LowNoise, RawNoise, 0.022f);
                MidNoise = FMath::Lerp(MidNoise, RawNoise, 0.16f);

                const float MuzzleBlast = RawNoise * FMath::Exp(-T * 118.f) * 1.34f;
                const float Pressure = (MidNoise * 0.82f + LowNoise * 1.18f) * FMath::Exp(-T * 15.5f);
                const float LowBody = (LowNoise * 1.32f + FMath::Sin(TwoPi * 67.f * T) * 0.18f)
                    * FMath::Exp(-T * 7.4f);
                const float ActionEnvelope = FMath::Exp(-FMath::Square((T - 0.020f) / 0.0085f));
                const float Mechanical = NoiseSample() * ActionEnvelope * 0.23f;
                const float ShortTail = MidNoise * FMath::Exp(-T * 8.8f) * 0.20f;

                Value = MuzzleBlast + Pressure * 0.78f + LowBody * 0.46f + Mechanical + ShortTail;
                if (Event == EURFPSAudioEvent::EnemyRifleShot)
                {
                    Value *= 0.86f;
                }
                break;
            }
            case EURFPSAudioEvent::RifleIndoorTail:
            {
                const float LocalT = FMath::Max(0.f, T - 0.026f);
                if (T >= 0.026f)
                {
                    const float RawNoise = NoiseSample();
                    LowNoise = FMath::Lerp(LowNoise, RawNoise, 0.018f);
                    MidNoise = FMath::Lerp(MidNoise, RawNoise, 0.10f);
                    const float Early = MidNoise * FMath::Exp(-LocalT * 11.0f) * 0.48f;
                    const float Room = LowNoise * FMath::Exp(-LocalT * 5.8f) * 0.42f;
                    const float Slap = NoiseSample() * FMath::Exp(-FMath::Square((LocalT - 0.055f) / 0.026f)) * 0.17f;
                    Value = Early + Room + Slap;
                }
                break;
            }
            case EURFPSAudioEvent::RifleOutdoorTail:
            {
                const float LocalT = FMath::Max(0.f, T - 0.065f);
                if (T >= 0.065f)
                {
                    const float RawNoise = NoiseSample();
                    LowNoise = FMath::Lerp(LowNoise, RawNoise, 0.014f);
                    MidNoise = FMath::Lerp(MidNoise, RawNoise, 0.075f);
                    const float DistantPressure = LowNoise * FMath::Exp(-LocalT * 3.6f) * 0.34f;
                    const float TerrainReturn = MidNoise * FMath::Exp(-LocalT * 4.7f) * 0.19f;
                    const float LateEcho = NoiseSample() * FMath::Exp(-FMath::Square((LocalT - 0.18f) / 0.055f)) * 0.08f;
                    Value = DistantPressure + TerrainReturn + LateEcho;
                }
                break;
            }
            case EURFPSAudioEvent::BulletCrack:
            {
                const float Snap = NoiseSample() * FMath::Exp(-T * 115.f) * 0.95f;
                const float Whip = FMath::Sin(TwoPi * (2150.f - T * 7400.f) * T) * FMath::Exp(-T * 54.f) * 0.38f;
                Value = Snap + Whip;
                break;
            }
            case EURFPSAudioEvent::GrenadeExplosion:
            {
                const float Initial = NoiseSample() * FMath::Exp(-T * 28.f) * 1.15f;
                const float Boom = FMath::Sin(TwoPi * (62.f - T * 18.f) * T) * FMath::Exp(-T * 4.8f) * 0.95f;
                const float Rumble = NoiseSample() * FMath::Exp(-T * 4.1f) * 0.26f;
                Value = Initial + Boom + Rumble;
                break;
            }
            case EURFPSAudioEvent::FootstepConcrete:
            {
                Value = FMath::Sin(TwoPi * 78.f * T) * FMath::Exp(-T * 29.f) * 0.68f
                    + NoiseSample() * FMath::Exp(-T * 37.f) * 0.34f;
                break;
            }
            case EURFPSAudioEvent::FootstepMetal:
            {
                Value = FMath::Sin(TwoPi * 165.f * T) * FMath::Exp(-T * 22.f) * 0.42f
                    + FMath::Sin(TwoPi * 760.f * T) * FMath::Exp(-T * 29.f) * 0.25f
                    + NoiseSample() * FMath::Exp(-T * 44.f) * 0.23f;
                break;
            }
            case EURFPSAudioEvent::FootstepWood:
            {
                Value = FMath::Sin(TwoPi * 94.f * T) * FMath::Exp(-T * 25.f) * 0.58f
                    + FMath::Sin(TwoPi * 285.f * T) * FMath::Exp(-T * 38.f) * 0.18f
                    + NoiseSample() * FMath::Exp(-T * 42.f) * 0.18f;
                break;
            }
            case EURFPSAudioEvent::ImpactMetal:
            {
                Value = FMath::Sin(TwoPi * 1720.f * T) * FMath::Exp(-T * 34.f) * 0.48f
                    + FMath::Sin(TwoPi * 2760.f * T) * FMath::Exp(-T * 46.f) * 0.26f
                    + NoiseSample() * FMath::Exp(-T * 75.f) * 0.30f;
                break;
            }
            case EURFPSAudioEvent::ImpactWood:
            {
                Value = FMath::Sin(TwoPi * 115.f * T) * FMath::Exp(-T * 37.f) * 0.62f
                    + NoiseSample() * FMath::Exp(-T * 58.f) * 0.22f;
                break;
            }
            case EURFPSAudioEvent::ImpactFlesh:
            {
                Value = FMath::Sin(TwoPi * 72.f * T) * FMath::Exp(-T * 43.f) * 0.48f
                    + NoiseSample() * FMath::Exp(-T * 63.f) * 0.20f;
                break;
            }
            case EURFPSAudioEvent::ImpactConcrete:
            {
                Value = NoiseSample() * FMath::Exp(-T * 68.f) * 0.56f
                    + FMath::Sin(TwoPi * 190.f * T) * FMath::Exp(-T * 43.f) * 0.24f;
                break;
            }
            case EURFPSAudioEvent::ReloadStart:
            {
                const float ClickA = FMath::Exp(-FMath::Square((T - 0.022f) / 0.010f)) * 0.62f;
                const float ClickB = FMath::Exp(-FMath::Square((T - 0.083f) / 0.013f)) * 0.42f;
                Value = NoiseSample() * (ClickA + ClickB) + FMath::Sin(TwoPi * 620.f * T) * ClickB * 0.20f;
                break;
            }
            case EURFPSAudioEvent::ReloadEnd:
            {
                const float Click = FMath::Exp(-FMath::Square((T - 0.030f) / 0.012f)) * 0.82f;
                const float Clack = FMath::Exp(-FMath::Square((T - 0.086f) / 0.016f)) * 0.60f;
                Value = NoiseSample() * (Click + Clack) + FMath::Sin(TwoPi * 410.f * T) * Clack * 0.30f;
                break;
            }
            case EURFPSAudioEvent::DryFire:
            {
                const float Click = FMath::Exp(-FMath::Square((T - 0.026f) / 0.010f));
                Value = NoiseSample() * Click * 0.52f + FMath::Sin(TwoPi * 1050.f * T) * Click * 0.16f;
                break;
            }
            case EURFPSAudioEvent::ShellClink:
            {
                const float Ping = FMath::Sin(TwoPi * 2650.f * T) * FMath::Exp(-T * 43.f) * 0.44f;
                const float Ring = FMath::Sin(TwoPi * 1680.f * T) * FMath::Exp(-T * 28.f) * 0.22f;
                const float Tick = NoiseSample() * FMath::Exp(-T * 95.f) * 0.34f;
                Value = Ping + Ring + Tick;
                break;
            }
            case EURFPSAudioEvent::DoorOpen:
            case EURFPSAudioEvent::DoorClose:
            {
                const float DirectionScale = Event == EURFPSAudioEvent::DoorClose ? 1.15f : 0.88f;
                const float Creak = FMath::Sin(TwoPi * (118.f + 38.f * FMath::Sin(T * 12.f)) * T)
                    * FMath::Exp(-T * 5.8f) * 0.34f;
                const float Scrape = NoiseSample() * FMath::Exp(-T * 6.5f) * 0.14f;
                const float Latch = FMath::Exp(-FMath::Square((T - (Event == EURFPSAudioEvent::DoorClose ? 0.28f : 0.07f)) / 0.018f))
                    * NoiseSample() * 0.58f;
                Value = (Creak + Scrape + Latch) * DirectionScale;
                break;
            }
            default:
                break;
            }

            AddSample(Result, Index, Value * 0.82f);
        }

        return Result;
    }


    const FGeneratedSound& GetGeneratedTemplate(EURFPSAudioEvent Event, int32 Variant)
    {
        // Cache a small variation bank per event. Automatic fire no longer replays one identical
        // PCM buffer every shot, while avoiding expensive procedural synthesis during combat.
        static TMap<uint16, FGeneratedSound> Cache;
        const uint16 EventKey = static_cast<uint16>(static_cast<uint8>(Event));
        const uint16 VariantKey = static_cast<uint16>(FMath::Clamp(Variant, 0, 3));
        const uint16 Key = static_cast<uint16>(EventKey * 4u + VariantKey);
        if (FGeneratedSound* Found = Cache.Find(Key))
        {
            return *Found;
        }

        Cache.Add(Key, Generate(Event));
        return *Cache.Find(Key);
    }

    USoundWaveProcedural* BuildWave(UObject* Outer, const FGeneratedSound& Generated)
    {
        if (!Outer) Outer = GetTransientPackage();

        USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
        if (!Wave) return nullptr;

        Wave->NumChannels = 1;
        Wave->SetSampleRate(SampleRate, false);
        Wave->Duration = Generated.Duration;
        Wave->bLooping = false;
        Wave->bCanProcessAsync = true;
        Wave->SoundGroup = SOUNDGROUP_Effects;
        Wave->SampleByteSize = sizeof(int16);
        Wave->QueueAudio(reinterpret_cast<const uint8*>(Generated.Samples.GetData()), Generated.Samples.Num() * sizeof(int16));
        return Wave;
    }

    void ConfigureAttenuation(USoundAttenuation* Attenuation, const FGeneratedSound& Generated)
    {
        if (!Attenuation) return;
        FSoundAttenuationSettings& Settings = Attenuation->Attenuation;
        Settings.bAttenuate = true;
        Settings.bSpatialize = true;
        Settings.AttenuationShape = EAttenuationShape::Sphere;
        Settings.AttenuationShapeExtents = FVector(Generated.InnerRadius, 0.f, 0.f);
        Settings.FalloffDistance = FMath::Max(100.f, Generated.MaxDistance - Generated.InnerRadius);
        Settings.bEnableOcclusion = Generated.bOcclusion;
        Settings.OcclusionTraceChannel = ECC_Visibility;
        Settings.OcclusionLowPassFilterFrequency = 2100.f;
        Settings.OcclusionVolumeAttenuation = 0.44f;
        Settings.OcclusionInterpolationTime = 0.11f;
        Settings.bAttenuateWithLPF = true;
        Settings.LPFRadiusMin = Generated.InnerRadius * 2.0f;
        Settings.LPFRadiusMax = Generated.MaxDistance;
        Settings.LPFFrequencyAtMin = 20000.f;
        Settings.LPFFrequencyAtMax = 4800.f;
    }
}

void URFPSAudio::PlaySpatial(UObject* WorldContextObject, EURFPSAudioEvent Event, const FVector& Location,
    float VolumeMultiplier, float PitchMultiplier)
{
    if (!WorldContextObject) return;

    const FGeneratedSound& Generated = GetGeneratedTemplate(Event, FMath::RandRange(0, 3));
    USoundWaveProcedural* Wave = BuildWave(WorldContextObject, Generated);
    if (!Wave) return;

    USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(Wave);
    ConfigureAttenuation(Attenuation, Generated);

    UGameplayStatics::SpawnSoundAtLocation(
        WorldContextObject,
        Wave,
        Location,
        FRotator::ZeroRotator,
        FMath::Max(0.f, VolumeMultiplier),
        FMath::Clamp(PitchMultiplier, 0.65f, 1.55f),
        0.f,
        Attenuation,
        nullptr,
        true);
}

void URFPSAudio::PlayGunshot(UObject* WorldContextObject, const FVector& Location, bool bEnemyShot,
    float VolumeMultiplier, float PitchMultiplier)
{
    if (!WorldContextObject) return;

    PlaySpatial(WorldContextObject, bEnemyShot ? EURFPSAudioEvent::EnemyRifleShot : EURFPSAudioEvent::RifleShot,
        Location, VolumeMultiplier, PitchMultiplier);

    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return;

    const FVector ProbeDirections[] =
    {
        FVector(1.f, 0.f, 0.f), FVector(-1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), FVector(0.f, -1.f, 0.f),
        FVector(0.f, 0.f, 1.f), FVector(1.f, 1.f, 0.35f).GetSafeNormal(), FVector(-1.f, 1.f, 0.35f).GetSafeNormal()
    };

    int32 NearbySurfaces = 0;
    float NormalizedDistanceSum = 0.f;
    constexpr float ProbeDistance = 1250.f;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GunshotAcousticProbe), false);
    if (const AActor* ActorContext = Cast<AActor>(WorldContextObject)) Params.AddIgnoredActor(ActorContext);

    for (const FVector& Direction : ProbeDirections)
    {
        FHitResult Hit;
        if (World->LineTraceSingleByChannel(Hit, Location, Location + Direction * ProbeDistance, ECC_Visibility, Params))
        {
            ++NearbySurfaces;
            NormalizedDistanceSum += FMath::Clamp(Hit.Distance / ProbeDistance, 0.f, 1.f);
        }
    }

    const bool bEnclosed = NearbySurfaces >= 4 || (NearbySurfaces >= 3 && NormalizedDistanceSum / FMath::Max(1, NearbySurfaces) < 0.58f);
    const EURFPSAudioEvent TailEvent = bEnclosed ? EURFPSAudioEvent::RifleIndoorTail : EURFPSAudioEvent::RifleOutdoorTail;
    const float TailVolume = VolumeMultiplier * (bEnclosed ? 0.78f : 0.48f);
    PlaySpatial(WorldContextObject, TailEvent, Location, TailVolume, FMath::FRandRange(0.96f, 1.04f));
}

void URFPSAudio::PlayLocal(UObject* WorldContextObject, EURFPSAudioEvent Event,
    float VolumeMultiplier, float PitchMultiplier)
{
    if (!WorldContextObject) return;
    const FGeneratedSound& Generated = GetGeneratedTemplate(Event, FMath::RandRange(0, 3));
    USoundWaveProcedural* Wave = BuildWave(WorldContextObject, Generated);
    if (!Wave) return;

    UGameplayStatics::PlaySound2D(
        WorldContextObject,
        Wave,
        FMath::Max(0.f, VolumeMultiplier),
        FMath::Clamp(PitchMultiplier, 0.65f, 1.55f),
        0.f,
        nullptr,
        nullptr,
        false);
}
