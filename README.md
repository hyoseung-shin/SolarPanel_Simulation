<div align="center">

# 🌞 태양광 패널 발전량 최적화 시뮬레이션

### Solar Panel Power Generation Optimization Simulation
**Unreal Engine 5.7 · C++17 · Gradient Ascent · 서울 365일 시뮬레이션**

<p>
<img src="https://img.shields.io/badge/Unreal%20Engine-5.7-black?logo=unrealengine"/>
<img src="https://img.shields.io/badge/C%2B%2B-17-blue?logo=cplusplus"/>
<img src="https://img.shields.io/badge/Algorithm-Gradient%20Ascent-orange"/>
<img src="https://img.shields.io/badge/License-MIT-green"/>
</p>

소프트웨어학부 · 2026학년도 모델링 및 시뮬레이션 · 신효승(2023245022)

</div>

---

## 🏆 핵심 성과

| 항목 | 결과 |
|------|------|
| **발전량 향상** | Mode A 최적화 시 초기 대비 **+47.9%** (84.65 → 125.16 W/m²) |
| **연간 최적 방위각** | φ = −16.156° (정남보다 약 16° 동쪽 편향) |
| **계절별 최적각 자동 학습** | 겨울 18.61° → 봄 37.57° → 여름 40.72° → 가을 29.36° |
| **365일 시뮬레이션** | Mode A / B / C 각각 완료, 총 약 11만 행 데이터 수집 |
| **알고리즘 안정성** | 모든 시나리오에서 발산 없이 수렴 확인 |

---

## 🚀 프로젝트 개요

태양광 패널의 발전량 최적화 문제를 **연속 수학적 최적화 문제로 재정의**하고, **Gradient Ascent 기반
수치 최적화 알고리즘**을 직접 구현하여 Unreal Engine 5.7의 실시간 3D 렌더링 환경과 통합한 인터랙티브
시뮬레이션입니다. 경사각(θ)과 방위각(φ)을 포함한 6개 물리 변수가 비선형으로 상호작용하는 목적 함수를
정의하고, 알고리즘이 **스스로 최적 각도를 찾아가는 과정을 실시간으로 시각화**합니다. 서울(위도 37.5°N)
기준 365일 연속 시뮬레이션을 완료했습니다.

### 주요 기술

- 6변수 비선형 발전량 목적 함수 + 물리 기반 태양 위치/패널 온도 모델
- 중앙 차분법 기반 Gradient Ascent 수치 최적화 엔진 (Mode A/B/C)
- 4가지 수치 안정화 기법 (Adaptive Scale · Boundary Clipping · Oscillation Damping)
- Substrate 머티리얼 · Niagara 파티클 · Lumen 조명 실시간 시각화
- C++ 연산 코어 ↔ Blueprint/UMG 표현 계층 역할 분리
- 365일 결과 CSV 자동 녹화 및 분석

### 전체 파이프라인 (매 틱)

```text
날씨 입력 (G, c, T, v)
    ↓
SolarMath  ──  태양 위치(δ, H, αs) · 패널 온도(NOCT) · 발전량 P 계산
    ↓
SolarPanelOptimizer  ──  ∇P(중앙 차분) → Gradient Ascent 1 step → 안정화
    ↓
ASolarPanelActor  ──  최적 θ, φ로 패널 Smooth-step 회전
    ↓
┌──────────────────────┬───────────────────────┬────────────────────┐
Substrate (발전량→색상)   Niagara (P→파티클)        Lumen (태양→조명 동기)
    ↓
USolarSimulationSubsystem  ──  CSV 녹화  →  HUD 위젯 갱신
```

---

## ⚡ Quick Start

```bash
git clone https://github.com/hyoseung-shin/SolarPanel_Simulation.git
cd SolarPanel_Simulation
```

1. `SolarSim.uproject` 우클릭 → **Generate Visual Studio project files**
2. `SolarSim.sln` 열기 → `Ctrl+Shift+B` (빌드)
3. `SolarSim.uproject` 더블클릭 → UE5 에디터 실행
4. `Window → World Settings → GameMode Override` → `BP_SolarGameMode`
5. ▶ Play — 알고리즘이 실시간으로 최적 각도를 탐색하는 과정을 관전

> 요구 환경: **Unreal Engine 5.7+**, **Visual Studio 2022**, **C++17**

---

## 📚 상세 문서

아래부터는 수학 모델, 최적화 알고리즘, 아키텍처, 365일 결과, 환경 설정 등 전체 문서입니다.

---

## 🗂️ 프로젝트 구조

```text
SolarPanel_Simulation/
├── SolarSim.uproject               UE5 프로젝트 파일
├── SolarSim.sln                    Visual Studio 솔루션
├── .vsconfig / .gitignore
│
├── Source/SolarSim/
│   ├── Public/
│   │   ├── SolarMath.h                  순수 수학 라이브러리 (태양 위치, NOCT, P 함수)
│   │   ├── SolarPanelOptimizer.h        Gradient Ascent 엔진 (Mode A/B/C)
│   │   ├── ASolarPanelActor.h           3D 패널 Actor
│   │   ├── USolarSimulationSubsystem.h  전역 관리자 (녹화, CSV)
│   │   └── USolarHUDWidget.h            UMG HUD 위젯
│   ├── Private/
│   │   ├── SolarPanelOptimizer.cpp
│   │   ├── ASolarPanelActor.cpp
│   │   ├── USolarSimulationSubsystem.cpp
│   │   └── USolarHUDWidget.cpp
│   └── SolarSim.Build.cs
│
├── Content/
│   ├── Blueprints/  ── BP_SolarPanelActor · BP_SolarGameMode
│   ├── Materials/   ── M_SolarPanel (Substrate: 발전량 → 색상)
│   ├── Effects/     ── NS_PowerGlow (Niagara)
│   └── UI/          ── WBP_SolarHUD (UMG)
│
├── Config/                         UE5 프로젝트 설정 (.ini)
└── Saved/SolarSim/                 시뮬레이션 결과 CSV (실행 시 자동 생성)
```

> 📎 수학 모델·Gradient Ascent를 동일하게 구현한 **Python 검증 스크립트**(`solar_engine.py`,
> `run_simulation.py`, `visualize.py`)를 별도 검증 레이어로 사용했습니다. 현재 본 저장소에는 UE5
> 구현(C++/Content)만 커밋되어 있으며, 필요 시 `python/` 폴더로 함께 포함할 수 있습니다.

---

## 🔢 수학적 모델링

### 목적 함수

단위 면적당 발전량 P는 6개 물리 변수의 비선형 연속 함수로 정의됩니다.

```text
P(θ, φ, G, c, T, v) = G · (1 − 0.85c) · cosθᵢ(θ,φ) · η₀ · [1 + kT(Tp − 25)] · ηsys · A
```

| 기호 | 변수 | 범위 | 구분 |
|------|------|------|------|
| θ | 경사각 | 0° ~ 90° | 최적화 대상 |
| φ | 방위각 (정남 기준) | −90° ~ +90° | 최적화 대상 |
| G | 일사량 | 0 ~ 1,200 W/m² | 날씨 입력 |
| c | 흐림도 | 0 ~ 1.0 | 날씨 입력 |
| T | 기온 | −10 ~ 50 °C | 날씨 입력 |
| v | 풍속 | 0 ~ 30 m/s | 날씨 입력 |
| η₀ | 공칭 효율 | 20% | 고정 상수 |
| kT | 온도 계수 | −0.4%/°C | 고정 상수 |
| ηsys | 시스템 효율 | 85% | 고정 상수 |

### 태양 위치 · 패널 온도 모델

```text
# 태양 적위 (Spencer, 1971)
δ = 23.45° × sin(360/365 × (DOY − 81))

# 시간각
H = 15° × (태양시 − 12)

# 태양 고도각 (Duffie & Beckman, 2013)
sin αs = sin φ_lat · sin δ + cos φ_lat · cos δ · cos H

# 패널 표면 온도 (NOCT 모델, IEC 61215)
Tp = T + (NOCT − 20) / 800 × G × (1 − 0.85c) − 1.2 × v
```

---

## ⚙️ 최적화 알고리즘

### Gradient Ascent

```text
x_{n+1} = x_n + η · ∇P(x_n) · scale
```

기울기 ∇P는 **중앙 차분법(Central Difference)** 으로 수치 계산합니다.

```text
∂P/∂xᵢ ≈ [P(x + h·eᵢ) − P(x − h·eᵢ)] / (2h)
```

### 4가지 안정화 기법

| 기법 | 수식 | 역할 |
|------|------|------|
| ① Central Difference | `[P(x+heᵢ)−P(x−heᵢ)] / 2h` | 정확한 기울기 계산 (O(h²)) |
| ② Adaptive Scale | `scale = min(1, 5 / (‖∇P‖ + ε))` | 기울기 폭발 방지 |
| ③ Boundary Clipping | `x = clip(x, lower, upper)` | 변수 범위 이탈 방지 |
| ④ Oscillation Damping | `‖Δx‖ < 0.3 → η ← η × 0.95` | 진동 감지 시 학습률 5% 감소 |

### 최적화 모드

| 모드 | 수식 | 설명 |
|------|------|------|
| **Mode A** | `argmax_{θ,φ} P(θ,φ \| G,c,T,v 고정)` | 고정 날씨 2D 최적화, 30초마다 자동 재실행 |
| **Mode B** | `argmax_{θ,φ} Σᵢ wᵢ·P(θ,φ\|ωᵢ)` | 연간 강건 최적화, 서울 12개월 가중 평균 |
| **Mode C** | `argmax_{θ,φ,G,c,T,v} P(x)` | 6D 전체 최적화, 이론적 최대 발전 잠재력 |

---

## 🏗️ 시스템 아키텍처

### C++ 클래스 의존 방향

```text
SolarMath.h → USolarPanelOptimizer → ASolarPanelActor → USolarSimulationSubsystem → USolarHUDWidget
   (수학)        (최적화 엔진)         (3D 오브젝트)        (전역 관리자)              (UI)

역방향 통신: Delegate (이벤트 브로드캐스트)만 허용
```

### C++ vs Blueprint 역할 분담

| 구분 | 방식 | 이유 |
|------|------|------|
| 수학 모델 (P, 태양 위치, NOCT) | **C++** | 매 틱 수천 회 호출 → 성능 필수 |
| Gradient Ascent 최적화 루프 | **C++** | 1,000회 이상 반복 연산 |
| 패널 각도 애니메이션 | **C++** | Smooth-step 보간 매 프레임 계산 |
| CSV 녹화·내보내기 | **C++** | 파일 I/O, 시스템 접근 |
| HUD 위젯 바인딩·콜백 | **C++** | 30개 위젯 이름 컴파일 타임 검증 |
| 메시·머티리얼·파티클 할당 | **Blueprint** | 코드 재컴파일 없이 에셋 교체 |
| 씬 초기화 흐름 | **Blueprint** | 실행 순서를 노드로 시각적 파악 |
| UMG 레이아웃·위젯 배치 | **UMG** | 드래그&드롭, 즉시 미리보기 |

---

## 🎮 실시간 시각화 스택

### Substrate 머티리얼 — 발전량 → 색상/발광 매핑

```text
0 W/m²   →  파란색 (저출력)
~80 W/m² →  노란색 (중출력)
200 W/m² →  흰색   (최대 출력)

PanelMID->SetVectorParameterValue("EmissiveColor", Col);
```

### Niagara 파티클 (NS_PowerGlow) — 3개 User Parameter 실시간 제어

- `User.PowerNormalized` → 파티클 스폰율
- `User.PowerColor` → 파티클 색상
- `User.SunDirection` → 파티클 흐름 방향
- UE5.7 MegaLights Beta 연동 → 씬 조명에 물리적 반영

### Lumen + DirectionalLight 동기화 — `SyncSkyActorToSun()` 매 틱 호출

- 태양 고도각·방위각에 따라 DirectionalLight 실시간 회전
- 광원 강도: 태양 고도에 비례
- 색온도: 일출·일몰 주황 → 정오 흰색 자동 전환

---

## 📊 시뮬레이션 결과 (365일)

### Mode A — 고정 날씨 2D 최적화

| 시점 | θ | φ | P (W/m²) |
|------|------|------|------|
| 최적화 전 (Frame 67) | 35.0° | 0.0° | 84.65 |
| 애니메이션 중 (Frame 79) | 11.9° | −12.7° | 114.98 |
| 수렴 완료 (Frame 87+) | **0.0°** | **−19.17°** | **125.16** |

> **발전량 향상: +47.9%** (하지 정오 기준)
> θ=0° 최적 이유: 서울 하지 정오 태양 고도각 ≈75.9° → cos(입사각) ≈ 0.97

### Mode B — 연간 강건 최적화

| 항목 | Mode A | Mode B | 차이 |
|------|------|------|------|
| 낮 평균 발전량 | 59.96 W/m² | **65.76 W/m²** | +9.7% |
| 겨울 평균 발전량 | 24.64 W/m² | **47.73 W/m²** | +93.7% |
| 여름 평균 발전량 | 78.66 W/m² | 78.19 W/m² | −0.6% |
| φ 표준편차 | 36.83° (불안정) | **0.00°** (완전 안정) | — |

### Mode C — 계절별 최적 θ 자동 학습

```text
겨울  18.61°  ████░░░░░░░░░░  P = 27.96 W/m²  태양 고도 낮음 → 패널 세움
봄    37.57°  ████████░░░░░░  P = 42.15 W/m²  태양 고도 상승, θ 증가
여름  40.72°  █████████░░░░░  P = 55.17 W/m²  고도 최고 → θ 가장 큼
가을  29.36°  ██████░░░░░░░░  P = 29.19 W/m²  고도 하강, θ 감소
```

> 별도 물리 규칙 주입 없이 Gradient Ascent가 계절적 물리 법칙을 **자동 학습**

---

## 🛠️ 개발 환경 설정

### 필수 환경

| 항목 | 버전 |
|------|------|
| Unreal Engine | 5.7 이상 |
| Visual Studio | 2022 |
| C++ 표준 | C++17 |

### UE5 프로젝트 설정

1. 저장소 클론 — `git clone https://github.com/hyoseung-shin/SolarPanel_Simulation.git`
2. `SolarSim.uproject` 우클릭 → **Generate Visual Studio project files**
3. `SolarSim.sln` 열기 → `Ctrl+Shift+B` (빌드)
4. `SolarSim.uproject` 더블클릭 → UE5 에디터 실행
5. `Edit → Project Settings → Maps & Modes`
   - Editor Startup Map: `SolarSimMap`
   - Game Default Map: `SolarSimMap`
6. `Window → World Settings → GameMode Override`: `BP_SolarGameMode`

---

## 🎛️ 시뮬레이션 파라미터

`BP_SolarPanelActor` → **Class Defaults**에서 조정합니다.

| 파라미터 | 기본값 | 설명 |
|------|------|------|
| `TimeScaleFactor` | 3600 | 시뮬레이션 배속 (3600 = 1초당 1시간) |
| `bContinuousOptimization` | true | 주기적 자동 재최적화 활성화 |
| `OptimizationInterval` | 30.0 | 자동 재최적화 주기 (초) |
| `OptimMode` | ModeA_2DAngle | 최적화 모드 선택 (A/B/C) |

### 365일 시뮬레이션 권장 설정

```text
TimeScaleFactor         = 3600   (실제 약 2시간 26분 소요)
RecordInterval          = 1.0f   (USolarSimulationSubsystem.h에서 수정)
bContinuousOptimization = true
OptimizationInterval    = 30.0
```

---

## 📁 CSV 출력 형식

결과는 `[ProjectDir]/Saved/SolarSim/`에 자동 저장됩니다 (`simulation_YYYYMMDD_HHMMSS.csv`, `annual_analysis.csv`).

| 컬럼 | 설명 | 컬럼 | 설명 |
|------|------|------|------|
| `Frame` | 프레임 인덱스 | `PowerWm2` | 발전량 (W/m²) |
| `SimTimeHour` | 시뮬레이션 시각 (h) | `EffPct` | 효율 (%) |
| `DayOfYear` | 연중 일수 (1~365) | `SunAltDeg` | 태양 고도각 (°) |
| `ThetaDeg` | 현재 경사각 (°) | `G_Wm2` | 일사량 (W/m²) |
| `PhiDeg` | 현재 방위각 (°) | `CloudPct` | 흐림도 (%) |
|  |  | `TempC` | 기온 (°C) |

---

## 🔧 알려진 이슈 · 🗺️ 향후 계획

| 이슈 | 원인 | 상태 |
|------|------|------|
| 야간 발전량 잔존 오류 | RecordInterval=1.0h 경계의 일출·일몰 샘플링 오차 | 확인됨 (Mode C에서 0.13%로 감소) |
| Mode A 겨울철 φ 불안정 | 일출·일몰 경계에서 지역 최적해로 수렴 | Adam Optimizer로 개선 예정 |

- [ ] **Mode B 다중 시간대 확장** — 정오 단일 시점 → 6–18시 전체 적분 (예상 최적각 33–37°)
- [ ] **Adam Optimizer 도입** — Mode A 지역 최적해 탈출, 수렴 속도 2~3배 향상
- [ ] **야간 경계 조건 오류 완전 해결** — 샘플링 간격 정밀화
- [ ] **실시간 기상 API 연동** — OpenWeatherMap → 서울 현재 날씨 자동 반영
- [ ] **멀티패널 어레이 모델링** — 패널 간 상호 음영(self-shading) 기하 모델

---

## 📚 참고문헌

| 수식 / 모델 | 출처 |
|------|------|
| 발전량 목적 함수 P(·) | King et al. (2004), *Sandia PV Array Performance Model*, SAND2004-3535 |
| 유효 일사량 감쇠 계수 0.85 | Hay, J.E. (1979), *Solar Energy*, 23(4), 301-307 |
| 태양 적위 | Spencer, J.W. (1971), *Search*, 2(5), 172 |
| 태양 고도각 | Duffie & Beckman (2013), *Solar Engineering of Thermal Processes*, 4th ed. |
| NOCT 패널 온도 모델 | Skoplaki & Palyvos (2009), *Solar Energy*, 83(5) / IEC 61215:2021 |
| Gradient Ascent | Ruder, S. (2016), *arXiv:1609.04747* |
| 중앙 차분법 | Burden & Faires, *Numerical Analysis*, 10th ed. |
| Adaptive Scale | Pascanu et al. (2013), *arXiv:1211.5063* |

---

## 📄 라이선스 · 작성자

본 프로젝트는 **MIT License**를 따릅니다 (`LICENSE` 파일 참조).

**신효승** · 소프트웨어학부 2023245022 · 2026학년도 모델링 및 시뮬레이션

---

<div align="center">

Built with Unreal Engine 5.7 · C++17 · Gradient Ascent

</div>
