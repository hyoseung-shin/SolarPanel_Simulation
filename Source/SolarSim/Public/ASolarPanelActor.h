#pragma once
// =============================================================================
//  ASolarPanelActor.h
//  3D Solar Panel Actor — Lumen lighting, Niagara particles, UMG HUD
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolarMath.h"
#include "SolarPanelOptimizer.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ASolarPanelActor.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOptimizationComplete,
    const FOptimizationResult&, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPowerUpdated,
    float, PowerWm2, float, EfficiencyPct);

// ─────────────────────────────────────────────────────────────────────────────
// Main Actor
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(BlueprintType, Blueprintable)
class SOLARSIM_API ASolarPanelActor : public AActor
{
    GENERATED_BODY()

public:
    ASolarPanelActor();

    // ── Components ────────────────────────────────────────────────────────

    /** Panel geometry */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Solar|Components")
    UStaticMeshComponent* PanelMesh;

    /** Panel support frame */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Solar|Components")
    UStaticMeshComponent* FrameMesh;

    /** Arrow showing panel normal direction */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Solar|Components")
    UArrowComponent* NormalArrow;

    /**
     * Niagara system — visual feedback for:
     *   • Power output intensity (particle density / color)
     *   • Gradient flow direction
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Solar|Components")
    UNiagaraComponent* PowerParticles;

    /** Floating text: current angle + power */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Solar|Components")
    UTextRenderComponent* InfoText;

    // ── Simulation State ──────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar|Simulation")
    EOptimizationMode OptimMode = EOptimizationMode::ModeA_2DAngle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar|Simulation")
    FSolarWeatherParams CurrentWeather;

    /** If true, solar time advances automatically each tick */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar|Simulation")
    bool bAutoAdvanceTime = false;

    /** Simulation speed multiplier (1 = real time, 3600 = 1h/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar|Simulation",
              meta=(ClampMin="1", ClampMax="86400"))
    float TimeScaleFactor = 360.0f;

    /** Whether to run optimizer every frame or only on-demand */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar|Simulation")
    bool bContinuousOptimization = false;

    /** Run optimization every N seconds when ContinuousOptimization=true */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar|Simulation",
              meta=(ClampMin="0.1"))
    float OptimizationInterval = 1.0f;

    // ── Optimizer reference ───────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar|Optimizer")
    USolarPanelOptimizer* Optimizer;

    // ── Delegates ─────────────────────────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category="Solar|Events")
    FOnOptimizationComplete OnOptimizationComplete;

    UPROPERTY(BlueprintAssignable, Category="Solar|Events")
    FOnPowerUpdated OnPowerUpdated;

    // ── Blueprint API ──────────────────────────────────────────────────────

    /**
     * Trigger a full optimization pass.
     * Result is broadcast via OnOptimizationComplete.
     */
    UFUNCTION(BlueprintCallable, Category="Solar")
    void RunOptimization();

    /** Manually set panel angles (skips optimizer) */
    UFUNCTION(BlueprintCallable, Category="Solar")
    void SetPanelAngles(float ThetaDeg, float PhiDeg);

    /** Set weather snapshot and re-evaluate power */
    UFUNCTION(BlueprintCallable, Category="Solar")
    void SetWeather(const FSolarWeatherParams& Weather);

    /** Current instantaneous power output */
    UFUNCTION(BlueprintPure, Category="Solar")
    float GetCurrentPower() const { return CurrentPowerWm2; }

    /** Current panel efficiency percentage */
    UFUNCTION(BlueprintPure, Category="Solar")
    float GetEfficiencyPct() const { return CurrentEfficiencyPct; }

    UFUNCTION(BlueprintPure, Category="Solar")
    FRotator GetOptimalPanelRotation() const;

    UFUNCTION(BlueprintPure, Category="Solar")
    FVector GetSunDirection() const;

    /** Animate panel smoothly toward target angles */
    UFUNCTION(BlueprintCallable, Category="Solar")
    void AnimateToAngles(float TargetTheta, float TargetPhi, float DurationSec = 2.0f);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // Current angles
    float CurrentTheta = 35.f;
    float CurrentPhi = 0.f;

private:
    // Animation state
    float TargetTheta  = 35.f;
    float TargetPhi    = 0.f;
    float AnimDuration = 0.f;
    float AnimElapsed  = 0.f;
    bool  bAnimating   = false;
    float AnimStartTheta = 35.f;
    float AnimStartPhi   = 0.f;

    float CurrentPowerWm2      = 0.f;
    float CurrentEfficiencyPct = 0.f;
    float OptimizationTimer    = 0.f;

    UPROPERTY()
    UMaterialInstanceDynamic* PanelMID = nullptr;

    void UpdatePanelTransform();
    void UpdatePowerCalculation();
    void UpdateVisuals();
    void UpdateNiagaraParams();
    void UpdateInfoText();

    /** Map power [0–200 W/m²] to material emissive color */
    FLinearColor PowerToColor(float PowerWm2) const;
};
