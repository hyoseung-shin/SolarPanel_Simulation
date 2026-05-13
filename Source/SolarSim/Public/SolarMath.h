#pragma once
// =============================================================================
//  SolarMath.h  —  Pure mathematical functions for solar simulation
//  No UE5 includes required; usable in unit tests without engine.
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "CoreMinimal.h"
#include <cmath>
#include "SolarMath.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Physical constants (단결정 Si 고정 파라미터)
// ─────────────────────────────────────────────────────────────────────────────
namespace SolarConst
{
    constexpr double ETA_0    = 0.20;     // 공칭 효율
    constexpr double K_T      = -0.004;   // 온도 계수 /°C
    constexpr double ETA_SYS  = 0.85;     // 시스템 손실
    constexpr double NOCT     = 45.0;     // Normal Operating Cell Temp
    constexpr double LAT_SEOUL = 37.5;    // 서울 위도 °N
    constexpr double SOLAR_PI = 3.14159265358979323846;
}

// ─────────────────────────────────────────────────────────────────────────────
// Input weather / state pack
// ─────────────────────────────────────────────────────────────────────────────
/* // (초기 코드)
struct FSolarWeatherParams
{
    double G   = 900.0;   // 일사량  [W/m²]
    double c   = 0.0;     // 흐림도  [0–1]
    double T   = 20.0;    // 기온    [°C]
    double v   = 2.0;     // 풍속    [m/s]
    int    doy = 172;     // day of year
    double hr  = 12.0;    // solar hour
};
*/
USTRUCT(BlueprintType)             // ← 추가
struct SOLARSIM_API FSolarWeatherParams
{
    GENERATED_BODY()               // ← 추가

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar")
    float G = 900.f;             // double → float

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar")
    float c = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar")
    float T = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar")
    float v = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar")
    int32 doy = 172;               // int → int32

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar")
    float hr = 12.f;
};

struct FSolarPanelAngles
{
    double theta = 35.0;  // 경사각 [°]
    double phi   = 0.0;   // 방위각 [°, 정남=0]
};

// ─────────────────────────────────────────────────────────────────────────────
// Namespace: solar math
// ─────────────────────────────────────────────────────────────────────────────
namespace SolarMath
{
    inline double D2R(double deg) { return deg * SolarConst::SOLAR_PI / 180.0; }
    inline double R2D(double rad) { return rad * 180.0 / SolarConst::SOLAR_PI; }
    inline double Clamp(double v, double lo, double hi)
        { return v < lo ? lo : (v > hi ? hi : v); }

    /** 태양 적위 [°] */
    inline double SolarDeclination(int doy)
    {
        return 23.45 * std::sin(D2R(360.0 / 365.0 * (doy - 81)));
    }

    /** 시간각 [°]  정오=0, 오후 양수 */
    inline double HourAngle(double solarHour)
    {
        return 15.0 * (solarHour - 12.0);
    }

    /**
     * 태양 고도각 [°]
     * sin αs = sin φlat·sin δ + cos φlat·cos δ·cos H
     */
    inline double SolarAltitude(int doy, double solarHour,
                                 double latDeg = SolarConst::LAT_SEOUL)
    {
        const double lat = D2R(latDeg);
        const double dec = D2R(SolarDeclination(doy));
        const double H   = D2R(HourAngle(solarHour));
        const double sa  = std::sin(lat) * std::sin(dec)
                         + std::cos(lat) * std::cos(dec) * std::cos(H);
        return R2D(std::asin(Clamp(sa, -1.0, 1.0)));
    }

    /** 태양 방위각 [°]  정남=0, 서쪽 음수 */
    inline double SolarAzimuth(int doy, double solarHour,
                                double latDeg = SolarConst::LAT_SEOUL)
    {
        const double lat   = D2R(latDeg);
        const double dec   = D2R(SolarDeclination(doy));
        const double alpha = D2R(SolarAltitude(doy, solarHour, latDeg));
        const double denom = std::cos(lat) * std::cos(alpha) + 1e-10;
        const double ca    = (std::sin(dec) - std::sin(lat) * std::sin(alpha))
                             / denom;
        const double az    = R2D(std::acos(Clamp(ca, -1.0, 1.0)));
        return (solarHour > 12.0) ? -az : az;
    }

    /**
     * cos(입사각) — 패널 법선과 태양 벡터의 내적
     * 음수(배면 입사)는 0으로 처리
     */
    inline double CosIncidence(double theta, double phi,
                                int doy, double solarHour)
    {
        const double alphaS = D2R(SolarAltitude(doy, solarHour));
        const double azS    = D2R(SolarAzimuth(doy, solarHour));
        const double thR    = D2R(theta);
        const double phR    = D2R(phi);

        // 패널 법선 벡터
        const double nx = std::sin(thR) * std::sin(phR);
        const double ny = std::sin(thR) * std::cos(phR);
        const double nz = std::cos(thR);

        // 태양 방향 단위 벡터
        const double sx = std::cos(alphaS) * std::sin(azS);
        const double sy = std::cos(alphaS) * std::cos(azS);
        const double sz = std::sin(alphaS);

        return Clamp(nx*sx + ny*sy + nz*sz, 0.0, 1.0);
    }

    /**
     * 패널 표면 온도 (NOCT model)
     * Tp = T + (NOCT-20)/800 · G·(1-0.85c) − 1.2v
     */
    inline double PanelTemperature(double T, double G, double c, double v)
    {
        const double Geff = G * (1.0 - 0.85 * c);
        return T + (SolarConst::NOCT - 20.0) / 800.0 * Geff - 1.2 * v;
    }

    /**
     * 목적 함수 P(θ, φ, G, c, T, v)
     * @return 단위 면적당 발전량 [W/m²]
     */
    inline double PowerOutput(double theta, double phi,
        const FSolarWeatherParams& w)
    {
        if (w.G <= 0.0) return 0.0;

        // ── 추가: 태양 고도각이 0° 이하면 발전량 0 ──────────────
        double sunAlt = SolarAltitude(w.doy, w.hr);
        if (sunAlt <= 0.0) return 0.0;
        // ─────────────────────────────────────────────────────────

        const double Geff = w.G * (1.0 - 0.85 * w.c);
        const double cosI = CosIncidence(theta, phi, w.doy, w.hr);
        const double Tp = PanelTemperature(w.T, w.G, w.c, w.v);
        const double eta = Clamp(SolarConst::ETA_0 * (1.0 + SolarConst::K_T * (Tp - 25.0)),
            0.01, 1.0);
        return Clamp(Geff * cosI * eta * SolarConst::ETA_SYS, 0.0, 300.0);
    }

    /** 연간 가중 기대 발전량 (Mode B) */
    inline double RobustAnnualPower(double theta, double phi,
                                     const TArray<TPair<FSolarWeatherParams, double>>& WeightedScenarios)
    {
        double total = 0.0;
        for (const auto& pair : WeightedScenarios)
            total += pair.Value * PowerOutput(theta, phi, pair.Key);
        return total;
    }
}
