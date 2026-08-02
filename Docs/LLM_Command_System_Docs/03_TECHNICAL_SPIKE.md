# 3. 기술 스파이크 — Recon → Report → Secure

## 3.1 목적

전체 계층형 지휘 시스템을 구현하기 전에 가장 위험한 가정을 하나의 작은 수직 슬라이스로 검증한다.

검증할 핵심 파이프라인:

```text
Hardcoded Structured Command
→ Mission Resolver
→ Existing NPC Decision / CombatState
→ Behavior Tree Execution
→ Observation
→ Report
→ Team Operational Memory
→ Next Structured Command
→ Completion / Failure Result
```

첫 스파이크에서는 LLM을 사용하지 않는다. 하드코딩된 구조화 명령을 주입하여 실행 계층 자체를 검증한다.

## 3.2 검증 질문

1. `ReconArea` 같은 추상 임무를 기존 AI가 실제 행동으로 수행할 수 있는가?
2. 목표와 제약을 주입해도 개인 AI의 긴급 대응과 자율성이 유지되는가?
3. 관측 결과가 보고를 거쳐 Team Operational Memory에 반영되는가?
4. `SecureArea` 명령이 특정 지역과 접근 경로를 포함해 실행되는가?
5. 명령의 성공·실패를 구조적으로 판정할 수 있는가?
6. 모든 단계가 Command ID와 Event ID로 추적 가능한가?

## 3.3 테스트 월드 구성

### 팀

- US Team
- Soviet Team

### 아군

- US HQ 1개
- Recon Group 1개
- Combat Group A 1개
- Combat Group B 1개

### 적군

- Soviet Garrison Group 1개

### 월드 요소

- Objective Area 1개
- Route A 1개
- Route B 1개
- Observation Point 2~3개

### 고정 상태

- Route A: 바리케이드로 통행 불가능
- Route B: 통행 가능하지만 적에게 노출될 위험이 높음
- Observation Point High: 관측성 높음, 이동 비용·노출 위험 높음
- Observation Point Safe: 관측성 보통, 접근 안전성 높음
- Soviet Garrison: Objective Area 내부 또는 인접 지역에 배치

## 3.4 초기 명령 타입

### ReconArea

목적:

- 지정 지역의 요구 정보를 수집한다.
- 수집된 정보를 상위 HQ에 보고한다.

초기 정보 요구사항:

- Route A 통행 가능 여부
- Route B 통행 가능 여부
- Objective Area의 적 존재 여부

기본 제약:

- AvoidEngagement: Soft
- MaintainStealth: Soft 또는 Hard 테스트 가능
- LongRangePursuit 금지: Hard

성공 조건:

- 필수 정보 요구사항을 모두 충족
- 해당 Fact를 포함한 Report가 HQ에 Received 상태로 도달

실패 조건:

- 정찰 그룹 전투력 임계치 미만
- 모든 관측 후보에 접근 불가
- 필수 통신 수단 상실
- 제한 시간 초과
- 명령 취소

### SecureArea

목적:

- 지정 지역에 아군 전투 그룹을 진입시키고 통제 상태를 확보한다.

기본 제약:

- 지정된 회피 지역 침범 금지: Hard
- MinimizeCasualties: Soft
- 목표 지역과 무관한 장거리 추격 금지: Hard

성공 조건:

- 지정 그룹이 목표 지역에 도달
- 팀이 알고 있는 주요 적 위협이 제거·후퇴·무력화
- 일정 시간 동안 아군 점유 유지

실패 조건:

- 수행 그룹 전투력 임계치 미만
- 목표까지 접근 가능한 경로 없음
- 제한 시간 초과
- 상위 명령 취소

## 3.5 권장 최소 데이터 구조

실제 클래스명과 파일 위치는 코드 분석 결과에 맞춰 조정한다.

```cpp
UENUM()
enum class ECommandVerb : uint8
{
    Recon,
    Secure,
    Defend,
    Block
};

UENUM()
enum class ECommandTargetType : uint8
{
    Area,
    Route,
    Position,
    Unit,
    Information
};

UENUM()
enum class ECommandStatus : uint8
{
    Proposed,
    Validated,
    Assigned,
    Executing,
    Completed,
    Failed,
    Cancelled
};
```

```cpp
USTRUCT()
struct FCommandIntent
{
    GENERATED_BODY()

    FGuid CommandId;
    FGuid ParentCommandId;
    FName IssuerId;
    FName AssignedGroupId;
    ECommandVerb Verb;
    ECommandTargetType TargetType;
    FName TargetId;
    FVector TargetLocation;
    int32 Priority = 50;
    TArray<FCommandConstraint> Constraints;
    TArray<FInformationRequirement> InformationRequirements;
    FCommandCompletionCriteria CompletionCriteria;
    ECommandStatus Status;
};
```

```cpp
USTRUCT()
struct FMissionContext
{
    GENERATED_BODY()

    FGuid CommandId;
    FName ObjectiveId;
    FVector ObjectiveLocation;
    TArray<FCommandConstraint> HardConstraints;
    TArray<FCommandConstraint> SoftConstraints;
    TMap<FName, float> DecisionWeightModifiers;
    TArray<FInformationRequirement> InformationRequirements;
};
```

## 3.6 Mission Resolver 책임

Mission Resolver는 BT Task를 직접 실행하지 않는다.

출력:

1. Objective
2. Contextual Target
3. Hard Constraints
4. Soft Weight Modifiers
5. Completion Monitor 정보

예시:

```text
ReconArea
→ Objective: ObjectiveArea 정보 확보
→ Contextual Target: 선택된 ObservationPoint
→ Increase: MoveToObservation, Observe, Report
→ Decrease: Engage, Pursue
→ Hard: LongRangePursuit 금지
```

```text
SecureArea
→ Objective: ObjectiveArea 점유
→ Contextual Target: ObjectiveArea 또는 지정 Route
→ Increase: Advance, EngageThreat, OccupyCover
→ Decrease: Idle, UnrelatedPursuit
→ Hard: 목표 작전 구역 이탈 제한
```

## 3.7 관측 지점 선택

LLM 또는 명령 데이터가 좌표를 직접 생성하지 않는다.

게임 시스템이 관측 후보를 생성하고 점수를 계산한다.

후보 평가 요소:

- 목표 지역 가시성
- 고도
- NavMesh 접근 가능성
- 이동 거리 및 비용
- 적 노출 위험
- 탈출 경로
- 통신 품질

첫 스파이크에서는 Utility Score로 최고 후보를 선택해도 된다. 성격 기반 LLM 선택은 후속 단계다.

## 3.8 정보와 보고

관측 즉시 HQ가 알게 하지 않는다.

```text
Observe
→ Local Operational Fact
→ Report Created
→ Report Transmitting
→ Report Received or Failed
→ Team Operational Memory Update
```

초기 Fact Predicate:

- RoutePassable
- RouteBlocked
- EnemyPresent
- EnemyStrengthEstimated
- AreaControlled
- AreaContested

각 Fact는 최소한 다음을 가진다.

- Fact ID
- Subject ID
- Predicate
- Value
- Source ID
- Source Event ID
- Confidence
- Observed Time
- Received Time
- Expiration Time
- Information Scope

## 3.9 실행 로그

반드시 다음 이벤트를 기록한다.

- CommandProposed
- CommandValidated
- CommandAssigned
- CommandStarted
- ObservationCreated
- ReportCreated
- ReportReceived / ReportFailed
- FactAdded / FactUpdated
- CommandCompleted / CommandFailed

모든 이벤트는 Command ID, Group ID, Scenario Run ID를 포함한다.

## 3.10 구현 순서

### Phase A — 데이터와 디버그 출력

- Command 관련 enum 및 구조체 추가
- MissionContext 추가
- Command 상태 전이와 로그 추가
- 아직 NPC 실행과 연결하지 않음

### Phase B — ReconArea 하드코딩 실행

- 레벨 시작 또는 디버그 UI로 ReconArea 주입
- 관측 후보 선택
- 기존 이동·수색 AI와 연결
- Fact와 Report 생성
- Team Operational Memory 갱신

### Phase C — SecureArea 하드코딩 실행

- Recon 결과를 확인한 뒤 SecureArea 주입
- Combat Group A/B에 서로 다른 Target 또는 Route 설정
- 기존 전투 AI로 수행
- 성공·실패 판정

### Phase D — 전체 Trace 검증

- 하나의 Run ID로 모든 이벤트 추적
- 실패 원인이 구조화되어 기록되는지 확인
- 동일 초기 상태로 반복 실행 가능한지 확인

## 3.11 통과 기준

- 같은 `ReconArea` 코드가 다른 Area ID에도 재사용된다.
- 정찰 그룹이 관측 후보를 선택하고 실제 이동한다.
- 필수 정보가 보고되기 전에는 HQ 메모리에 반영되지 않는다.
- `SecureArea`가 목표 지역과 경로 정보를 기존 AI에 전달한다.
- 전투 중 긴급 대응 후 원래 임무로 복귀하거나 실패를 보고한다.
- Command 완료와 실패를 시스템이 판정한다.
- 모든 결과를 Command ID와 Run ID로 추적할 수 있다.

## 3.12 중단 및 재설계 기준

다음 중 하나가 발생하면 기능 확장을 중단하고 구조를 수정한다.

- 임무 하나를 위해 기존 BT 대부분을 다시 작성해야 함
- 대상 위치를 지정하면 개인 AI의 긴급 대응이 작동하지 않음
- 명령 성공·실패를 시스템이 판정하지 못함
- 상위 제약이 CombatState 또는 BT 단계에서 유실됨
- 하나의 명령이 상황마다 완전히 별도 코드가 필요함
- 실행 결과가 어떤 Command에서 발생했는지 추적할 수 없음
- LLM 없이도 파이프라인이 안정적으로 실행되지 않음

## Editor Integration Requirement

각 단계는 `06_EDITOR_INTEGRATION.md`를 따른다. Codex는 C++ 구현 후 다음을 `Generated/EDITOR_ACTIONS.md`에 기록한다.

- 생성 또는 수정이 필요한 Blueprint/DataAsset/Widget/Level Actor
- 부모 클래스와 설정할 프로퍼티
- Group ID, Team ID, Target ID, Route/Area/Observation Point 참조
- PIE 검증 순서와 기대 로그

BP나 레벨 연결 전 상태는 `Code Complete`일 뿐 기술 스파이크 통과가 아니다. 기술 스파이크 통과는 사용자가 에디터 연결을 마치고 PIE에서 전체 흐름을 확인한 이후다.
