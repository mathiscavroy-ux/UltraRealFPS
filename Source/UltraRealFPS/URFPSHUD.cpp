#include "URFPSHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "URFPSCharacter.h"
#include "URFPSGameMode.h"

void AURFPSHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GetOwningPawn()) return;

    AURFPSCharacter* Player = Cast<AURFPSCharacter>(GetOwningPawn());
    if (!Player) return;

    UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
    if (!Font) return;

    const float ScreenW = Canvas->ClipX;
    const float ScreenH = Canvas->ClipY;
    const float CenterX = ScreenW * 0.5f;
    const float CenterY = ScreenH * 0.5f;
    const float Obstruction = Player->GetWeaponObstructionAlpha();

    const bool bShowCrosshair = !Player->IsAiming() && !Player->IsDead() && !Player->IsLowReady() && !Player->IsTreating();
    if (bShowCrosshair)
    {
        const float Spread = Player->GetCurrentSpreadDegrees();
        const float Gap = 5.5f + FMath::Clamp(Spread * 3.7f, 0.f, 11.f);
        const float Len = 6.5f;
        const FLinearColor CrosshairColor = Obstruction > 0.78f
            ? FLinearColor(0.95f, 0.36f, 0.18f, 0.76f)
            : FLinearColor(0.94f, 0.96f, 0.94f, 0.68f);

        DrawLine(CenterX - Gap - Len, CenterY, CenterX - Gap, CenterY, CrosshairColor, 1.15f);
        DrawLine(CenterX + Gap, CenterY, CenterX + Gap + Len, CenterY, CrosshairColor, 1.15f);
        DrawLine(CenterX, CenterY - Gap - Len, CenterX, CenterY - Gap, CrosshairColor, 1.15f);
        DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Gap + Len, CrosshairColor, 1.15f);
    }

    const float HitAlpha = Player->GetHitMarkerAlpha();
    if (HitAlpha > 0.f)
    {
        const bool bHeadshot = Player->GetHeadshotMarkerAlpha() > 0.f;
        const FLinearColor HitColor = bHeadshot
            ? FLinearColor(1.f, 0.72f, 0.25f, HitAlpha)
            : FLinearColor(1.f, 1.f, 1.f, HitAlpha);
        const float Inner = 6.f;
        const float Outer = 12.f;
        DrawLine(CenterX - Outer, CenterY - Outer, CenterX - Inner, CenterY - Inner, HitColor, 1.7f);
        DrawLine(CenterX + Outer, CenterY - Outer, CenterX + Inner, CenterY - Inner, HitColor, 1.7f);
        DrawLine(CenterX - Outer, CenterY + Outer, CenterX - Inner, CenterY + Inner, HitColor, 1.7f);
        DrawLine(CenterX + Outer, CenterY + Outer, CenterX + Inner, CenterY + Inner, HitColor, 1.7f);
    }

    const float DamageDirectionAlpha = Player->GetDamageDirectionAlpha();
    if (DamageDirectionAlpha > 0.01f && !Player->IsDead())
    {
        const float AngleRad = FMath::DegreesToRadians(Player->GetDamageDirectionDegrees());
        const FVector2D Direction(FMath::Sin(AngleRad), -FMath::Cos(AngleRad));
        const FVector2D Tangent(-Direction.Y, Direction.X);
        const FVector2D IndicatorCenter(CenterX + Direction.X * 88.f, CenterY + Direction.Y * 88.f);
        const FLinearColor IndicatorColor(1.f, 0.15f, 0.08f, 0.86f * DamageDirectionAlpha);
        const FVector2D Tip = IndicatorCenter + Direction * 9.f;
        const FVector2D Left = IndicatorCenter - Direction * 5.f + Tangent * 7.f;
        const FVector2D Right = IndicatorCenter - Direction * 5.f - Tangent * 7.f;
        DrawLine(Tip.X, Tip.Y, Left.X, Left.Y, IndicatorColor, 2.1f);
        DrawLine(Tip.X, Tip.Y, Right.X, Right.Y, IndicatorColor, 2.1f);
        DrawLine(Left.X, Left.Y, Right.X, Right.Y, IndicatorColor, 1.4f);
    }

    AURFPSGameMode* GameMode = Cast<AURFPSGameMode>(UGameplayStatics::GetGameMode(this));
    if (GameMode)
    {
        FString ObjectiveText;
        if (GameMode->IsWaveCleared())
        {
            ObjectiveText = FString::Printf(TEXT("SECTEUR SECURISE  |  AUTO %.0fs  |  POSTE RADIO: E"),
                FMath::CeilToFloat(GameMode->GetNextWaveTimeRemaining()));
        }
        else
        {
            ObjectiveText = FString::Printf(TEXT("VAGUE %02d   HOSTILES %02d   KILLS %03d   %02.0fs"),
                GameMode->GetCurrentWave(), GameMode->GetEnemiesAlive(), GameMode->GetTotalKills(), GameMode->GetCurrentWaveElapsed());
        }
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.40f), CenterX - 235.f, 24.f, 470.f, 30.f);
        DrawText(ObjectiveText, FLinearColor(0.92f, 0.94f, 0.92f, 0.94f), CenterX - 215.f, 32.f, Font, 0.82f, false);

        if (!GameMode->IsWaveCleared())
        {
            const FString Pressure = FString::Printf(TEXT("FEU ENNEMI  %d/%d"), GameMode->GetActiveShooters(), GameMode->GetMaxActiveShooters());
            DrawText(Pressure, FLinearColor(0.72f, 0.74f, 0.70f, 0.72f), CenterX - 52.f, 57.f, Font, 0.66f, false);
        }

        const float IntroAlpha = GameMode->GetWaveIntroAlpha();
        if (IntroAlpha > 0.01f)
        {
            DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f * IntroAlpha), CenterX - 210.f, CenterY - 118.f, 420.f, 64.f);
            DrawText(FString::Printf(TEXT("VAGUE %02d"), GameMode->GetCurrentWave()),
                FLinearColor(0.94f, 0.94f, 0.91f, IntroAlpha), CenterX - 48.f, CenterY - 106.f, Font, 1.20f, false);
            DrawText(GameMode->GetWaveName(), FLinearColor(0.90f, 0.53f, 0.22f, IntroAlpha),
                CenterX - 92.f, CenterY - 82.f, Font, 0.90f, false);
        }
    }

    float Heading = FMath::Fmod(Player->GetControlRotation().Yaw + 360.f, 360.f);
    if (Heading < 0.f) Heading += 360.f;
    FString Cardinal = TEXT("N");
    if (Heading >= 45.f && Heading < 135.f) Cardinal = TEXT("E");
    else if (Heading >= 135.f && Heading < 225.f) Cardinal = TEXT("S");
    else if (Heading >= 225.f && Heading < 315.f) Cardinal = TEXT("O");
    DrawText(FString::Printf(TEXT("%s  %03.0f"), *Cardinal, Heading), FLinearColor(0.86f, 0.88f, 0.86f, 0.78f), CenterX - 28.f, 77.f, Font, 0.78f, false);

    const float PanelY = ScreenH - 104.f;
    const float HealthRatio = Player->GetMaxHealth() > 0.f ? Player->GetHealth() / Player->GetMaxHealth() : 0.f;
    const float ArmorRatio = Player->GetMaxArmor() > 0.f ? Player->GetArmor() / Player->GetMaxArmor() : 0.f;

    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.50f), 28.f, PanelY, 218.f, 64.f);
    DrawText(FString::Printf(TEXT("SANTE  %03.0f"), Player->GetHealth()), FLinearColor::White, 39.f, PanelY + 6.f, Font, 1.02f, false);
    DrawRect(FLinearColor(0.72f, 0.12f, 0.10f, 0.88f), 39.f, PanelY + 27.f, 165.f * HealthRatio, 4.f);
    DrawText(FString::Printf(TEXT("PROTECTION  %02.0f"), Player->GetArmor()), FLinearColor(0.78f, 0.86f, 0.94f, 0.92f), 39.f, PanelY + 34.f, Font, 0.78f, false);
    DrawRect(FLinearColor(0.36f, 0.58f, 0.78f, 0.86f), 139.f, PanelY + 45.f, 65.f * ArmorRatio, 3.f);

    if (Player->GetBleedSeverity() > 0.01f)
    {
        DrawText(FString::Printf(TEXT("SAIGNEMENT  %.0f%%  |  H BANDAGE"), Player->GetBleedSeverity() * 100.f),
            FLinearColor(0.96f, 0.26f, 0.20f, 0.96f), 39.f, PanelY + 51.f, Font, 0.68f, false);
    }

    const float StaminaRatio = Player->GetMaxStamina() > 0.f ? Player->GetStamina() / Player->GetMaxStamina() : 0.f;
    if (StaminaRatio < 0.995f || Player->IsSprinting() || Player->IsHoldingBreath())
    {
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.42f), 28.f, PanelY - 24.f, 218.f, 16.f);
        DrawRect(FLinearColor(0.82f, 0.84f, 0.78f, 0.82f), 39.f, PanelY - 17.f, 165.f * StaminaRatio, 3.f);
    }

    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.50f), ScreenW - 260.f, PanelY, 232.f, 72.f);
    DrawText(FString::Printf(TEXT("%02d  /  %03d"), Player->GetAmmoInMagazine(), Player->GetReserveAmmo()),
        FLinearColor::White, ScreenW - 243.f, PanelY + 8.f, Font, 1.35f, false);
    DrawText(FString::Printf(TEXT("%s   ZERO %dm"), *Player->GetFireModeName(), Player->GetZeroDistanceMeters()),
        FLinearColor(0.82f, 0.84f, 0.81f, 0.94f), ScreenW - 132.f, PanelY + 11.f, Font, 0.72f, false);
    DrawText(FString::Printf(TEXT("GRENADES %d   BANDAGES %d"), Player->GetGrenadeCount(), Player->GetBandageCount()),
        FLinearColor(0.80f, 0.82f, 0.78f, 0.84f), ScreenW - 243.f, PanelY + 36.f, Font, 0.72f, false);
    DrawText(FString::Printf(TEXT("PRECISION %.0f%%   HS %d"), Player->GetAccuracyPercent(), Player->GetHeadshotCount()),
        FLinearColor(0.66f, 0.68f, 0.65f, 0.72f), ScreenW - 243.f, PanelY + 50.f, Font, 0.63f, false);

    FString StateText;
    if (Player->IsTreating()) StateText = TEXT("TRAITEMENT EN COURS");
    else if (Obstruction > 0.82f) StateText = TEXT("ARME BLOQUEE");
    else if (Player->IsReloading()) StateText = TEXT("RECHARGEMENT");
    else if (Player->IsLowReady()) StateText = TEXT("LOW READY");
    else if (Player->IsHoldingBreath()) StateText = TEXT("RETENUE RESPIRATION");
    else if (Player->IsSprinting()) StateText = TEXT("SPRINT");
    else if (Player->IsWalkingSlow()) StateText = TEXT("MARCHE TACTIQUE");
    else if (Player->IsAiming()) StateText = TEXT("VISEE");

    if (!StateText.IsEmpty())
    {
        const FLinearColor StateColor = Obstruction > 0.82f
            ? FLinearColor(0.95f, 0.42f, 0.20f, 0.95f)
            : FLinearColor(0.86f, 0.88f, 0.86f, 0.92f);
        DrawText(StateText, StateColor, ScreenW - 260.f, PanelY - 18.f, Font, 0.82f, false);
    }

    if (Player->IsReloading())
    {
        const float ReloadProgress = Player->GetReloadProgress();
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.42f), ScreenW - 260.f, PanelY + 78.f, 232.f, 10.f);
        DrawRect(FLinearColor(0.78f, 0.80f, 0.75f, 0.88f), ScreenW - 256.f, PanelY + 81.f, 224.f * ReloadProgress, 4.f);
    }

    if (Player->IsTreating())
    {
        const float Treatment = Player->GetTreatmentProgress();
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.56f), CenterX - 132.f, ScreenH - 188.f, 264.f, 28.f);
        DrawRect(FLinearColor(0.74f, 0.78f, 0.70f, 0.92f), CenterX - 122.f, ScreenH - 174.f, 244.f * Treatment, 4.f);
        DrawText(TEXT("BANDAGE"), FLinearColor(0.92f, 0.94f, 0.90f, 0.94f), CenterX - 29.f, ScreenH - 184.f, Font, 0.76f, false);
    }

    if (Player->IsFlashlightOn())
    {
        DrawText(TEXT("LAMPE"), FLinearColor(0.95f, 0.88f, 0.64f, 0.9f), 31.f, PanelY - 46.f, Font, 0.78f, false);
    }

    const FString InteractionPrompt = Player->GetInteractionPrompt();
    if (!InteractionPrompt.IsEmpty() && !Player->IsDead())
    {
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.52f), CenterX - 152.f, ScreenH - 154.f, 304.f, 30.f);
        DrawText(InteractionPrompt, FLinearColor(0.94f, 0.94f, 0.90f, 1.f), CenterX - 132.f, ScreenH - 146.f, Font, 0.88f, false);
    }

    DrawText(TEXT("T LOW READY   G GRENADE   H BANDAGE   B MODE   V ZERO"),
        FLinearColor(0.68f, 0.70f, 0.67f, 0.58f), 30.f, ScreenH - 22.f, Font, 0.62f, false);

    const float Suppression = Player->GetSuppression();
    if (Suppression > 0.01f && !Player->IsDead())
    {
        const FLinearColor SuppressionColor(0.58f, 0.59f, 0.56f, 0.10f * Suppression);
        const float Edge = 70.f;
        DrawRect(SuppressionColor, 0.f, 0.f, ScreenW, Edge);
        DrawRect(SuppressionColor, 0.f, ScreenH - Edge, ScreenW, Edge);
        DrawRect(SuppressionColor, 0.f, Edge, Edge, ScreenH - Edge * 2.f);
        DrawRect(SuppressionColor, ScreenW - Edge, Edge, Edge, ScreenH - Edge * 2.f);
    }

    if (Player->GetBleedSeverity() > 0.01f && !Player->IsDead())
    {
        const float BleedAlpha = 0.025f + Player->GetBleedSeverity() * 0.055f;
        const FLinearColor BleedColor(0.35f, 0.005f, 0.005f, BleedAlpha);
        const float Edge = 28.f + Player->GetBleedSeverity() * 22.f;
        DrawRect(BleedColor, 0.f, 0.f, ScreenW, Edge);
        DrawRect(BleedColor, 0.f, ScreenH - Edge, ScreenW, Edge);
    }

    const float DamageAlpha = Player->GetDamageFeedback();
    if (DamageAlpha > 0.01f)
    {
        const FLinearColor DamageColor(0.55f, 0.01f, 0.01f, 0.19f * DamageAlpha);
        const float Edge = 42.f;
        DrawRect(DamageColor, 0.f, 0.f, ScreenW, Edge);
        DrawRect(DamageColor, 0.f, ScreenH - Edge, ScreenW, Edge);
        DrawRect(DamageColor, 0.f, Edge, Edge, ScreenH - Edge * 2.f);
        DrawRect(DamageColor, ScreenW - Edge, Edge, Edge, ScreenH - Edge * 2.f);
    }

    if (Player->IsDead())
    {
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.60f), 0.f, 0.f, ScreenW, ScreenH);
        DrawText(TEXT("HORS COMBAT"), FLinearColor(0.9f, 0.9f, 0.9f, 1.f), CenterX - 72.f, CenterY - 22.f, Font, 1.45f, false);
        DrawText(TEXT("R  REPRENDRE"), FLinearColor(0.82f, 0.84f, 0.82f, 0.92f), CenterX - 58.f, CenterY + 14.f, Font, 0.92f, false);
        DrawText(FString::Printf(TEXT("TIRS %d   TOUCHES %d   PRECISION %.0f%%   HEADSHOTS %d"),
            Player->GetShotsFired(), Player->GetConfirmedHits(), Player->GetAccuracyPercent(), Player->GetHeadshotCount()),
            FLinearColor(0.70f, 0.72f, 0.70f, 0.88f), CenterX - 150.f, CenterY + 42.f, Font, 0.74f, false);
    }
}
