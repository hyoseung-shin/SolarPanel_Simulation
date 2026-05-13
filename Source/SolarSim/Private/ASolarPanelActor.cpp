// =============================================================================
//  ASolarPanelActor.cpp
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "ASolarPanelActor.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"

ASolarPanelActor::ASolarPanelActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // ── Root ─────────────────────────────────────────────────────────────────
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    // ── Panel Mesh ────────────────────────────────────────────────────────────
    PanelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PanelMesh"));
    PanelMesh->SetupAttachment(Root);
    PanelMesh->SetRelativeScale3D(FVector(2.0f, 1.0f, 0.05f));  // 2m×1m thin slab
    // Assign mesh in Blueprint: SM_SolarPanel or built-in plane mesh

    // ── Frame Mesh ────────────────────────────────────────────────────────────
    FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMesh->SetupAttachment(Root);

    // ── Normal Arrow ──────────────────────────────────────────────────────────
    NormalArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("NormalArrow"));
    NormalArrow->SetupAttachment(PanelMesh);
    NormalArrow->ArrowColor = FColor::Yellow;
    NormalArrow->ArrowLength = 120.f;
    NormalArrow->SetRelativeLocation(FVector(0, 0, 5));

    // ── Niagara ───────────────────────────────────────────────────────────────
    PowerParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PowerParticles"));
    PowerParticles->SetupAttachment(PanelMesh);
    // Assign NS_PowerGlow system in Blueprint

    // ── Info Text ─────────────────────────────────────────────────────────────
    InfoText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InfoText"));
    InfoText->SetupAttachment(Root);
    InfoText->SetRelativeLocation(FVector(0, 0, 150.f));
    InfoText->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
    InfoText->SetTextRenderColor(FColor::White);
    InfoText->WorldSize = 18.f;
    InfoText->HorizontalAlignment = EHTA_Center;
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::BeginPlay()
{
    Super::BeginPlay();

    // Create optimizer instance
    if (!Optimizer)
        Optimizer = NewObject<USolarPanelOptimizer>(this);

    // Dynamic material for emissive power feedback
    if (PanelMesh && PanelMesh->GetMaterial(0))
    {
        PanelMID = UMaterialInstanceDynamic::Create(
            PanelMesh->GetMaterial(0), this);
        PanelMesh->SetMaterial(0, PanelMID);
    }

    UpdatePanelTransform();
    UpdatePowerCalculation();
    UpdateVisuals();
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ── Time auto-advance ─────────────────────────────────────────────────────
    if (bAutoAdvanceTime)
    {
        CurrentWeather.hr += DeltaTime * TimeScaleFactor / 3600.0f;

        // Wrap 24h, advance day
        if (CurrentWeather.hr >= 24.0)
        {
            CurrentWeather.hr -= 24.0;
            CurrentWeather.doy = (CurrentWeather.doy % 365) + 1;
        }
        if (CurrentWeather.hr < 0.0) CurrentWeather.hr += 24.0;
    }

    // ── Animation (smooth panel rotation) ────────────────────────────────────
    if (bAnimating)
    {
        AnimElapsed += DeltaTime;
        float t = FMath::Clamp(AnimElapsed / AnimDuration, 0.f, 1.f);
        // Smooth-step easing
        t = t * t * (3.f - 2.f * t);

        CurrentTheta = FMath::Lerp(AnimStartTheta, TargetTheta, t);
        CurrentPhi   = FMath::Lerp(AnimStartPhi,   TargetPhi,   t);

        UpdatePanelTransform();
        if (AnimElapsed >= AnimDuration) bAnimating = false;
    }

    // ── Continuous optimization ───────────────────────────────────────────────
    if (bContinuousOptimization)
    {
        OptimizationTimer += DeltaTime;
        if (OptimizationTimer >= OptimizationInterval)
        {
            OptimizationTimer = 0.f;
            RunOptimization();
        }
    }

    UpdatePowerCalculation();
    UpdateVisuals();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::RunOptimization()
{
    if (!Optimizer) return;

    FOptimizationResult Result;

    switch (OptimMode)
    {
    case EOptimizationMode::ModeA_2DAngle:
        Result = Optimizer->OptimizeModeA(CurrentWeather, CurrentTheta, CurrentPhi);
        break;

    case EOptimizationMode::ModeB_RobustAnnual:
        {
            // Seoul 12-month representative weather
            TArray<FSolarWeatherParams> Scenarios;
            TArray<float> Weights;

            const float MonthG[]  = {550,620,750,820,870,780,700,900,840,720,580,510};
            const float MonthC[]  = {.20f,.22f,.28f,.35f,.40f,.65f,.72f,.30f,.25f,.20f,.22f,.20f};
            const float MonthT[]  = {-3,0,7,14,20,24,27,28,22,14,5,-1};
            const float MonthV[]  = {3.5f,3.8f,3.2f,2.8f,2.5f,2.2f,2.0f,2.3f,2.6f,3.0f,3.5f,3.8f};
            const int   MonthDOY[]= {15,46,75,105,136,167,197,228,259,289,320,350};
            const float MonthW[]  = {.065f,.072f,.082f,.090f,.095f,.075f,.060f,.100f,.095f,.090f,.075f,.101f};

            for (int i = 0; i < 12; ++i)
            {
                FSolarWeatherParams W;
                W.G=MonthG[i]; W.c=MonthC[i]; W.T=MonthT[i];
                W.v=MonthV[i]; W.doy=MonthDOY[i]; W.hr=12.0;
                Scenarios.Add(W);
                Weights.Add(MonthW[i]);
            }
            Result = Optimizer->OptimizeModeB(Scenarios, Weights, CurrentTheta, CurrentPhi);
        }
        break;

    case EOptimizationMode::ModeC_6DFull:
        Result = Optimizer->OptimizeModeC(CurrentWeather.doy, (float)CurrentWeather.hr);
        break;
    }

    // Animate panel to optimal angle
    AnimateToAngles(Result.ThetaOptDeg, Result.PhiOptDeg, 2.0f);

    OnOptimizationComplete.Broadcast(Result);

    UE_LOG(LogTemp, Log,
        TEXT("[SolarSim] Mode=%d  θ=%.2f°  φ=%.2f°  P=%.3f W/m²  iter=%d  conv=%d"),
        (int)OptimMode, Result.ThetaOptDeg, Result.PhiOptDeg,
        Result.PowerOptWm2, Result.Iterations, (int)Result.bConverged);
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::SetPanelAngles(float ThetaDeg, float PhiDeg)
{
    CurrentTheta = FMath::Clamp(ThetaDeg,  0.f, 90.f);
    CurrentPhi   = FMath::Clamp(PhiDeg, -90.f, 90.f);
    UpdatePanelTransform();
    UpdatePowerCalculation();
    UpdateVisuals();
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::SetWeather(const FSolarWeatherParams& Weather)
{
    CurrentWeather = Weather;
    UpdatePowerCalculation();
    UpdateVisuals();
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::AnimateToAngles(float InTargetTheta, float InTargetPhi,
                                        float DurationSec)
{
    AnimStartTheta = CurrentTheta;
    AnimStartPhi   = CurrentPhi;
    TargetTheta    = FMath::Clamp(InTargetTheta,  0.f, 90.f);
    TargetPhi      = FMath::Clamp(InTargetPhi, -90.f, 90.f);
    AnimDuration   = FMath::Max(0.1f, DurationSec);
    AnimElapsed    = 0.f;
    bAnimating     = true;
}

// ─────────────────────────────────────────────────────────────────────────────
FRotator ASolarPanelActor::GetOptimalPanelRotation() const
{
    // Panel normal: tilt θ forward (south), rotate φ around vertical
    // In UE5 coordinate: X=North, Y=East, Z=Up
    // South = -X direction, so:
    //   Pitch = -CurrentTheta (tilt from horizontal toward south)
    //   Yaw   =  CurrentPhi   (east/west deviation from south)
    return FRotator(-CurrentTheta, CurrentPhi, 0.f);
}

// ─────────────────────────────────────────────────────────────────────────────
FVector ASolarPanelActor::GetSunDirection() const
{
    double Alt = SolarMath::SolarAltitude(CurrentWeather.doy, CurrentWeather.hr);
    double Az  = SolarMath::SolarAzimuth(CurrentWeather.doy, CurrentWeather.hr);

    // Convert to UE5 world direction (X=North, Y=East, Z=Up)
    double AltR = SolarMath::D2R(Alt);
    double AzR  = SolarMath::D2R(Az);

    // Azimuth from south: south = -X
    float Sx = -(float)(FMath::Cos(AltR) * FMath::Cos(AzR));  // North component
    float Sy =  (float)(FMath::Cos(AltR) * FMath::Sin(AzR));  // East component
    float Sz =  (float)(FMath::Sin(AltR));                      // Up component

    return FVector(Sx, Sy, Sz).GetSafeNormal();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::UpdatePanelTransform()
{
    if (!PanelMesh) return;
    // Pitch: tilt away from horizontal toward south
    // Yaw:   azimuth deviation from south
    PanelMesh->SetRelativeRotation(FRotator(-CurrentTheta, CurrentPhi, 0.f));
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::UpdatePowerCalculation()
{
    CurrentPowerWm2 = (float)SolarMath::PowerOutput(
        (double)CurrentTheta, (double)CurrentPhi, CurrentWeather);

    // Efficiency relative to theoretical maximum irradiance condition
    const float MaxTheoretical = (float)(SolarConst::ETA_0 * SolarConst::ETA_SYS * 1200.0);
    CurrentEfficiencyPct = FMath::Clamp(
        CurrentPowerWm2 / (MaxTheoretical + 1e-6f) * 100.f, 0.f, 100.f);

    OnPowerUpdated.Broadcast(CurrentPowerWm2, CurrentEfficiencyPct);
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::UpdateVisuals()
{
    UpdateNiagaraParams();
    UpdateInfoText();

    if (PanelMID)
    {
        FLinearColor Col = PowerToColor(CurrentPowerWm2);
        PanelMID->SetVectorParameterValue(TEXT("EmissiveColor"), Col);
        PanelMID->SetScalarParameterValue(TEXT("EmissiveIntensity"),
            FMath::Clamp(CurrentPowerWm2 / 200.f, 0.f, 1.f) * 3.f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::UpdateNiagaraParams()
{
    if (!PowerParticles) return;

    // Niagara user parameters (set these up in the NS_PowerGlow asset):
    //   User.PowerNormalized  [0–1]  → controls spawn rate & size
    //   User.PowerColor       [RGBA] → particle tint
    //   User.SunDirection     [vec3] → gradient flow direction

    float Norm = FMath::Clamp(CurrentPowerWm2 / 200.f, 0.f, 1.f);
    PowerParticles->SetFloatParameter(TEXT("User.PowerNormalized"), Norm);
    PowerParticles->SetColorParameter(TEXT("User.PowerColor"),
        FLinearColor(PowerToColor(CurrentPowerWm2)));
    PowerParticles->SetVectorParameter(TEXT("User.SunDirection"), GetSunDirection());
}

// ─────────────────────────────────────────────────────────────────────────────
void ASolarPanelActor::UpdateInfoText()
{
    if (!InfoText) return;

    FString Line1 = FString::Printf(TEXT("θ=%.1f°  φ=%.1f°"), CurrentTheta, CurrentPhi);
    FString Line2 = FString::Printf(TEXT("P = %.1f W/m²  (%.1f%%)"),
        CurrentPowerWm2, CurrentEfficiencyPct);
    FString Line3 = FString::Printf(TEXT("☀ Alt=%.1f°  T=%.1f°C"),
        (float)SolarMath::SolarAltitude(CurrentWeather.doy, CurrentWeather.hr),
        (float)CurrentWeather.T);

    InfoText->SetText(FText::FromString(Line1 + TEXT("\n") + Line2 + TEXT("\n") + Line3));
}

// ─────────────────────────────────────────────────────────────────────────────
FLinearColor ASolarPanelActor::PowerToColor(float PowerWm2) const
{
    // 0 W/m² → deep blue → cyan → green → yellow → orange → 200+ W/m² red-white
    float t = FMath::Clamp(PowerWm2 / 200.f, 0.f, 1.f);
    // Hue: 240° (blue) → 0° (red)
    float H = FMath::Lerp(240.f, 0.f, t);
    FLinearColor HSV(H, 1.f - t * 0.3f, 0.5f + t * 0.5f);
    return HSV.HSVToLinearRGB();
}
