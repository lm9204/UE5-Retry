# 4. 시나리오 레벨 로더 및 실행 UI

## 4.1 목적

기술 스파이크와 이후 프롬프트 실험을 반복하려면 동일한 초기 상황을 빠르게 재현할 수 있어야 한다.

현재처럼 매번 NPC, 그룹, 장애물, 목표물을 레벨에 수동 배치하면 다음 문제가 생긴다.

- 초기 상태 재현 비용이 큼
- 배치 실수로 테스트 조건이 달라짐
- 여러 시나리오를 비교하기 어려움
- 프롬프트 변경 전후 결과를 공정하게 비교하기 어려움
- Codex가 자동화 테스트나 반복 실행을 구성하기 어려움

따라서 초기에는 **간단한 레벨 선택 UI + 시나리오 초기화 진입점**을 구현한다.

## 4.2 초기 범위

첫 버전은 복잡한 에디터 도구가 아니다.

필수 기능:

1. 실행 가능한 테스트 레벨 목록 표시
2. 레벨 선택
3. 선택한 레벨 열기
4. 선택한 시나리오 ID 또는 설정 전달
5. 현재 레벨 재시작
6. 메인 시나리오 선택 화면으로 복귀
7. 실행 시 Scenario Run ID 생성

선택 기능:

- Seed 입력
- 자동 실행 여부
- 로그 저장 여부
- LLM 사용 / 하드코딩 명령 선택

## 4.3 권장 사용자 흐름

```text
Game Start
→ Scenario Select UI
→ Test Level 선택
→ Scenario Variant 선택
→ Start
→ OpenLevel
→ Scenario Initializer 실행
→ NPC / Group / Objective 초기화
→ Run ID 생성
→ 테스트 시작
```

테스트 중:

```text
Restart Scenario
Return to Scenario Menu
Export Run Log
```

## 4.4 레벨과 시나리오를 분리

가능하면 레벨 이름과 시나리오 설정을 분리한다.

```text
Level:
- 지형, 건물, NavMesh, 고정 월드 요소

Scenario Definition:
- 팀과 그룹 구성
- NPC Spawn Point
- 목표 지역
- 도로 상태
- 바리케이드 활성화
- 초기 메모리
- 초기 명령
- Random Seed
```

같은 레벨에서 여러 시나리오를 실행할 수 있어야 한다.

예:

```text
Level_TestValley
├─ Scenario_Recon_A
├─ Scenario_Recon_B
└─ Scenario_Secure_C
```

첫 스파이크에서 구현 비용이 크다면 레벨별 고정 배치로 시작해도 된다. 다만 UI와 실행 구조는 향후 Scenario Definition을 받을 수 있게 만든다.

## 4.5 데이터 구조 후보

코드베이스 분석 후 DataAsset 또는 DataTable 중 기존 프로젝트 관례에 맞는 것을 선택한다.

권장 후보:

```cpp
USTRUCT(BlueprintType)
struct FScenarioLevelEntry
{
    GENERATED_BODY()

    FName ScenarioId;
    FText DisplayName;
    TSoftObjectPtr<UWorld> Level;
    FText Description;
    int32 DefaultSeed;
    bool bUseLLMByDefault;
};
```

확장 후보:

```cpp
UCLASS(BlueprintType)
class UScenarioDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    FName ScenarioId;
    TSoftObjectPtr<UWorld> Level;
    int32 DefaultSeed;
    TArray<FScenarioGroupSpawn> Groups;
    TArray<FScenarioWorldOverride> WorldOverrides;
    TArray<FCommandIntent> InitialCommands;
};
```

## 4.6 책임 분리

### Scenario Select Widget

- 목록 표시
- 사용자 선택 수집
- Start / Restart / Return UI

### Scenario Registry

- 실행 가능한 Scenario Definition 목록 제공
- 에디터 전용 경로 탐색에 의존하지 않도록 명시적 등록 우선

### GameInstance 또는 Subsystem

- 선택한 Scenario ID 저장
- Seed와 LLM 사용 여부 저장
- 레벨 전환 후에도 설정 유지

### Scenario Initializer

- 레벨 BeginPlay 이후 선택된 시나리오 적용
- 그룹·NPC·목표·장애물 상태 초기화
- Team Memory 초기화
- Scenario Run ID 생성
- 로그 시스템 초기화
- 자동 테스트 시작

## 4.7 레벨 목록 생성 방식

초기에는 패키징 안정성을 위해 폴더 자동 검색보다 **DataAsset 또는 설정 배열에 명시적으로 등록된 레벨 목록**을 권장한다.

이유:

- 런타임 Asset Registry 차이 감소
- 개발·패키징 환경 결과 일치
- 테스트 대상이 아닌 레벨 노출 방지
- 표시 이름과 설명 관리 용이

후속으로 에디터 Utility를 만들어 지정 폴더의 레벨을 자동 등록할 수 있다.

## 4.8 시나리오 초기화 규칙

레벨 재실행 시 다음 상태가 반드시 초기화되어야 한다.

- Team Operational Memory
- Group Memory
- Personal Memory
- 현재 Command 및 MissionContext
- LLM 요청 큐
- Command / Decision / Event ID 카운터 또는 Run Namespace
- 그룹 전투력과 NPC 체력
- 바리케이드·도로 상태
- 목표 지역 통제 상태
- 랜덤 시드
- 디버그 UI와 로그 버퍼

레벨 전환만으로 자동 초기화되지 않는 GameInstance, Subsystem, static 상태를 코드 분석에서 반드시 찾는다.

## 4.9 재현성

각 실행은 다음 메타데이터를 기록한다.

```text
ScenarioRunId
ScenarioId
LevelName
Seed
Build or Commit Identifier
Prompt Version
Model Identifier
LLM Enabled
Start Time
End Time
Result
```

랜덤 요소는 가능한 한 Scenario Seed에서 파생한다.

완전한 결정론이 어렵더라도 최소한 다음은 고정 가능해야 한다.

- 초기 NPC 위치
- 그룹 편성
- 장애물 활성화 상태
- 초기 명령
- 지휘관 성격 값
- 초기 메모리

## 4.10 초기 UI 구성

최소 화면:

```text
Scenario Test Menu

[Scenario Dropdown/List]
[Description]
[Seed Input]
[Use LLM Checkbox]
[Enable Logging Checkbox]

[Start Scenario]
[Quit]
```

인게임 디버그 메뉴:

```text
[Restart Current Scenario]
[Return to Scenario Menu]
[Pause / Resume]
[Export Current Log]
```

## 4.11 기술 스파이크와의 연결

첫 등록 시나리오:

```text
ScenarioId: TS_ReconSecure_001
Level: 기술 스파이크용 테스트 레벨
Command Mode: Hardcoded
Seed: 1001
```

자동 진행 옵션:

1. BeginPlay 후 ReconArea 명령 주입
2. Recon 완료 및 HQ Report 수신 대기
3. 성공 시 SecureArea 명령 주입
4. 완료·실패 시 결과 로그 기록

UI에서 같은 시나리오를 반복 시작할 수 있어야 한다.

## 4.12 구현 순서

1. 현재 레벨 전환 구조 분석
2. Scenario Entry 데이터 정의
3. Scenario Select Widget 생성
4. GameInstance 또는 Subsystem에 선택 상태 저장
5. OpenLevel 연결
6. Scenario Initializer 생성
7. Restart / Return 기능
8. Run ID와 Seed 기록
9. 기술 스파이크 자동 시작 연결

## 4.13 통과 기준

- 게임 시작 후 UI에서 테스트 레벨을 선택할 수 있다.
- 선택한 레벨과 Scenario ID가 정확히 전달된다.
- Restart 시 이전 실행의 메모리·명령·로그 상태가 남지 않는다.
- 같은 Seed와 설정으로 초기 배치가 동일하게 재현된다.
- 기술 스파이크 시나리오를 수동 재배치 없이 반복 실행할 수 있다.
- 패키징 빌드에서도 등록된 테스트 레벨을 열 수 있다.

## 4.14 사용자 에디터 작업과 Codex 구현의 경계

이 기능은 C++만으로 완결되지 않을 가능성이 높다. `06_EDITOR_INTEGRATION.md`를 적용한다.

Codex는 Registry, Scenario Definition 타입, Subsystem, Initializer, 레벨 전환 API와 검증 로직을 구현한다. 사용자는 분석 결과에 따라 Scenario DataAsset, Widget Blueprint, 메뉴 GameMode/Controller 연결, 테스트 레벨의 Initializer와 Marker 배치를 수행한다.

Codex는 구현 후 `Generated/EDITOR_ACTIONS.md`에 다음을 정확히 기록한다.

1. 생성할 에셋 이름과 타입
2. 부모 C++ 클래스
3. 설정할 Soft Object Reference와 ID
4. 버튼 이벤트에서 호출할 함수
5. Project Settings 또는 World Settings 변경 여부
6. 테스트 레벨에 배치할 Actor/Volume/Marker
7. PIE 검증 절차

가능하면 `ValidateScenarioSetup()` 같은 `CallInEditor` 검증 함수를 제공하여 누락된 연결을 사용자가 빠르게 찾을 수 있게 한다.
