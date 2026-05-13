#pragma once
// =============================================================================
//  SolarPanelOptimizer.h
//  Gradient Ascent Engine with Adaptive Scale + Oscillation Damping
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "CoreMinimal.h"
#include "SolarMath.h"
#include "SolarPanelOptimizer.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Enums
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EOptimizationMode : uint8
{
    ModeA_2DAngle       UMETA(DisplayName = "Mode A — 2D Angle (고정 날씨)"),
    ModeB_RobustAnnual  UMETA(DisplayName = "Mode B — Robust Annual"),
    ModeC_6DFull        UMETA(DisplayName = "Mode C — 6D Full"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Result struct (Blueprint-accessible)
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct SOLARSIM_API FOptimizationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Solar|Result")
    float ThetaOptDeg = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Solar|Result")
    float PhiOptDeg = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Solar|Result")
    float PowerOptWm2 = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Solar|Result")
    int32 Iterations = 0;

    UPROPERTY(BlueprintReadOnly, Category="Solar|Result")
    bool bConverged = false;

    UPROPERTY(BlueprintReadOnly, Category="Solar|Result")
    float FinalLearningRate = 0.f;

    /** Per-iteration convergence history for visualization */
    TArray<float> PowerHistory;
    TArray<float> ThetaHistory;
    TArray<float> PhiHistory;
};

// ─────────────────────────────────────────────────────────────────────────────
// Optimizer UObject
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(BlueprintType, Blueprintable)
class SOLARSIM_API USolarPanelOptimizer : public UObject
{
    GENERATED_BODY()

public:
    // ── Hyperparameters (tweakable in Blueprint defaults) ─────────────────

    /** Initial learning rate η */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optimizer|Params")
    float LearningRate = 0.8f;

    /** Central-difference step fraction of variable range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optimizer|Params")
    float HFraction = 0.005f;

    /** Convergence threshold on step norm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optimizer|Params")
    float Tolerance = 1e-6f;

    /** Max gradient ascent iterations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optimizer|Params")
    int32 MaxIterations = 1000;

    /** Adaptive scale cap denominator constant k  (scale = min(1, k/‖∇P‖)) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optimizer|Params")
    float AdaptiveScaleK = 5.0f;

    /** lr decay factor on oscillation detect */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optimizer|Params")
    float OscillationDecay = 0.95f;

    /** Step-norm threshold below which oscillation is assumed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optimizer|Params")
    float OscillationThreshold = 0.3f;

    // ── Public API ─────────────────────────────────────────────────────────

    /**
     * Mode A: Optimize (θ, φ) for fixed weather conditions.
     * Called from Blueprint or C++ with current weather snapshot.
     */
    UFUNCTION(BlueprintCallable, Category="Solar|Optimizer")
    FOptimizationResult OptimizeModeA(const FSolarWeatherParams& Weather,
                                       float InitTheta = 35.f,
                                       float InitPhi   = 0.f);

    /**
     * Mode B: Robust annual optimization.
     * Maximizes Σᵢ wᵢ·P(θ,φ|ωᵢ) over provided monthly scenarios.
     */
    UFUNCTION(BlueprintCallable, Category="Solar|Optimizer")
    FOptimizationResult OptimizeModeB(
        const TArray<FSolarWeatherParams>& MonthlyScenarios,
        const TArray<float>&               Weights,
        float InitTheta = 35.f,
        float InitPhi   = 0.f);

    /**
     * Mode C: Full 6-variable optimization.
     * Finds theoretical maximum over all controllable parameters.
     */
    UFUNCTION(BlueprintCallable, Category="Solar|Optimizer")
    FOptimizationResult OptimizeModeC(int DayOfYear = 172, float SolarHour = 12.f);

    /** Read-only last result */
    UFUNCTION(BlueprintPure, Category="Solar|Optimizer")
    FOptimizationResult GetLastResult() const { return LastResult; }

private:
    FOptimizationResult LastResult;

    // Internal double-precision optimizer to avoid float truncation errors
    struct FInternalResult
    {
        TArray<double> X;
        double         P;
        int32          Iter;
        bool           Converged;
        float          LRFinal;
        TArray<float>  PHistory;
        TArray<float>  VarHistory0;
        TArray<float>  VarHistory1;
    };

    FInternalResult RunGradientAscent(
        TArray<double>                      X0,
        TArray<TPair<double,double>>        Bounds,
        TFunction<double(TArray<double>)>   ObjFunc,
        double                              LR,
        int32                               MaxIter,
        double                              Tol,
        double                              HFrac);

    TArray<double> NumericalGradient(
        const TArray<double>&             X,
        const TArray<TPair<double,double>>& Bounds,
        TFunction<double(TArray<double>)> Func,
        double                            HFrac);

    static TArray<double> ClipToBounds(const TArray<double>& X,
                                        const TArray<TPair<double,double>>& Bounds);
};
