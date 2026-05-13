#pragma once
// =============================================================================
//  USolarHUDWidget.h  — 수정본
//  수정: SyncWeatherToSubsystem() 헬퍼 추가
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/CanvasPanel.h"
#include "ASolarPanelActor.h"
#include "USolarSimulationSubsystem.h"
#include "USolarHUDWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SOLARSIM_API USolarHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ── Bound Widgets ──────────────────────────────────────────────────────
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UProgressBar*    PB_Irradiance;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UProgressBar*    PB_Cloudiness;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UProgressBar*    PB_Temperature;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UProgressBar*    PB_WindSpeed;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_IrradianceVal;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_CloudinessVal;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_TemperatureVal;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_WindSpeedVal;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) USlider*         SL_Irradiance;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) USlider*         SL_Cloudiness;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) USlider*         SL_Temperature;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) USlider*         SL_WindSpeed;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_DayOfYear;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_SolarHour;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_SolarAltitude;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) USlider*         SL_Hour;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) USlider*         SL_DayOfYear;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UButton*         BTN_PlayPause;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UComboBoxString* CB_OptimMode;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UButton*         BTN_Optimize;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UButton*         BTN_ExportCSV;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_ThetaResult;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_PhiResult;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_PowerResult;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_EfficiencyResult;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_IterationsResult;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_ConvStatus;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UProgressBar*    PB_EfficiencyMain;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UTextBlock*      TB_PowerMain;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UProgressBar*    PB_TempLoss;
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget)) UProgressBar*    PB_CloudLoss;

    // ── Panel reference ───────────────────────────────────────────────────
    UPROPERTY(BlueprintReadWrite, Category="Solar|HUD")
    ASolarPanelActor* TrackedPanel = nullptr;

    // ── Lifecycle ─────────────────────────────────────────────────────────
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ── Public API ────────────────────────────────────────────────────────

    /** Blueprint에서 마지막 SET 노드 대신 이 함수를 호출해야 합니다. */
    UFUNCTION(BlueprintCallable, Category="Solar|HUD")
    void SetTrackedPanelAndBind(ASolarPanelActor* NewPanel);

    UFUNCTION(BlueprintCallable, Category="Solar|HUD")
    void RefreshWeatherDisplay();

    UFUNCTION(BlueprintCallable, Category="Solar|HUD")
    void RefreshResultDisplay(const FOptimizationResult& Result);

    UFUNCTION(BlueprintCallable, Category="Solar|HUD")
    void DrawConvergenceCurve(const TArray<float>& PowerHistory);

    // ── Callbacks ─────────────────────────────────────────────────────────
    UFUNCTION() void OnOptimizeClicked();
    UFUNCTION() void OnExportCSVClicked();
    UFUNCTION() void OnPlayPauseClicked();
    UFUNCTION() void OnHourSliderChanged(float Value);
    UFUNCTION() void OnDaySliderChanged(float Value);
    UFUNCTION() void OnIrradianceSliderChanged(float Value);
    UFUNCTION() void OnCloudinessSliderChanged(float Value);
    UFUNCTION() void OnTemperatureSliderChanged(float Value);
    UFUNCTION() void OnWindSpeedSliderChanged(float Value);
    UFUNCTION() void OnOptimizationComplete(const FOptimizationResult& Result);
    UFUNCTION() void OnPowerUpdated(float PowerWm2, float EfficiencyPct);

private:
    bool bPlaying = false;
    TArray<float> LastPowerHistory;

    USolarSimulationSubsystem* GetSubsystem() const;

    /** [수정 추가] 패널 날씨를 Subsystem GlobalWeather에도 동기화 */
    void SyncWeatherToSubsystem();

    FLinearColor PowerToBarColor(float NormValue) const;
};
