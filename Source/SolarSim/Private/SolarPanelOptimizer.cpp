// =============================================================================
//  SolarPanelOptimizer.cpp
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "SolarPanelOptimizer.h"
#include "Math/UnrealMathUtility.h"

// ─────────────────────────────────────────────────────────────────────────────
// Internal: Clip to bounds
// ─────────────────────────────────────────────────────────────────────────────
TArray<double> USolarPanelOptimizer::ClipToBounds(
    const TArray<double>& X,
    const TArray<TPair<double,double>>& Bounds)
{
    TArray<double> Out;
    Out.SetNum(X.Num());
    for (int32 i = 0; i < X.Num(); ++i)
        Out[i] = FMath::Clamp(X[i], Bounds[i].Key, Bounds[i].Value);
    return Out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: Central-Difference Numerical Gradient
// ─────────────────────────────────────────────────────────────────────────────
TArray<double> USolarPanelOptimizer::NumericalGradient(
    const TArray<double>& X,
    const TArray<TPair<double,double>>& Bounds,
    TFunction<double(TArray<double>)> Func,
    double HFrac)
{
    TArray<double> Grad;
    Grad.SetNum(X.Num());

    for (int32 i = 0; i < X.Num(); ++i)
    {
        const double Span = Bounds[i].Value - Bounds[i].Key;
        const double h    = HFrac * Span;

        TArray<double> Xp = X, Xm = X;
        Xp[i] = FMath::Clamp(X[i] + h, Bounds[i].Key, Bounds[i].Value);
        Xm[i] = FMath::Clamp(X[i] - h, Bounds[i].Key, Bounds[i].Value);

        Grad[i] = (Func(Xp) - Func(Xm)) / (2.0 * h + 1e-12);
    }
    return Grad;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: Gradient Ascent Core
// ─────────────────────────────────────────────────────────────────────────────
USolarPanelOptimizer::FInternalResult USolarPanelOptimizer::RunGradientAscent(
    TArray<double>                     X0,
    TArray<TPair<double,double>>       Bounds,
    TFunction<double(TArray<double>)>  ObjFunc,
    double                             LR,
    int32                              MaxIter,
    double                             Tol,
    double                             HFrac)
{
    TArray<double> X     = ClipToBounds(X0, Bounds);
    TArray<double> PrevX;
    bool           bConverged = false;
    float          LRf        = (float)LR;

    FInternalResult Res;
    Res.PHistory.Reserve(MaxIter);
    if (X.Num() > 0) Res.VarHistory0.Reserve(MaxIter);
    if (X.Num() > 1) Res.VarHistory1.Reserve(MaxIter);

    for (int32 Iter = 0; Iter < MaxIter; ++Iter)
    {
        const double PVal = ObjFunc(X);
        Res.PHistory.Add((float)PVal);
        if (X.Num() > 0) Res.VarHistory0.Add((float)X[0]);
        if (X.Num() > 1) Res.VarHistory1.Add((float)X[1]);

        // ── Numerical gradient ────────────────────────────────────────────
        TArray<double> Grad = NumericalGradient(X, Bounds, ObjFunc, HFrac);

        // ‖∇P‖
        double GradNorm = 0.0;
        for (double g : Grad) GradNorm += g * g;
        GradNorm = FMath::Sqrt(GradNorm);

        if (GradNorm < Tol) { bConverged = true; break; }

        // ── Adaptive Scale  (발산 방지) ───────────────────────────────────
        const double Scale = FMath::Min(1.0, (double)AdaptiveScaleK / (GradNorm + 1e-8));

        // ── Oscillation Damping ───────────────────────────────────────────
        if (PrevX.Num() == X.Num())
        {
            double DeltaNorm = 0.0;
            for (int32 i = 0; i < X.Num(); ++i)
            {
                double d = X[i] - PrevX[i];
                DeltaNorm += d * d;
            }
            DeltaNorm = FMath::Sqrt(DeltaNorm);
            if (DeltaNorm < (double)OscillationThreshold)
                LR *= (double)OscillationDecay;
        }

        PrevX = X;

        // ── Parameter update + Boundary Clipping ─────────────────────────
        TArray<double> XNew;
        XNew.SetNum(X.Num());
        for (int32 i = 0; i < X.Num(); ++i)
            XNew[i] = X[i] + LR * Grad[i] * Scale;

        XNew = ClipToBounds(XNew, Bounds);

        // ── Step convergence check ────────────────────────────────────────
        double StepNorm = 0.0;
        for (int32 i = 0; i < X.Num(); ++i)
        {
            double d = XNew[i] - X[i];
            StepNorm += d * d;
        }
        StepNorm = FMath::Sqrt(StepNorm);

        X = XNew;
        if (StepNorm < Tol) { bConverged = true; break; }
    }

    Res.X         = X;
    Res.P         = ObjFunc(X);
    Res.Iter      = Res.PHistory.Num();
    Res.Converged = bConverged;
    Res.LRFinal   = (float)LR;
    return Res;
}

// ─────────────────────────────────────────────────────────────────────────────
// MODE A: 2D Angle Optimization
// ─────────────────────────────────────────────────────────────────────────────
FOptimizationResult USolarPanelOptimizer::OptimizeModeA(
    const FSolarWeatherParams& Weather,
    float InitTheta, float InitPhi)
{
    TArray<TPair<double,double>> Bounds = {
        {0.0, 90.0},     // theta
        {-90.0, 90.0},   // phi
    };

    // Capture weather by value for lambda
    FSolarWeatherParams W = Weather;

    // Multi-start: 3x3 grid to avoid local optima
    FInternalResult Best;
    Best.P = -1e9;
    TArray<float> ThetaStarts = {20.f, 37.f, 55.f};
    TArray<float> PhiStarts   = {-20.f, 0.f, 20.f};

    for (float T0 : ThetaStarts)
    for (float P0 : PhiStarts)
    {
        TArray<double> X0 = {(double)T0, (double)P0};
        auto ObjFunc = [W](TArray<double> X) -> double {
            return SolarMath::PowerOutput(X[0], X[1], W);
        };

        FInternalResult Res = RunGradientAscent(
            X0, Bounds, ObjFunc,
            (double)LearningRate, MaxIterations,
            (double)Tolerance, (double)HFraction);

        if (Res.P > Best.P) Best = Res;
    }

    LastResult.ThetaOptDeg      = (float)Best.X[0];
    LastResult.PhiOptDeg        = (float)Best.X[1];
    LastResult.PowerOptWm2      = (float)Best.P;
    LastResult.Iterations       = Best.Iter;
    LastResult.bConverged       = Best.Converged;
    LastResult.FinalLearningRate= Best.LRFinal;
    LastResult.PowerHistory     = Best.PHistory;
    LastResult.ThetaHistory     = Best.VarHistory0;
    LastResult.PhiHistory       = Best.VarHistory1;

    return LastResult;
}

// ─────────────────────────────────────────────────────────────────────────────
// MODE B: Robust Annual Optimization
// ─────────────────────────────────────────────────────────────────────────────
FOptimizationResult USolarPanelOptimizer::OptimizeModeB(
    const TArray<FSolarWeatherParams>& MonthlyScenarios,
    const TArray<float>&               Weights,
    float InitTheta, float InitPhi)
{
    check(MonthlyScenarios.Num() == Weights.Num());

    // Normalize weights
    float TotalW = 0.f;
    for (float w : Weights) TotalW += w;
    TArray<float> NormW = Weights;
    for (float& w : NormW) w /= TotalW;

    TArray<TPair<double,double>> Bounds = { {0.0, 90.0}, {-90.0, 90.0} };

    // Capture by value
    TArray<FSolarWeatherParams> Scenarios = MonthlyScenarios;
    TArray<float> W = NormW;

    FInternalResult Best;
    Best.P = -1e9;
    TArray<float> ThetaStarts = {25.f, 37.f, 45.f, 55.f};
    TArray<float> PhiStarts   = {-15.f, 0.f, 15.f};

    for (float T0 : ThetaStarts)
    for (float P0 : PhiStarts)
    {
        TArray<double> X0 = {(double)T0, (double)P0};
        auto ObjFunc = [Scenarios, W](TArray<double> X) -> double {
            double Total = 0.0;
            for (int32 i = 0; i < Scenarios.Num(); ++i)
                Total += (double)W[i] * SolarMath::PowerOutput(X[0], X[1], Scenarios[i]);
            return Total;
        };

        FInternalResult Res = RunGradientAscent(
            X0, Bounds, ObjFunc,
            0.6, MaxIterations,
            (double)Tolerance, (double)HFraction);

        if (Res.P > Best.P) Best = Res;
    }

    LastResult.ThetaOptDeg      = (float)Best.X[0];
    LastResult.PhiOptDeg        = (float)Best.X[1];
    LastResult.PowerOptWm2      = (float)Best.P;
    LastResult.Iterations       = Best.Iter;
    LastResult.bConverged       = Best.Converged;
    LastResult.FinalLearningRate= Best.LRFinal;
    LastResult.PowerHistory     = Best.PHistory;
    LastResult.ThetaHistory     = Best.VarHistory0;
    LastResult.PhiHistory       = Best.VarHistory1;

    return LastResult;
}

// ─────────────────────────────────────────────────────────────────────────────
// MODE C: 6D Full Optimization
// ─────────────────────────────────────────────────────────────────────────────
FOptimizationResult USolarPanelOptimizer::OptimizeModeC(
    int DayOfYear, float SolarHour)
{
    TArray<TPair<double,double>> Bounds = {
        {0.0,   90.0},    // theta
        {-90.0, 90.0},    // phi
        {0.0,  1200.0},   // G
        {0.0,    1.0},    // c
        {-10.0,  50.0},   // T
        {0.0,   30.0},    // v
    };

    int   DOY = DayOfYear;
    float HR  = SolarHour;

    auto ObjFunc = [DOY, HR](TArray<double> X) -> double {
        FSolarWeatherParams W;
        W.G   = X[2]; W.c   = X[3]; W.T   = X[4];
        W.v   = X[5]; W.doy = DOY;  W.hr  = (double)HR;
        return SolarMath::PowerOutput(X[0], X[1], W);
    };

    TArray<TArray<double>> Starts = {
        {37.0, 0.0, 1000.0, 0.0, -10.0, 15.0},
        {45.0, 0.0, 1200.0, 0.0, -10.0, 20.0},
        {30.0, 5.0,  900.0, 0.0,  10.0, 10.0},
        {50.0,-5.0, 1100.0, 0.0,   0.0, 25.0},
    };

    FInternalResult Best;
    Best.P = -1e9;

    for (TArray<double>& X0 : Starts)
    {
        FInternalResult Res = RunGradientAscent(
            X0, Bounds, ObjFunc,
            0.5, MaxIterations * 2,
            (double)Tolerance, 0.003);

        if (Res.P > Best.P) Best = Res;
    }

    LastResult.ThetaOptDeg      = (float)Best.X[0];
    LastResult.PhiOptDeg        = (float)Best.X[1];
    LastResult.PowerOptWm2      = (float)Best.P;
    LastResult.Iterations       = Best.Iter;
    LastResult.bConverged       = Best.Converged;
    LastResult.FinalLearningRate= Best.LRFinal;
    LastResult.PowerHistory     = Best.PHistory;
    LastResult.ThetaHistory     = Best.VarHistory0;
    LastResult.PhiHistory       = Best.VarHistory1;

    return LastResult;
}
