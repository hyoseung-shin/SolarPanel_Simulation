// =============================================================================
//  USolarSimulationSubsystem.cpp
//  신효승 2023245022 | Modeling & Simulation 2026
// =============================================================================

#include "USolarSimulationSubsystem.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    SharedOptimizer = NewObject<USolarPanelOptimizer>(this);

    // Default: Seoul, summer noon
    GlobalWeather.G   = 900.0;
    GlobalWeather.c   = 0.05;
    GlobalWeather.T   = 28.0;
    GlobalWeather.v   = 2.0;
    GlobalWeather.doy = 172;
    GlobalWeather.hr  = 12.0;

    UE_LOG(LogTemp, Log, TEXT("[SolarSim] Subsystem initialized."));
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::Deinitialize()
{
    if (bRecording) StopRecording();
    RegisteredPanels.Empty();
    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    SyncSkyActorToSun();
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::RegisterPanel(ASolarPanelActor* Panel)
{
    if (Panel && !RegisteredPanels.Contains(Panel))
    {
        RegisteredPanels.Add(Panel);
        Panel->SetWeather(GlobalWeather);
        UE_LOG(LogTemp, Log, TEXT("[SolarSim] Panel registered. Total: %d"),
            RegisteredPanels.Num());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::UnregisterPanel(ASolarPanelActor* Panel)
{
    RegisteredPanels.Remove(Panel);
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::OptimizeAllPanels(EOptimizationMode Mode)
{
    for (ASolarPanelActor* Panel : RegisteredPanels)
    {
        if (Panel)
        {
            Panel->OptimMode = Mode;
            Panel->RunOptimization();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::SetGlobalWeather(const FSolarWeatherParams& Weather)
{
    GlobalWeather = Weather;
    for (ASolarPanelActor* Panel : RegisteredPanels)
        if (Panel) Panel->SetWeather(GlobalWeather);

    SyncSkyActorToSun();
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::SyncSkyActorToSun()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Find BP_Sky_Sphere or Directional Light tagged "SunLight"
    TArray<AActor*> Lights;
    UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), Lights);

    double Alt = SolarMath::SolarAltitude(GlobalWeather.doy, GlobalWeather.hr);
    double Az  = SolarMath::SolarAzimuth(GlobalWeather.doy, GlobalWeather.hr);

    for (AActor* Actor : Lights)
    {
        ADirectionalLight* DirLight = Cast<ADirectionalLight>(Actor);
        if (DirLight && DirLight->ActorHasTag(TEXT("SunLight")))
        {
            // UE5: Pitch=-altitude, Yaw = azimuth from North (north=0, east=90)
            // Our azimuth is from South (south=0, east=-90), so: yaw = 180 + Az
            FRotator SunRot(-(float)Alt, (float)(180.0 + Az), 0.f);
            DirLight->SetActorRotation(SunRot);

            // Scale intensity with altitude
            float Intensity = FMath::Clamp((float)Alt / 90.f, 0.f, 1.f) * 10.f;
            DirLight->GetLightComponent()->SetIntensity(Intensity);

            // Color: dawn/dusk orange → noon white
            float t = FMath::Clamp((float)Alt / 60.f, 0.f, 1.f);
            FLinearColor SunColor = FLinearColor::LerpUsingHSV(
                FLinearColor(1.f, 0.5f, 0.1f),  // orange
                FLinearColor(1.f, 0.97f, 0.9f), // white
                t);
            DirLight->GetLightComponent()->SetLightColor(SunColor);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Recording
// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::StartRecording()
{
    SimRecords.Empty();
    FrameIndex  = 0;
    bRecording  = true;
    RecordTimer = 0.f;
    UE_LOG(LogTemp, Log, TEXT("[SolarSim] Recording started."));
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::StopRecording()
{
    bRecording = false;
    UE_LOG(LogTemp, Log, TEXT("[SolarSim] Recording stopped. %d records."),
        SimRecords.Num());
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::Tick(float DeltaTime)
{
    if (!bRecording || RegisteredPanels.Num() == 0) return;

    RecordTimer += DeltaTime;
    if (RecordTimer < RecordInterval) return;
    RecordTimer = 0.f;

    LogRecord();
}

// ─────────────────────────────────────────────────────────────────────────────
void USolarSimulationSubsystem::LogRecord()
{
    ASolarPanelActor* Primary = RegisteredPanels.Num() > 0 ? RegisteredPanels[0] : nullptr;
    if (!Primary) return;

    FSimulationRecord R;
    R.FrameIndex  = FrameIndex++;
    R.SimTimeHour = (float)GlobalWeather.hr;
    R.DayOfYear   = GlobalWeather.doy;
    R.ThetaDeg = Primary->CurrentTheta;
    R.PhiDeg = Primary->CurrentPhi;
    R.PowerWm2    = Primary->GetCurrentPower();
    R.EffPct      = Primary->GetEfficiencyPct();
    R.SunAltDeg   = (float)SolarMath::SolarAltitude(GlobalWeather.doy, GlobalWeather.hr);
    R.G           = (float)GlobalWeather.G;
    R.CloudPct    = (float)(GlobalWeather.c * 100.0);
    R.TempC       = (float)GlobalWeather.T;

    SimRecords.Add(R);
    OnRecordLogged.Broadcast(R);
}
// ----------------------------------------------
void USolarSimulationSubsystem::RunOptimization()
{
    // 패널 없으면 중단
    if (RegisteredPanels.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("RunOptimization: 등록된 패널이 없습니다."));
        return;
    }

    // Optimizer 없으면 중단
    if (!SharedOptimizer)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("RunOptimization: SharedOptimizer가 null입니다."));
        return;
    }

    // 현재 날씨로 Mode A 최적화 실행
    FOptimizationResult Result = SharedOptimizer->OptimizeModeA(
        GlobalWeather,
        35.f,   // 초기 Theta
        0.f     // 초기 Phi
    );

    // 결과를 모든 패널에 적용
    for (ASolarPanelActor* Panel : RegisteredPanels)
    {
        if (!Panel) continue;

        /*FRotator NewRot = Panel->GetActorRotation();
        NewRot.Pitch = Result.ThetaOptDeg;
        NewRot.Yaw = Result.PhiOptDeg;
        Panel->SetActorRotation(NewRot);*/
        Panel->AnimateToAngles(Result.ThetaOptDeg, Result.PhiOptDeg, 2.0f);
    }

    UE_LOG(LogTemp, Log,
        TEXT("RunOptimization 완료 — Theta: %.1f°  Phi: %.1f°  Power: %.2f W/m²  반복: %d  수렴: %s"),
        Result.ThetaOptDeg,
        Result.PhiOptDeg,
        Result.PowerOptWm2,
        Result.Iterations,
        Result.bConverged ? TEXT("✅") : TEXT("❌"));
}

// ─────────────────────────────────────────────────────────────────────────────
// CSV Export
// ─────────────────────────────────────────────────────────────────────────────
FString USolarSimulationSubsystem::CSVHeader()
{
    return TEXT("Frame,SimTimeHour,DayOfYear,ThetaDeg,PhiDeg,PowerWm2,"
                 "EffPct,SunAltDeg,G_Wm2,CloudPct,TempC\n");
}

FString USolarSimulationSubsystem::RecordToCSVRow(const FSimulationRecord& R)
{
    return FString::Printf(
        TEXT("%d,%.3f,%d,%.3f,%.3f,%.4f,%.2f,%.2f,%.1f,%.1f,%.1f\n"),
        R.FrameIndex, R.SimTimeHour, R.DayOfYear,
        R.ThetaDeg, R.PhiDeg, R.PowerWm2, R.EffPct,
        R.SunAltDeg, R.G, R.CloudPct, R.TempC);
}

FString USolarSimulationSubsystem::ExportToCsv()
{
    FString Dir = FPaths::ProjectSavedDir() / TEXT("SolarSim");
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*Dir);

    FDateTime Now = FDateTime::Now();
    FString FileName = FString::Printf(TEXT("simulation_%s.csv"),
        *Now.ToString(TEXT("%Y%m%d_%H%M%S")));
    FString FilePath = Dir / FileName;

    FString Content = CSVHeader();
    for (const FSimulationRecord& R : SimRecords)
        Content += RecordToCSVRow(R);

    if (FFileHelper::SaveStringToFile(Content, *FilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("[SolarSim] CSV exported → %s (%d rows)"),
            *FilePath, SimRecords.Num());
        return FilePath;
    }
    UE_LOG(LogTemp, Error, TEXT("[SolarSim] CSV export FAILED → %s"), *FilePath);
    return TEXT("");
}

// ─────────────────────────────────────────────────────────────────────────────
FString USolarSimulationSubsystem::ExportAnnualAnalysis()
{
    FString Dir = FPaths::ProjectSavedDir() / TEXT("SolarSim");
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*Dir);

    FString FilePath = Dir / TEXT("annual_analysis.csv");

    const float MonthG[]  = {550,620,750,820,870,780,700,900,840,720,580,510};
    const float MonthC[]  = {.20f,.22f,.28f,.35f,.40f,.65f,.72f,.30f,.25f,.20f,.22f,.20f};
    const float MonthT[]  = {-3,0,7,14,20,24,27,28,22,14,5,-1};
    const float MonthV[]  = {3.5f,3.8f,3.2f,2.8f,2.5f,2.2f,2.0f,2.3f,2.6f,3.0f,3.5f,3.8f};
    const int   MonthDOY[]= {15,46,75,105,136,167,197,228,259,289,320,350};
    const char* MonthName[]= {"Jan","Feb","Mar","Apr","May","Jun",
                               "Jul","Aug","Sep","Oct","Nov","Dec"};
    // Angles to compare
    const float AngPairs[4][2] = {{35,0},{37,0},{30,0},{45,0}};

    FString Content = TEXT("Month,DayOfYear,G_Wm2,CloudPct,TempC,");
    for (auto& ap : AngPairs)
        Content += FString::Printf(TEXT("P_%.0fdeg_Wm2,"), ap[0]);
    Content += TEXT("BestAngle\n");

    for (int m = 0; m < 12; ++m)
    {
        FSolarWeatherParams W;
        W.G=MonthG[m]; W.c=MonthC[m]; W.T=MonthT[m];
        W.v=MonthV[m]; W.doy=MonthDOY[m]; W.hr=12.0;

        FString Row = FString::Printf(TEXT("%s,%d,%.0f,%.0f,%.0f,"),
            ANSI_TO_TCHAR(MonthName[m]), MonthDOY[m],
            MonthG[m], MonthC[m]*100, MonthT[m]);

        float Best = -1.f; float BestAngle = 35.f;
        for (auto& ap : AngPairs)
        {
            float P = (float)SolarMath::PowerOutput(ap[0], ap[1], W);
            Row += FString::Printf(TEXT("%.3f,"), P);
            if (P > Best) { Best = P; BestAngle = ap[0]; }
        }
        Row += FString::Printf(TEXT("%.0f\n"), BestAngle);
        Content += Row;
    }

    FFileHelper::SaveStringToFile(Content, *FilePath);
    UE_LOG(LogTemp, Log, TEXT("[SolarSim] Annual analysis → %s"), *FilePath);
    return FilePath;
}
