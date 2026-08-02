# Phase 2 — Implementation Plan

작성일: 2026-08-02 (Asia/Seoul)  
기준: 현재 워킹트리, `BASELINE_STATUS.md`, `CODEBASE_FLOW_ANALYSIS.md`, Unreal MCP 검증 결과  
상태: **Plan Complete / Implementation Not Started**

> 이 문서는 구현 기준을 위한 기술 문서다. 용어나 설계가 어렵다면 먼저 [`LEARNING_GUIDE.md`](LEARNING_GUIDE.md)에서 게임 화면 기준 설명과 Unreal 개념 해설을 확인한다.

## 1. 확정된 설계 결정

사용자와 확인한 결정은 다음과 같다.

1. 시나리오의 NPC, 그룹, 목표, 경로, 관측 지점은 테스트 레벨에 한 번 수동 배치하고 저장한다. 반복 실행 때 다시 배치하지 않으며 `AScenarioInitializer`가 레벨 구성을 검증하고 초기화한다.
2. `UScenarioDefinition`은 `UPrimaryDataAsset`으로 만들고 초기 버전에서는 Scenario ID, 레벨, Seed, 실행 옵션 등 런타임 선택 정보만 소유한다. NPC 전체 Spawn 정의는 Phase 3 범위에 넣지 않는다.
3. 별도 메뉴 레벨 `Lvl_ScenarioMenu`를 사용한다.
4. `UScenarioRegistrySettings`를 Project Settings에 노출하여 실행 가능한 Scenario DataAsset을 명시적으로 등록한다. Custom GameInstance/BP는 만들지 않는다.
5. Scenario 선택값, Seed, Run ID처럼 레벨 전환을 넘어 유지해야 하는 값은 `UGameInstanceSubsystem`에 둔다.
6. Team Operational Memory는 `UWorldSubsystem`에 두어 레벨 전환 시 폐기한다.
7. `AGroupManagerActor`에 기존 `ARetryNPCCharacter::TeamID`와 같은 `uint8 TeamID`를 추가한다.
8. 임무 판단은 C++에서 수행한다. BT 실행에 필요한 최소 Actor/Location/제약 결과만 Blackboard에 전달한다.
9. Blackboard에 과거 C++ write-map의 누락 key를 일괄 추가하지 않는다. 실제 Mission 실행에 필요한 key만 Phase 5/6에서 검증 후 추가한다.
10. 현재 모든 CombatState decorator의 `Observer Aborts=None`은 알려진 후속 BT 부채로 유지한다. Phase 3/4 blocker로 취급하지 않는다.

## 2. 범위와 비범위

이 계획은 Phase 3부터 Phase 7까지의 변경 경계와 순서를 정한다. Phase 2에서는 게임 로직, 설정, Blueprint 또는 `.uasset`을 수정하지 않는다.

유지할 기존 실행 경로:

```text
AGroupManagerActor
→ UNPCDecisionComponent::SetOrder
→ utility score
→ ENPCCombatState
→ Blackboard CombatState 및 최소 실행 데이터
→ BT_LowIntelNPC
→ 기존 BT Service / Task
```

초기 스파이크에서 하지 않을 것:

- LLM이 좌표나 BT Task를 직접 생성하는 기능
- `UNPCGroupComponent`를 두 번째 그룹 런타임 시스템으로 완성하는 작업
- 범용 HTN planner 또는 모든 Command 조합
- Phase 3에서 NPC/그룹 전체를 DataAsset 기반으로 Spawn하는 기능
- 기존 BT 전체 재작성
- 누락된 Blackboard key의 일괄 생성
- Phase 7 이전의 LLM Command 생성 연결

## 3. 제안 아키텍처

```text
Lvl_ScenarioMenu / WBP_ScenarioSelect
  → UScenarioRuntimeSubsystem (GameInstance lifetime)
      → UScenarioRegistrySettings
      → UScenarioDefinition
      → OpenLevel

Loaded Test Level
  → AScenarioInitializer
      → validate placed actors and IDs
      → generate ScenarioRunId and apply Seed
      → reset world-scoped state
      → reset ULLMRequestQueue
      → start hardcoded scenario when enabled

Hardcoded FCommandIntent
  → FCommandValidator
  → AGroupManagerActor::AssignCommand
  → UMissionResolver
  → FMissionContext
  → UNPCDecisionComponent mission overlay
  → existing score / CombatState / Blackboard / BT

Observation
  → FOperationalFact (group/local scope)
  → FOperationalReport
  → report received gate
  → UTeamOperationalMemorySubsystem (World lifetime)
  → next command / completion monitor

All transitions
  → UScenarioExecutionLogSubsystem
  → ScenarioRunId / CommandId / EventId trace
```

### 수명과 소유권

| 데이터 | 소유자 | 수명 | 초기화 원칙 |
|---|---|---|---|
| 등록된 Scenario 목록 | `UScenarioRegistrySettings` | 프로세스/config | Project Settings에서 명시적 등록 |
| 선택 Scenario/Seed/옵션 | `UScenarioRuntimeSubsystem` | GameInstance | 메뉴 선택 때 교체, Return 때 안전하게 유지/해제 |
| Scenario Run ID | `UScenarioRuntimeSubsystem` | 한 실행 | 테스트 레벨 초기화 직전에 새 GUID 생성 |
| 레벨 배치 구성 | `AScenarioInitializer`와 레벨 Actor | World | 레벨 저장 상태를 기준으로 검증/복원 |
| Team Operational Memory | `UTeamOperationalMemorySubsystem` | World | World 생성 시 빈 상태, Run 시작 시 명시적 Reset 추가 확인 |
| 현재 Command/Mission | `AGroupManagerActor` | World | Run 시작/종료/취소 때 Clear |
| 개인/그룹 memory | 기존 `UMemoryComponent`, `AGroupManagerActor` | World Actor | 명시적 Reset API 추가 |
| LLM queue | 기존 `ULLMRequestQueue` | GameInstance | level transition 전에 Cancel/Reset |
| 실행 로그 | `UScenarioExecutionLogSubsystem` | GameInstance | Run namespace별 버퍼 분리, 새 Run 때 교체 |

## 4. Phase 3 — Scenario Level Loader

### 신규 파일

| 파일 | 타입/책임 |
|---|---|
| `Source/Retry/Scenario/ScenarioTypes.h` | `FScenarioLaunchOptions`, `FScenarioRunContext`, 초기화 결과 enum |
| `Source/Retry/Scenario/ScenarioDefinition.h/.cpp` | `UScenarioDefinition : UPrimaryDataAsset`; Scenario ID, 표시명, 설명, `TSoftObjectPtr<UWorld>`, 기본 Seed, LLM/로그/자동시작 기본값 |
| `Source/Retry/Scenario/ScenarioRegistrySettings.h/.cpp` | `UScenarioRegistrySettings : UDeveloperSettings`; 명시적으로 등록된 `TSoftObjectPtr<UScenarioDefinition>` 목록과 중복 ID 검증 |
| `Source/Retry/Scenario/ScenarioRuntimeSubsystem.h/.cpp` | `UGameInstanceSubsystem`; 목록 조회, 선택 상태, Start/Restart/Return, OpenLevel, Run ID, 안전한 오류 로그 |
| `Source/Retry/Scenario/ScenarioInitializer.h/.cpp` | 레벨 배치 Actor; BeginPlay 초기화, 레벨 구성 검증, Reset 호출, 자동 실행 진입점 |
| `Source/Retry/UI/ScenarioSelectWidget.h/.cpp` | `UUserWidget` C++ 부모; Blueprint UI가 호출할 최소 API와 목록 갱신 event |
| `Source/Retry/Tests/ScenarioRuntimeTests.cpp` | ID/registry/launch validation 및 Run context 자동화 테스트 |

### 수정 파일

| 파일 | 계획된 변경 |
|---|---|
| `Source/Retry/Retry.Build.cs` | `UDeveloperSettings` 사용을 위한 `DeveloperSettings` 모듈 의존성 추가 |
| `Source/Retry/LLMRequestQueue.h/.cpp` | `ResetQueueForScenarioTransition()` 추가: pending queue 비우기, timeout 해제, active request 취소/세대 무효화, late callback 적용 방지 |
| `Source/Retry/AI/GroupManagerActor.h/.cpp` | `TeamID`, `ResetGroupRuntimeState()` 추가; 레벨 배치 구성은 보존하고 memory/current mission만 초기화 |
| `Source/Retry/Components/MemoryComponent.h/.cpp` | `ResetMemories()` 추가 |
| `Config/DefaultEngine.ini` | 에디터 통합 후 `Lvl_ScenarioMenu`를 Game Default Map으로 지정. 기존 Editor Startup Map 변경은 별도 사용자 확인 후 수행 |
| `Config/DefaultGame.ini` | Project Settings에서 등록한 Scenario soft reference 저장 |

### Blueprint 노출 API 계획

- `UScenarioRuntimeSubsystem::GetRegisteredScenarios()` — `WBP_ScenarioSelect` 목록 표시용, 읽기 전용.
- `StartScenario(ScenarioId, LaunchOptions)` — Start 버튼용. 등록/soft level/ID를 검증한 뒤에만 전환한다.
- `RestartCurrentScenario()` — 인게임 디버그 UI용.
- `ReturnToScenarioMenu()` — 메뉴 복귀용.
- `GetCurrentRunContext()` — 디버그 표시용, 읽기 전용.
- `AScenarioInitializer::ValidateScenarioSetup()` — `CallInEditor`; 배치 Actor, ID, TeamID, 참조 누락 진단용.
- `UScenarioSelectWidget::BP_RefreshScenarioList()` — 레이아웃과 항목 생성은 BP가 담당하도록 하는 event.

### 레벨/에셋 통합 계획

- 사용자 에디터 작업으로 `Lvl_ScenarioMenu`, `WBP_ScenarioSelect`, 메뉴용 GameMode/Controller, `DA_TS_ReconSecure_001`을 생성한다.
- 기술 스파이크 레벨에는 NPC, `BP_GroupManager`, Objective/Route/Observation marker와 `AScenarioInitializer`를 한 번 배치한다.
- `AScenarioInitializer`는 Actor를 Spawn하지 않고 저장된 레벨 구성을 ID로 검증한다.
- 등록되지 않은 Scenario, 중복 Scenario ID, 잘못된 World soft reference는 크래시 없이 Start를 거부한다.

### Phase 3 테스트

1. Registry unit test: 빈 ID, 중복 ID, null definition/level 거부.
2. Start test: Scenario/Seed/옵션이 `OpenLevel` 전에 보존되는지 확인.
3. Initializer test: Run ID 생성, Seed 적용, 필수 배치 누락 진단.
4. Restart test: 새 Run ID가 생성되고 memory/command/log/LLM queue가 이전 Run을 참조하지 않는지 확인.
5. Return test: 메뉴 복귀 후 현재 실행 상태가 inactive인지 확인.
6. `RetryEditor Win64 Development` 빌드.
7. Editor 통합 뒤 PIE에서 Start → Restart → Return 반복.
8. 통합 완료 게이트에서 등록 map cook/package 열기 검증.

## 5. Phase 4 — Command Data and Trace

### 신규 파일

| 파일 | 타입/책임 |
|---|---|
| `Source/Retry/AI/CommandTypes.h` | `ECommandVerb`, `ECommandTargetType`, `ECommandStatus`, constraint/info/completion 타입, `FCommandIntent`, `FMissionContext`; `LLMTypes.h`와 분리 |
| `Source/Retry/AI/CommandValidation.h/.cpp` | 허용 verb-target 조합, ID, 대상, priority, constraint 모순 검증과 구조화 결과 코드 |
| `Source/Retry/Scenario/ScenarioExecutionLogSubsystem.h/.cpp` | Run/Command/Event ID가 포함된 구조화 이벤트 기록과 Run별 reset/export 경계 |
| `Source/Retry/Tests/CommandValidationTests.cpp` | 유효/무효 조합 및 상태 전이 테스트 |

### 수정 파일

| 파일 | 계획된 변경 |
|---|---|
| `Source/Retry/AI/GroupManagerActor.h/.cpp` | 현재 `FCommandIntent`/`FMissionContext` 소유, Assign/Cancel/Clear, 상태 전이와 로그 연결 |
| `Source/Retry/AI/NPCOrderTypes.h` | 기존 `ENPCOrder` 유지. 새 Command enum과 합치지 않는다. |
| `Source/Retry/LLMTypes.h` | Phase 4에서는 변경하지 않는다. Command 타입이 HTTP DTO에 결합되지 않도록 유지 |

### 상태 전이 규칙

```text
Proposed → Validated → Assigned → Executing → Completed
                                        ├→ Failed
                                        └→ Cancelled
```

- 역방향 전이는 거부한다.
- terminal 상태에서 재적용하지 않는다.
- 모든 전이는 Run ID, Command ID, Group ID, Event ID와 실패 이유를 기록한다.
- `ParentCommandId`가 없는 최상위 명령은 invalid GUID를 명시적으로 허용한다.

### Phase 4 테스트

- `Recon+Area`, `Recon+Route`, `Secure+Area`, `Defend+Position`, `Block+Route`만 초기 허용.
- 잘못된 group/target/priority/constraint 거부.
- terminal 상태 이중 완료와 잘못된 역전이 거부.
- 기존 `SetOrderForAll` 직접 호출 동작이 유지되는지 회귀 확인.
- C++ 빌드만으로 `Code Complete`; 이 단계의 기본 에디터 작업은 없음.

## 6. Phase 5 — ReconArea Vertical Slice

### 신규 파일

| 파일 | 타입/책임 |
|---|---|
| `Source/Retry/AI/MissionResolver.h/.cpp` | `FCommandIntent`를 target, hard constraint, weight modifier, completion data가 있는 `FMissionContext`로 변환 |
| `Source/Retry/AI/ScenarioMarkerTypes.h` | Area/Route/Observation의 안정적 ID와 공통 검증 타입 |
| `Source/Retry/AI/ObjectiveAreaActor.h/.cpp` | 레벨 배치 목표 영역과 Objective ID/점유 질의 |
| `Source/Retry/AI/RouteMarkerActor.h/.cpp` | Route ID, 경로 상태, 관련 marker 참조 |
| `Source/Retry/AI/ObservationPointActor.h/.cpp` | Observation ID, 가시성/위험/통신/비용 입력 |
| `Source/Retry/AI/ObservationPointSelector.h/.cpp` | Nav 접근성·거리·노출·가시성 기반 후보 점수 계산 |
| `Source/Retry/AI/OperationalTypes.h` | Fact/Report/predicate/status와 source/run/command ID 구조 |
| `Source/Retry/AI/TeamOperationalMemorySubsystem.h/.cpp` | team partition, Report Received gate, fact upsert/expiry/query/reset |
| `Source/Retry/AI/CommandExecutionMonitor.h/.cpp` | Recon completion/failure/timeout/cancel 판정 |

### 수정 파일

| 파일 | 계획된 변경 |
|---|---|
| `Source/Retry/AI/GroupManagerActor.h/.cpp` | `AssignCommand`, mission dispatch, report 생성, monitor 소유, group 전투력 질의 |
| `Source/Retry/Components/NPCDecisionComponent.h/.cpp` | optional `FMissionContext` overlay, clear/restore, hard constraint 검사, 기존 utility score에 soft modifier 합성 |
| `Source/Retry/NPCContext.h` | 현재 명령/임무에서 실제 점수 계산에 필요한 최소 입력만 추가 |
| `Source/Retry/BT/BTService_Decision.h/.cpp` | 기존 Update 진입점 유지; mission overlay가 같은 update 주기에 평가되도록 연결 |
| `Source/Retry/BT/BTTask_MoveToTarget.h/.cpp` | mission location 이동을 사용할 경우 request 결과와 completion lifecycle을 정상 처리하도록 보완 |
| `Source/Retry/Components/MemoryComponent.h/.cpp` | 관측을 개인/local fact로 변환할 최소 hook. 기존 감정 memory를 Team Memory로 복제하지 않음 |
| `Source/Retry/AI/GroupMemoryTypes.h` | group-local operational observation을 감정 event와 분리하거나 연결 ID만 추가 |

### Blackboard 원칙

- 판단 결과는 `UNPCDecisionComponent`와 `FMissionContext`가 권위 상태다.
- BT가 실행에 필요한 최소값만 쓴다. 예상 후보는 `MissionTargetActor`, `MissionTargetLocation`, `bMissionMovementAllowed`이나, 정확한 key와 타입은 Phase 5 시작 시 Editor asset을 재검증하고 사용자에게 확인받은 뒤 확정한다.
- `TargetActor`, `LastKnownEnemyLocation`, `CoverLocation`에 임무 목적을 억지로 덮어써 전투 의미를 오염시키지 않는다.
- mission 이동 후 긴급 `Dead`, `Reload`, visible threat 대응은 기존 우선순위를 유지한다.

### Recon 데이터 흐름

```text
hardcoded ReconArea
→ validate and assign to Recon Group
→ select placed ObservationPoint
→ mission target overlay
→ existing Decision/BT movement
→ observe required predicates
→ local/group fact
→ report Created/Transmitting/Received
→ Team Memory upsert only on Received
→ all requirements satisfied
→ Command Completed
```

### Phase 5 테스트

- 같은 command 코드가 다른 Objective ID에 재사용되는지 확인.
- 접근 가능한 후보 중 utility 최고값 선택.
- 모든 후보 path 실패 시 구조화 실패.
- Report Received 전 Team Memory가 비어 있는지 확인.
- timeout/group combat power/cancel 실패 이유 확인.
- 전투 긴급 대응 후 mission 복귀 또는 명시적 실패 확인.
- Command/Event/Fact/Report ID 연결성 확인.
- 기존 NPC 전투 및 Patrol 회귀 확인.

## 7. Phase 6 — SecureArea Vertical Slice

### 신규 파일

- 별도 resolver/task 세트를 만들지 않는다. `UMissionResolver`와 `UCommandExecutionMonitor`를 확장한다.
- 필요할 때만 `Source/Retry/AI/AreaControlEvaluator.h/.cpp`를 추가하여 목표 영역 점유, 위협, 유지 시간을 판정한다.

### 수정 파일

- `MissionResolver.h/.cpp`: `Secure+Area`를 Route/Objective/constraint/legacy order modifier로 변환.
- `GroupManagerActor.h/.cpp`: Combat Group A/B에 서로 다른 mission target/route 배정.
- `NPCDecisionComponent.h/.cpp`: 작전 구역 이탈과 장거리 추격 hard constraint 적용.
- `CommandExecutionMonitor.h/.cpp`: 도달, 위협 제거/후퇴/무력화, 점유 유지, 전투력/경로/timeout 실패 판정.
- marker/Team Memory 타입: Route 상태와 Area control fact 갱신.

### Phase 6 테스트

- A/B 그룹이 서로 다른 Route/target을 받는지 확인.
- Route A 차단/Route B 위험 정보가 Recon Report 이후에만 사용되는지 확인.
- 기존 BT 전면 재작성 없이 이동·엄폐·사격이 실행되는지 확인.
- 작전 구역 밖 장거리 추격이 hard constraint로 차단되는지 확인.
- 점유 유지 시간 이전에는 완료되지 않는지 확인.
- 그룹 전투력, no path, timeout, cancel을 서로 다른 failure code로 기록.

## 8. Phase 7 — Structured LLM Command

Phase 5와 6이 하드코딩 명령으로 Integrated Complete가 된 뒤에만 시작한다.

### 수정 중심 계획

- `LLMRequestQueue.h/.cpp`: HQ command request 타입, schema validation, generation guard, cancel/reset, doctrine fallback 추가.
- `LLMTypes.h`: HTTP transport DTO만 유지하고 `FCommandIntent` 자체는 참조 또는 변환한다.
- `GroupManagerActor.h/.cpp`: LLM 문자열 enum을 기존 `SetOrderForAll`로 즉시 적용하는 경로를 HQ structured command 경로로 분리. 기존 personality/group dialogue 기능은 회귀 방지용으로 유지.
- 신규 `CommandSchemaParser.h/.cpp`: JSON을 validation 전 DTO로 parse하며 actor pointer/좌표를 신뢰하지 않는다.

### 안전 규칙

- LLM은 semantic Target ID만 제안한다. 실제 Actor/Location은 registry/marker 조회로 해석한다.
- schema parse 실패, unknown ID, invalid verb-target, stale Run ID는 적용하지 않는다.
- timeout과 late callback이 같은 요청을 두 번 완료하지 못하게 한다.
- 실패 시 대사 fallback이 아니라 별도 doctrine command fallback을 사용한다.

## 9. 기존 시스템 재사용 요약

| 기존 시스템 | 재사용 방식 |
|---|---|
| `AGroupManagerActor` | 실제 그룹 authority/member registry/명령 fan-out 유지 |
| `UNPCDecisionComponent` | utility score와 state 안정화 유지, mission overlay만 추가 |
| `ENPCOrder` | Mission Resolver의 legacy tactical adapter 출력으로 유지 |
| `ENPCCombatState` | 실행 상태 vocabulary 유지 |
| `BT_LowIntelNPC` 및 C++ BT nodes | 원자 실행 계층 유지; mission용 최소 branch/key만 에디터 검증 후 추가 |
| `ARetryNPCCharacter::TeamID` | GroupManager와 Team Memory partition의 숫자 team identity로 재사용 |
| `UMemoryComponent` | 개인 경험 보존; Team Memory로 직접 합치지 않음 |
| `FGroupMemoryEvent` | 감정/목격 group memory 보존; Operational Fact와 역할 분리 |
| `ULLMRequestQueue` | Phase 7 transport 재사용, Phase 3에서 lifecycle 안전성 먼저 보강 |
| `ARetryPlayerController` UI 관례 | Widget 생성 관례만 참고; Scenario 메뉴는 별도 menu controller/widget로 격리 |

## 10. 공통 검증 게이트

각 구현 Phase는 다음 순서로 종료한다.

1. 관련 C++ 자동화/validation test.
2. `RetryEditor Win64 Development` 빌드.
3. 기존 `Lvl_ThirdPerson` NPC 전투 smoke test.
4. 해당 Phase의 `EDITOR_ACTIONS.md` 갱신.
5. 사용자 또는 MCP가 문서에 적힌 BP/DataAsset/level 연결 수행.
6. PIE 절차와 기대 로그 검증.
7. `Code Complete`와 `Integrated Complete`를 분리 보고.

Phase 3은 추가로 Game target 및 등록 map cook/package 검증을 완료 게이트에 포함한다.

## 11. 롤백 전략

- 새 기능은 `Scenario`, `Command`, `MissionContext`가 없으면 기존 AI 경로가 그대로 실행되도록 optional하게 추가한다.
- `UNPCDecisionComponent::ClearMissionContext()`는 모든 overlay를 제거하고 기존 `ENPCOrder`/utility 동작으로 복귀시킨다.
- Scenario menu 통합 전까지 기존 `GameDefaultMap=/Game/ThirdPerson/Lvl_ThirdPerson`을 유지한다. 통합 실패 시 config 한 항목만 복원한다.
- 새 Blackboard key/BT branch는 기존 branch를 제거하지 않고 별도 mission gate 아래 추가한다. asset 문제가 생기면 branch/key만 제거하여 기존 전투 트리를 복원한다.
- `UScenarioRuntimeSubsystem`과 Team Memory는 기존 GameMode/Character 생성 경로를 소유하지 않는다. subsystem 비활성/definition 누락 시 기존 레벨은 정상 시작하고 시나리오 자동 실행만 실패한다.
- `ULLMRequestQueue` lifecycle 보강은 기존 dialogue/group request public API를 보존한다.
- 사용자 dirty worktree의 기존 변경과 새 Phase 변경을 파일 단위로 구분하며, 되돌릴 때 unrelated 변경을 건드리지 않는다.

## 12. 위험과 대응

| 위험 | 대응 |
|---|---|
| BT movement task가 요청 직후 Success 또는 영구 InProgress | Phase 5 연결 전에 실제 사용 node lifecycle부터 수정하고 path failure test 추가 |
| `Observer Aborts=None`으로 state 변경이 늦게 반영 | 알려진 BT 부채로 추적; mission branch 연결 시 abort 정책을 사용자 확인 후 별도 수정 |
| combat target과 mission target 충돌 | 별도 mission key/context 사용, 기존 combat key 덮어쓰기 금지 |
| perception lost target cache가 mission 복귀를 막음 | Phase 5 preflight defect fix와 회귀 test로 분리 |
| group-less allied witness null 접근 | Phase 5 전 최소 guard fix |
| LLM queue가 level transition을 넘어 stale actor 참조 | Phase 3에서 cancel/reset/generation guard 선행 |
| 레벨 고정 배치가 Scenario DataAsset 값과 불일치 | `ValidateScenarioSetup()`로 ID/team/reference 중복·누락 진단 |
| package에서 soft level 누락 | Primary Asset/registered soft reference와 cook/package test로 검증 |

## 13. 아직 결정하지 않은 항목과 질문 게이트

아래 항목은 현재 Phase 2 아키텍처나 Phase 3 구현을 막지 않으므로 해당 Phase 시작 직전에 결정한다. Codex는 임의로 확정하지 않고 사용자에게 질문한다.

1. **Phase 3 UI 세부 구성**: `WBP_ScenarioSelect`의 표시 방식, 인게임 디버그 메뉴 입력 키와 배치.
2. **Phase 5 Blackboard schema**: mission 전용 key의 정확한 이름/타입과 BT branch/Observer Abort 정책.
3. **Report 전송 모델**: 첫 스파이크에서 즉시 수신, 고정 지연, 통신 actor/component 중 어느 수준까지 구현할지.
4. **Recon/Secure 수치 기준**: 제한 시간, 전투력 실패 임계치, 점유 유지 시간, observation utility 가중치.
5. **레벨 marker 세분화**: Objective/Route/Observation을 각각 Actor로 둘지 공통 base actor/component를 둘지 Phase 5 실제 레벨 구성 확인 후 확정.
6. **로그 export 형식과 위치**: JSON Lines, 단일 JSON, CSV 중 선택 및 저장 경로.
7. **Editor Startup Map 변경 여부**: Game Default Map은 메뉴로 변경하되 에디터 시작도 메뉴로 바꿀지는 통합 시 확인.

## 14. Phase 2 완료 체크리스트

- [x] 실제 클래스명과 파일명을 사용했다.
- [x] 신규 파일과 수정 파일을 구분했다.
- [x] 제안 아키텍처와 소유권/수명을 명시했다.
- [x] 기존 시스템 재사용 방법을 명시했다.
- [x] 데이터 흐름을 명시했다.
- [x] Phase별 테스트와 Editor Integration Gate를 명시했다.
- [x] 롤백 전략과 회귀 위험을 명시했다.
- [x] 현재 결정된 항목과 후속 질문 게이트를 구분했다.
- [x] 게임 로직, 설정, Blueprint, `.uasset`은 수정하지 않았다.

## 15. Phase 2 판정

**Phase 2 Complete / Phase 3 Ready / Implementation Not Started**

Phase 3 시작 시에는 먼저 신규/수정 예정 파일, 예상 위험, 검증 방법을 다시 제시하고, 13절의 Phase 3 UI 세부 구성 중 구현에 필요한 선택을 사용자에게 확인한다.
