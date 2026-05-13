// =============================================================================
//  USolarHUDWidget.cpp  — 수정본
//  수정 사항:
//    1. 슬라이더 콜백이 TrackedPanel + Subsystem GlobalWeather 동시 갱신
//    2. float 타입 일관성 유지 (double 캐스팅 제거)
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "USolarHUDWidget.h"
#include "Kismet/GameplayStatics.h"

USolarSimulationSubsystem* USolarHUDWidget::GetSubsystem() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<USolarSimulationSubsystem>() : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (BTN_Optimize)  BTN_Optimize->OnClicked.AddDynamic(this,  &USolarHUDWidget::OnOptimizeClicked);
    if (BTN_ExportCSV) BTN_ExportCSV->OnClicked.AddDynamic(this, &USolarHUDWidget::OnExportCSVClicked);
    if (BTN_PlayPause) BTN_PlayPause->OnClicked.AddDynamic(this, &USolarHUDWidget::OnPlayPauseClicked);

    if (SL_Hour)        SL_Hour->OnValueChanged.AddDynamic(this,        &USolarHUDWidget::OnHourSliderChanged);
    if (SL_DayOfYear)   SL_DayOfYear->OnValueChanged.AddDynamic(this,   &USolarHUDWidget::OnDaySliderChanged);
    if (SL_Irradiance)  SL_Irradiance->OnValueChanged.AddDynamic(this,  &USolarHUDWidget::OnIrradianceSliderChanged);
    if (SL_Cloudiness)  SL_Cloudiness->OnValueChanged.AddDynamic(this,  &USolarHUDWidget::OnCloudinessSliderChanged);
    if (SL_Temperature) SL_Temperature->OnValueChanged.AddDynamic(this, &USolarHUDWidget::OnTemperatureSliderChanged);
    if (SL_WindSpeed)   SL_WindSpeed->OnValueChanged.AddDynamic(this,   &USolarHUDWidget::OnWindSpeedSliderChanged);

    if (CB_OptimMode)
    {
        CB_OptimMode->ClearOptions();
        CB_OptimMode->AddOption(TEXT("Mode A — 고정 날씨 2D 각도"));
        CB_OptimMode->AddOption(TEXT("Mode B — 연간 강건 최적화"));
        CB_OptimMode->AddOption(TEXT("Mode C — 6D 전체 최적화"));
        CB_OptimMode->SetSelectedIndex(0);
    }

    if (SL_Hour)        { SL_Hour->SetMinValue(0.f);   SL_Hour->SetMaxValue(24.f);    SL_Hour->SetValue(12.f); }
    if (SL_DayOfYear)   { SL_DayOfYear->SetMinValue(1.f); SL_DayOfYear->SetMaxValue(365.f); SL_DayOfYear->SetValue(172.f); }
    if (SL_Irradiance)  { SL_Irradiance->SetMinValue(0.f);  SL_Irradiance->SetMaxValue(1200.f); SL_Irradiance->SetValue(900.f); }
    if (SL_Cloudiness)  { SL_Cloudiness->SetMinValue(0.f);  SL_Cloudiness->SetMaxValue(1.f);    SL_Cloudiness->SetValue(0.05f); }
    if (SL_Temperature) { SL_Temperature->SetMinValue(-10.f); SL_Temperature->SetMaxValue(50.f); SL_Temperature->SetValue(28.f); }
    if (SL_WindSpeed)   { SL_WindSpeed->SetMinValue(0.f);  SL_WindSpeed->SetMaxValue(30.f);   SL_WindSpeed->SetValue(2.f); }

    // TrackedPanel은 NativeConstruct 시점에 아직 null일 수 있으므로
    // 델리게이트 바인딩은 SetTrackedPanelAndBind()에서만 수행합니다.

    RefreshWeatherDisplay();
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!TrackedPanel) return;

    const FSolarWeatherParams& W = TrackedPanel->CurrentWeather;
    float AltDeg = (float)SolarMath::SolarAltitude(W.doy, W.hr);

    if (TB_SolarHour)     TB_SolarHour->SetText(
        FText::FromString(FString::Printf(TEXT("%.2f h"), W.hr)));
    if (TB_DayOfYear)     TB_DayOfYear->SetText(
        FText::FromString(FString::Printf(TEXT("DOY %d"), W.doy)));
    if (TB_SolarAltitude) TB_SolarAltitude->SetText(
        FText::FromString(FString::Printf(TEXT("Alt %.1f deg"), AltDeg)));

    // 슬라이더 위치를 현재 날씨와 동기화 (자동 시간 진행 시 반영)
    if (SL_Hour)      SL_Hour->SetValue(W.hr);
    if (SL_DayOfYear) SL_DayOfYear->SetValue((float)W.doy);

    if (PB_EfficiencyMain)
    {
        float eff = TrackedPanel->GetEfficiencyPct() / 100.f;
        PB_EfficiencyMain->SetPercent(eff);
        PB_EfficiencyMain->SetFillColorAndOpacity(PowerToBarColor(eff));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// [수정 1] SetTrackedPanelAndBind — 델리게이트 바인딩의 유일한 진입점
// Blueprint에서 마지막 SET 노드 대신 이 함수를 호출해야 합니다.
// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::SetTrackedPanelAndBind(ASolarPanelActor* NewPanel)
{
    // 기존 패널 델리게이트 해제
    if (TrackedPanel)
    {
        TrackedPanel->OnOptimizationComplete.RemoveDynamic(
            this, &USolarHUDWidget::OnOptimizationComplete);
        TrackedPanel->OnPowerUpdated.RemoveDynamic(
            this, &USolarHUDWidget::OnPowerUpdated);
    }

    TrackedPanel = NewPanel;

    if (TrackedPanel)
    {
        // 델리게이트 바인딩
        TrackedPanel->OnOptimizationComplete.AddDynamic(
            this, &USolarHUDWidget::OnOptimizationComplete);
        TrackedPanel->OnPowerUpdated.AddDynamic(
            this, &USolarHUDWidget::OnPowerUpdated);

        RefreshWeatherDisplay();

        UE_LOG(LogTemp, Log, TEXT("[HUD] TrackedPanel 바인딩 완료"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::OnOptimizeClicked()
{
    if (!TrackedPanel)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUD] OnOptimizeClicked: TrackedPanel이 null입니다."));
        return;
    }

    int32 Idx = CB_OptimMode ? CB_OptimMode->GetSelectedIndex() : 0;
    EOptimizationMode Modes[] = {
        EOptimizationMode::ModeA_2DAngle,
        EOptimizationMode::ModeB_RobustAnnual,
        EOptimizationMode::ModeC_6DFull
    };
    TrackedPanel->OptimMode = Modes[FMath::Clamp(Idx, 0, 2)];

    UE_LOG(LogTemp, Log, TEXT("[HUD] 최적화 시작 — Mode=%d"), Idx);
    TrackedPanel->RunOptimization();
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::OnExportCSVClicked()
{
    USolarSimulationSubsystem* Sub = GetSubsystem();
    if (!Sub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUD] OnExportCSVClicked: Subsystem이 null입니다."));
        return;
    }

    FString Path = Sub->ExportToCsv();
    Sub->ExportAnnualAnalysis();

    if (TB_ConvStatus)
        TB_ConvStatus->SetText(FText::FromString(
            FString::Printf(TEXT("Exported: %s"), *FPaths::GetCleanFilename(Path))));

    UE_LOG(LogTemp, Log, TEXT("[HUD] CSV 내보내기 완료 → %s"), *Path);
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::OnPlayPauseClicked()
{
    if (!TrackedPanel) return;

    bPlaying = !bPlaying;
    TrackedPanel->bAutoAdvanceTime = bPlaying;

    USolarSimulationSubsystem* Sub = GetSubsystem();
    if (!Sub) return;

    if (bPlaying)
        Sub->StartRecording();
    else
        Sub->StopRecording();
}

// ─────────────────────────────────────────────────────────────────────────────
// [수정 2] 슬라이더 콜백 — Panel과 Subsystem 동시 갱신
// ─────────────────────────────────────────────────────────────────────────────

// 내부 헬퍼: 패널과 Subsystem에 날씨를 동시에 반영
void USolarHUDWidget::SyncWeatherToSubsystem()
{
    if (!TrackedPanel) return;
    USolarSimulationSubsystem* Sub = GetSubsystem();
    if (Sub)
        Sub->SetGlobalWeather(TrackedPanel->CurrentWeather);
}

void USolarHUDWidget::OnHourSliderChanged(float Value)
{
    if (!TrackedPanel) return;
    TrackedPanel->CurrentWeather.hr = Value;   // float 직접 대입
    SyncWeatherToSubsystem();
}

void USolarHUDWidget::OnDaySliderChanged(float Value)
{
    if (!TrackedPanel) return;
    TrackedPanel->CurrentWeather.doy = FMath::RoundToInt(Value);
    SyncWeatherToSubsystem();
}

void USolarHUDWidget::OnIrradianceSliderChanged(float Value)
{
    if (!TrackedPanel) return;
    TrackedPanel->CurrentWeather.G = Value;    // float 직접 대입
    SyncWeatherToSubsystem();

    if (TB_IrradianceVal) TB_IrradianceVal->SetText(
        FText::FromString(FString::Printf(TEXT("%.0f W/m2"), Value)));
    if (PB_Irradiance) PB_Irradiance->SetPercent(Value / 1200.f);
}

void USolarHUDWidget::OnCloudinessSliderChanged(float Value)
{
    if (!TrackedPanel) return;
    TrackedPanel->CurrentWeather.c = Value;    // float 직접 대입
    SyncWeatherToSubsystem();

    if (TB_CloudinessVal) TB_CloudinessVal->SetText(
        FText::FromString(FString::Printf(TEXT("%.0f%%"), Value * 100.f)));
    if (PB_Cloudiness) PB_Cloudiness->SetPercent(Value);
    if (PB_CloudLoss)  PB_CloudLoss->SetPercent(Value * 0.85f);
}

void USolarHUDWidget::OnTemperatureSliderChanged(float Value)
{
    if (!TrackedPanel) return;
    TrackedPanel->CurrentWeather.T = Value;    // float 직접 대입
    SyncWeatherToSubsystem();

    if (TB_TemperatureVal) TB_TemperatureVal->SetText(
        FText::FromString(FString::Printf(TEXT("%.1f C"), Value)));
    float TempLossFrac = FMath::Max(0.f, (Value - 25.f) * 0.004f);
    if (PB_TempLoss) PB_TempLoss->SetPercent(FMath::Clamp(TempLossFrac, 0.f, 1.f));
}

void USolarHUDWidget::OnWindSpeedSliderChanged(float Value)
{
    if (!TrackedPanel) return;
    TrackedPanel->CurrentWeather.v = Value;    // float 직접 대입
    SyncWeatherToSubsystem();

    if (TB_WindSpeedVal) TB_WindSpeedVal->SetText(
        FText::FromString(FString::Printf(TEXT("%.1f m/s"), Value)));
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::OnOptimizationComplete(const FOptimizationResult& Result)
{
    RefreshResultDisplay(Result);
    DrawConvergenceCurve(Result.PowerHistory);
}

void USolarHUDWidget::OnPowerUpdated(float PowerWm2, float EfficiencyPct)
{
    if (TB_PowerMain)
        TB_PowerMain->SetText(FText::FromString(
            FString::Printf(TEXT("P = %.1f W/m2"), PowerWm2)));
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::RefreshWeatherDisplay()
{
    if (!TrackedPanel) return;
    const FSolarWeatherParams& W = TrackedPanel->CurrentWeather;

    if (PB_Irradiance)  PB_Irradiance->SetPercent(W.G / 1200.f);
    if (PB_Cloudiness)  PB_Cloudiness->SetPercent(W.c);
    if (PB_Temperature) PB_Temperature->SetPercent((W.T + 10.f) / 60.f);
    if (PB_WindSpeed)   PB_WindSpeed->SetPercent(W.v / 30.f);

    if (TB_IrradianceVal)  TB_IrradianceVal->SetText(FText::FromString(FString::Printf(TEXT("%.0f W/m2"), W.G)));
    if (TB_CloudinessVal)  TB_CloudinessVal->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), W.c * 100.f)));
    if (TB_TemperatureVal) TB_TemperatureVal->SetText(FText::FromString(FString::Printf(TEXT("%.1f C"), W.T)));
    if (TB_WindSpeedVal)   TB_WindSpeedVal->SetText(FText::FromString(FString::Printf(TEXT("%.1f m/s"), W.v)));
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::RefreshResultDisplay(const FOptimizationResult& Result)
{
    if (TB_ThetaResult)      TB_ThetaResult->SetText(
        FText::FromString(FString::Printf(TEXT("Theta = %.2f deg"), Result.ThetaOptDeg)));
    if (TB_PhiResult)        TB_PhiResult->SetText(
        FText::FromString(FString::Printf(TEXT("Phi = %.2f deg"), Result.PhiOptDeg)));
    if (TB_PowerResult)      TB_PowerResult->SetText(
        FText::FromString(FString::Printf(TEXT("P = %.3f W/m2"), Result.PowerOptWm2)));
    if (TB_EfficiencyResult) TB_EfficiencyResult->SetText(
        FText::FromString(FString::Printf(TEXT("Eff = %.1f%%"),
            Result.PowerOptWm2 / (float)(SolarConst::ETA_0 * SolarConst::ETA_SYS * 1200.0) * 100.f)));
    if (TB_IterationsResult) TB_IterationsResult->SetText(
        FText::FromString(FString::Printf(TEXT("%d iters"), Result.Iterations)));
    if (TB_ConvStatus)       TB_ConvStatus->SetText(
        FText::FromString(Result.bConverged ? TEXT("Converged") : TEXT("Max iter reached")));

    LastPowerHistory = Result.PowerHistory;
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarHUDWidget::DrawConvergenceCurve(const TArray<float>& PowerHistory)
{
    LastPowerHistory = PowerHistory;
}

// ─────────────────────────────────────────────────────────────────────────────
FLinearColor USolarHUDWidget::PowerToBarColor(float NormValue) const
{
    float H = FMath::Lerp(240.f, 0.f, FMath::Clamp(NormValue, 0.f, 1.f));
    return FLinearColor(H, 0.9f, 0.8f).HSVToLinearRGB();
}
