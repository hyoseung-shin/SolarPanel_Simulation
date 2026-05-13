#pragma once
// =============================================================================
//  USolarSimulationSubsystem.h
//  World Subsystem: manages all panel actors, runs batch simulations,
//  exports results to CSV (Saved/ directory), drives sky/sun actor.
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "Tickable.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ASolarPanelActor.h"
#include "SolarPanelOptimizer.h"
#include "USolarSimulationSubsystem.generated.h"


// ─────────────────────────────────────────────────────────────────────────────
// CSV Row struct
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FSimulationRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32  FrameIndex = 0;
    UPROPERTY(BlueprintReadOnly) float  SimTimeHour = 0.f;
    UPROPERTY(BlueprintReadOnly) int32  DayOfYear   = 1;
    UPROPERTY(BlueprintReadOnly) float  ThetaDeg    = 0.f;
    UPROPERTY(BlueprintReadOnly) float  PhiDeg      = 0.f;
    UPROPERTY(BlueprintReadOnly) float  PowerWm2    = 0.f;
    UPROPERTY(BlueprintReadOnly) float  EffPct      = 0.f;
    UPROPERTY(BlueprintReadOnly) float  SunAltDeg   = 0.f;
    UPROPERTY(BlueprintReadOnly) float  G           = 0.f;
    UPROPERTY(BlueprintReadOnly) float  CloudPct    = 0.f;
    UPROPERTY(BlueprintReadOnly) float  TempC       = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecordLogged,
    const FSimulationRecord&, Record);

// ─────────────────────────────────────────────────────────────────────────────
// Subsystem
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class SOLARSIM_API USolarSimulationSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(
            USolarSimulationSubsystem, STATGROUP_Tickables);
    }
    virtual bool IsTickable() const override
    {
        return !IsTemplate();
    }
    // ── World Subsystem lifecycle ─────────────────────────────────────────

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    // ── Panel Registry ────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    void RegisterPanel(ASolarPanelActor* Panel);

    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    void UnregisterPanel(ASolarPanelActor* Panel);

    /** Run optimization on all registered panels */
    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    void OptimizeAllPanels(EOptimizationMode Mode);

    // ── Data Logging ──────────────────────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category="Solar|Subsystem")
    FOnRecordLogged OnRecordLogged;

    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    void StartRecording();

    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    void StopRecording();

    /** Export to CSV: <ProjectSaved>/SolarSim/simulation_YYYYMMDD_HHMMSS.csv */
    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    FString ExportToCsv();

    /** Export Mode B annual analysis */
    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    FString ExportAnnualAnalysis();

    UFUNCTION(BlueprintPure, Category="Solar|Subsystem")
    int32 GetRecordCount() const { return SimRecords.Num(); }

    // ── Global Weather ────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    void SetGlobalWeather(const FSolarWeatherParams& Weather);

    UFUNCTION(BlueprintPure, Category="Solar|Subsystem")
    FSolarWeatherParams GetGlobalWeather() const { return GlobalWeather; }

    /** Drive sky light actor azimuth/altitude from solar position */
    UFUNCTION(BlueprintCallable, Category="Solar|Subsystem")
    void SyncSkyActorToSun();

    // ------------수정-------------
    // void Tick(float DeltaTime);
    UFUNCTION(BlueprintCallable, Category = "Solar")
    void RunOptimization();

private:
    UPROPERTY() TArray<ASolarPanelActor*> RegisteredPanels;
    UPROPERTY() USolarPanelOptimizer*     SharedOptimizer = nullptr;

    FSolarWeatherParams GlobalWeather;
    TArray<FSimulationRecord> SimRecords;
    bool bRecording = false;
    int32 FrameIndex = 0;
    float RecordInterval   = 1.0f;  // seconds
    float RecordTimer      = 0.f;

    void LogRecord();
    static FString RecordToCSVRow(const FSimulationRecord& R);
    static FString CSVHeader();
};
