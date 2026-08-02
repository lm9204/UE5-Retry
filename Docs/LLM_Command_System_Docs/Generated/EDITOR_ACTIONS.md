# Phase 0/1 — User Editor Verification Actions

Phase 0/1에서는 에셋을 생성하거나 수정하지 않는다. 아래는 사용자가 Unreal Editor에서 **열어 보고 확인할 항목만** 정리한 체크리스트다.

## Phase 2 — Implementation Plan

### Codex가 작성한 것

- `Generated/IMPLEMENTATION_PLAN.md`에 Phase 3~7의 아키텍처, 클래스 책임, 신규/수정 파일, 데이터 흐름, 테스트 및 롤백 전략을 작성했다.
- 레벨 고정 배치 재사용, 별도 `Lvl_ScenarioMenu`, Project Settings 기반 Scenario 등록, C++ 판단과 최소 Blackboard 실행 데이터, `uint8 TeamID`, World-scoped Team Memory 결정을 반영했다.

### 사용자 에디터 작업

사용자 에디터 작업 없음. Phase 2는 계획 확정 단계이며 Blueprint, DataAsset, Level, Project Settings를 수정하지 않는다.

### 검증 절차

1. `IMPLEMENTATION_PLAN.md`의 확정 결정과 신규/수정 파일 구분을 확인한다.
2. 후속 질문 게이트가 Phase별로 분리되었는지 확인한다.
3. 소스 코드와 `.uasset`이 Phase 2에서 변경되지 않았는지 확인한다.

### 현재 통합 상태

- [x] Phase 2 계획 문서 작성
- [x] 사용자 설계 결정 반영
- [x] 사용자 에디터 작업 없음
- [x] Implementation Not Started

분류:

- **확인된 사실**: 에셋 파일 존재, C++ 노출 property, config, 과거 로그로 확인된 범위
- **코드에 근거한 추론**: C++에 생성 경로가 없어 level 배치로 보이는 경우 등. Editor에서 확정 필요
- **현재 확인할 수 없는 항목**: `.uasset` 내부 저장값, graph, level instance reference, 현재 PIE 결과. 아래 절차의 직접 확인 대상

## 현재 통합 상태

- [x] **Code Analysis Complete**
- [x] **Build Verification Complete** — Development Editor
- [x] **Editor Verification Performed via Unreal MCP** — 2026-08-02
- [x] **Implementation Not Started**
- [x] BP/asset reference 확인
- [x] level placement 확인
- [x] current working tree PIE 확인

## 1. Maps & Modes / Game framework chain

### 확인 대상

- 타입/이름: Project Settings > Maps & Modes
- 예상 경로: `/Game/ThirdPerson/Lvl_ThirdPerson`, `/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode`
- 부모 클래스: `BP_ThirdPersonGameMode`의 부모가 `ARetryGameMode`인지 확인

### 확인할 프로퍼티 또는 참조

- Editor Startup Map, Game Default Map, Global Default GameMode
- GameInstance Class가 기본인지
- BP GameMode의 Default Pawn Class, Player Controller Class, HUD Class, Game State Class
- `Lvl_ThirdPerson` World Settings의 GameMode Override

### 확인할 레벨 및 배치 Actor

- `Lvl_ThirdPerson`을 연 상태에서 World Settings 확인

### 확인할 Blueprint 이벤트 또는 함수 연결

- `BP_ThirdPersonGameMode` 또는 Level Blueprint의 BeginPlay에서 widget, NPC, manager를 추가 생성하는지

### PIE 절차와 기대 결과

1. `Lvl_ThirdPerson`을 연다.
2. Output Log를 비운 뒤 PIE를 시작한다.
3. 로그의 `Game class is`가 의도한 GameMode인지 확인한다.

기대 결과: `BP_ThirdPersonGameMode_C`와 의도한 Pawn/Controller가 생성되고 시작 오류가 없다.

실패 시: World Settings override, Maps & Modes, BP compile errors, default class assignments를 확인한다.

코드만으로 확인할 수 없는 이유: default GameMode path는 config에서 확인되지만 BP 내부 class defaults와 per-level override는 `.uasset`에 저장된다.

## 2. NPC Character Blueprint

### 확인 대상

- 타입/이름: Character Blueprint `BP_RetryNPCCharacter`
- 경로: `/Game/AI/Blueprints/BP_RetryNPCCharacter`
- 부모 클래스: `ARetryNPCCharacter`

### 확인할 프로퍼티 또는 참조

- AI Controller Class = `BP_RetryNPCController`
- Auto Possess AI = Placed in World or Spawned(또는 의도한 동등값)
- TeamID, DefaultPersonality, bIsHighIntelligence
- DefaultWeapon/DefaultAmmo, StartingItems
- FloatingNameWidgetClass = `WBP_FloatingName`
- AIDebugWidgetClass = `WBP_AIDebug`
- DialogueWidgetClass = `WBP_Dialogue`
- Health/Combat/Inventory/Personality/Weapon/Memory/Dialog components 존재

### 확인할 레벨 및 배치 Actor

- `Lvl_ThirdPerson`에 배치된 네 NPC instance
- 각 instance의 NPCName, TeamID, MyGroup, bIsGroupLeader, PatrolPoints

### 확인할 Blueprint 이벤트 또는 함수 연결

- Construction Script/Event Graph에서 C++ BeginPlay/group registration을 중복 수행하거나 값을 덮어쓰는지

### PIE 절차와 기대 결과

1. 네 NPC 각각의 instance 값을 기록한다.
2. PIE 시작.
3. 네 번의 `[NPC] ... BeginPlay`와 controller BT start 로그를 확인한다.

기대 결과: 네 NPC가 의도한 controller에 possess되고 정확한 group/leader로 한 번씩 등록된다.

실패 시: AIControllerClass, Auto Possess AI, MyGroup soft/actor reference, group actor validity, BP compile status를 확인한다.

코드만으로 확인할 수 없는 이유: instance-only references와 BP class defaults는 C++에 없다.

## 3. NPC Controller / Behavior Tree

### 확인 대상

- 타입/이름: AIController Blueprint `BP_RetryNPCController`
- 경로: `/Game/AI/Blueprints/BP_RetryNPCController`
- 부모 클래스: `ARetryNPCController`
- 참조 에셋: `/Game/AI/BT_LowIntelNPC`

### 확인할 프로퍼티 또는 참조

- `BehaviorTreeAsset` = `BT_LowIntelNPC`
- Perception component/sight config의 BP overrides

### 확인할 레벨 및 배치 Actor

- 별도 controller actor 배치는 불필요해야 하며 Character possession으로 생성되는지 확인

### 확인할 Blueprint 이벤트 또는 함수 연결

- BP OnPossess가 parent call을 누락하지 않는지
- perception 이벤트를 BP가 별도로 처리/덮어쓰는지

### PIE 절차와 기대 결과

1. AI Debugger/Behavior Tree debugger에서 각 NPC를 선택한다.
2. `BT_LowIntelNPC` 실행과 `UBTService_Decision` 반복 tick을 확인한다.
3. target acquire/loss 때 blackboard 변화를 관찰한다.

기대 결과: possess 직후 BT 실행, CombatState 주기 갱신, target 사망 시 target clear.

실패 시: BehaviorTreeAsset, Auto Possess AI, controller class, service 배치, parent OnPossess를 확인한다.

코드만으로 확인할 수 없는 이유: 실제 BehaviorTreeAsset assignment와 graph는 Blueprint/Behavior Tree asset에 저장된다.

## 4. Blackboard와 CombatState enum

### 확인 대상

- Blackboard `/Game/AI/BB_NPC`
- Enum `/Game/AI/E_CombatState`
- native enum `ENPCCombatState` (`Source/Retry/NPCContext.h`)

### 확인할 프로퍼티 또는 참조

- `CombatState` key의 exact type
- native 순서와 asset enum 순서가 다음과 정확히 같은지:
  `Idle, Patrol, Alert, Search, Attack, TakeCover, Reload, Retreat, Hold, Suppress, Dead`
- 모든 C++ key 존재/type:
  `TargetActor(Object/Actor)`, `CombatState(Enum)`, `bAlerted`, `bCanSeeTarget`, `LastKnownEnemyLocation`, `CoverLocation`, `StimulusLocation`, `MoveDestination`, `PatrolIndex`, `DistanceToTarget`, `bIsLowHP`, `bIsOutOfAmmo`, `bIsReloading`, `Aggression`, `Fear`, `Trust`, `TrustBias`, `Loyalty`

### 확인할 레벨 및 배치 Actor

- 없음. AI debugger에서 runtime key 값 확인.

### 확인할 Blueprint 이벤트 또는 함수 연결

- BT decorators가 `CombatState` 값에 어떤 비교를 하는지 기록
- observer abort 설정과 branch 우선순위 기록

### PIE 절차와 기대 결과

1. AI debugger로 한 NPC blackboard를 연다.
2. 평시/적 발견/탄약 소진/엄폐/후퇴에서 enum 값과 branch를 비교한다.

기대 결과: C++ enum 이름과 BT branch 의미가 일치하며 missing key warning이 없다.

실패 시: key 이름 대소문자, type, BP enum ordinal, BT decorator value를 확인한다.

코드만으로 확인할 수 없는 이유: C++은 문자열 key와 ordinal만 쓰며 Blackboard schema/enum binding은 asset 내부다.

## 5. Behavior Tree graph와 Task 사용 현황

### 확인 대상

- Behavior Tree `/Game/AI/BT_LowIntelNPC`
- 부모/기반 Blackboard: `/Game/AI/BB_NPC`

### 확인할 프로퍼티 또는 참조

- root 또는 항상 실행되는 composite에 `BTService_Decision`이 붙어 있는지
- CombatState 11종 branch 중 실제 branch 수
- 다음 C++ Task 18종의 배치/미사용 여부를 각각 기록:
  AimAtTarget, CallOut, ChargeEnemy, CloseDistance, FireAtTarget, FireFromCover, LookAround, MoveToCover, MoveToLastKnown, MoveToPatrolPoint, MoveToTarget, Overwatch, Reload, Retreat, SearchArea, SelectCombatRoute, SuppressiveFire
- 각 movement task 뒤에 완료 확인 node가 있는지
- target loss/path failure decorator와 abort policy

### 확인할 레벨 및 배치 Actor

- `Lvl_ThirdPerson` NPC로 live debugging

### 확인할 Blueprint 이벤트 또는 함수 연결

- Blueprint Task/Service가 C++ 목록 외에 존재하는지

### PIE 절차와 기대 결과

1. 한 NPC의 active branch를 평시부터 전투 종료까지 추적한다.
2. target loss 후 Search/Patrol/Idle 복귀를 확인한다.
3. NavMesh 밖 목표 또는 차단 상황에서 branch가 정지하지 않는지 확인한다.

기대 결과: CombatState 변화가 올바른 branch abort/선택으로 이어지고 BT가 영구 InProgress에 갇히지 않는다.

실패 시: `AimAtTarget`/`MoveToCover` latent finish 부재, movement request result, decorator abort, missing BB key를 확인한다.

코드만으로 확인할 수 없는 이유: Task 클래스 코드는 읽을 수 있지만 실제 graph 배치와 composite semantics는 asset에 있다.

## 6. GroupManager와 group instance 연결

### 확인 대상

- Actor Blueprint `/Game/AI/Blueprints/BP_GroupManager`
- 부모 클래스: `AGroupManagerActor`
- 예상 level instances: Group A, Group B

### 확인할 프로퍼티 또는 참조

- 각 actor의 GroupID, GroupEmotionThreshold
- level NPC `MyGroup` references와 bIsGroupLeader
- 각 group에 leader 정확히 1명, member 총 2명인지

### 확인할 레벨 및 배치 Actor

- `Lvl_ThirdPerson` Outliner의 GroupManager 두 actor와 NPC 네 actor

### 확인할 Blueprint 이벤트 또는 함수 연결

- leader death가 `OnLeaderDied`를 호출하도록 BP에서 연결돼 있는지
- 별도 group creation/disband/event graph가 있는지

### PIE 절차와 기대 결과

1. PIE 시작 직후 Group A/B register 로그를 확인한다.
2. leader를 사망시킨다.
3. `OnLeaderDied` 로그와 `HoldFire, Weight=0` 전파 여부를 확인한다.

기대 결과: 중복 등록 없이 각 group 2명, leader death 처리 연결.

실패 시: MyGroup reference, leader flag, BP death event, C++ delegate 연결 부재를 확인한다.

코드만으로 확인할 수 없는 이유: C++ `ARetryNPCCharacter::OnDeath`는 `OnLeaderDied`를 호출하지 않으므로 BP 연결 여부가 유일한 미확인 경로다.

## 7. NavMesh / patrol / spawn

### 확인 대상

- `Lvl_ThirdPerson`
- Actor types: `NavMeshBoundsVolume`, `BP_PatrolPoint`, PlayerStart, 네 NPC, 두 GroupManager

### 확인할 프로퍼티 또는 참조

- NavMeshBoundsVolume extent와 모든 NPC/cover/search 영역 coverage
- NPC capsule/agent radius와 crowd navigation compatibility
- each PatrolPoints array의 valid index/order
- runtime spawned NPC class는 cheat manager에 설정됐는지

### 확인할 Blueprint 이벤트 또는 함수 연결

- Level Blueprint/initializer/spawner가 있는지. 현재 C++ scenario initializer는 없음.

### PIE 절차와 기대 결과

1. `P`로 navmesh visualization.
2. patrol NPC가 모든 point에 도달하는지 확인.
3. combat cover/search/retreat 이동 실패 로그와 정지 여부를 확인.

기대 결과: 이동 후보가 green nav area에 있고 path request가 정상 완료된다.

실패 시: NavMesh bounds, agent size, collision, zero vector keys, disconnected islands, patrol array null을 확인한다.

코드만으로 확인할 수 없는 이유: navigation data와 level geometry는 cooked/generated asset 상태다.

## 8. UI 및 fallback DataTable

### 확인 대상

- PlayerController BP `/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController`
- Widgets `/Game/UI/WBP_Inventory`, `WBP_Loot`, `WBP_FloatingName`, `WBP_AIDebug`, `WBP_Dialogue`
- DataTable `/Game/UI/DT_FallbackDialogue`

### 확인할 프로퍼티 또는 참조

- Inventory/Loot/Mobile widget classes와 Enhanced Input actions/mapping contexts
- NPC widget classes
- DataTable row struct = `FFallbackDialogueRow`, 각 `EPersonalityType` row 존재

### 확인할 레벨 및 배치 Actor

- PlayerController는 GameMode에서 지정; widget actor 배치 불필요

### 확인할 Blueprint 이벤트 또는 함수 연결

- Widget Blueprint가 native events/functions에 올바르게 binding되는지
- GameMode/Level Blueprint가 별도 HUD를 생성하는지

### PIE 절차와 기대 결과

1. inventory/loot UI가 기존대로 생성되고 toggle되는지 확인.
2. `ToggleAIDebug`와 high-intelligence dialogue를 확인.
3. LLM server를 끈 테스트는 사용자가 의도적으로 수행할 때만 진행하고 fallback dialogue를 확인.

기대 결과: missing class/table error 없이 기존 UI와 fallback이 표시된다.

실패 시: BP class assignment, DataTable row struct, hard path, bIsHighIntelligence를 확인한다.

코드만으로 확인할 수 없는 이유: widget class assignment, designer bindings, DataTable rows는 assets에 저장된다.

## 9. 현재 워킹트리 PIE 최종 절차

1. 위 1~8의 참조를 저장 변경 없이 확인한다.
2. Output Log를 비운다.
3. `Lvl_ThirdPerson` PIE를 시작한다.
4. GameMode, 네 BT start, Group A/B registration을 캡처한다.
5. 한 교전을 끝까지 관찰해 CombatState transition과 group/personal memory를 확인한다.
6. group threshold에 도달하면 LLM request/order flow를 확인한다. LLM 서버가 없으면 failure/fallback만 기록하고 성공으로 간주하지 않는다.
7. PIE 종료 후 crash/assert/access violation이 없는지 확인한다.
8. 결과와 로그 경로를 이 문서 checklist에 반영한다.

### 기대 결과

- 기존 전투/BT가 동작한다.
- Group A/B가 독립 등록된다.
- order response가 도착한 경우 `SetOrderForAll` 후 score/CombatState 변화가 관찰된다.
- target loss/사망 후 BT가 정상 상태로 복귀한다.

### 실패 시 우선 확인

- native/BP enum mismatch
- missing Blackboard key
- `BehaviorTreeAsset`/AIControllerClass/Auto Possess AI
- `MyGroup` null 또는 wrong group reference
- group-less allied witness null path
- latent BT task finish 부재
- LLM timeout 뒤 late callback

## 10. Phase 2 진입 전 사용자 확인 목록

- [x] Maps & Modes와 World Settings 기록
- [x] BP class parent/default reference 기록
- [x] `BB_NPC` key/type 전체 대조 — C++ 내부 판단 구조에 따른 의도된 최소 schema임을 사용자 확인
- [x] `E_CombatState` ordinal 대조 — 11개 표시 순서 및 BT 사용 ordinal 일치
- [x] `BT_LowIntelNPC` graph/task 사용표 작성
- [x] Group A/B와 네 NPC instance property 확인
- [x] NavMesh bounds와 초기 NPC 포함 여부 확인 — 실제 동적 path coverage 전체는 Phase 2/3 PIE에서 재검증
- [x] current working tree PIE baseline 실행 — 전투 흐름 통과, LLM 서버 미접속으로 order 적용은 미검증
- [x] packaging 검증 필요 여부 결정 — Phase 2 전에는 불필요, Scenario Loader가 들어가는 Phase 3 완료 게이트에서 수행
- [x] 확인 결과의 처리 방침 승인 — Blackboard는 현행 유지, BT abort 개선은 후속 작업

## 11. 2026-08-02 Unreal MCP 직접 검증 결과

### 검증 방식

- Unreal MCP의 Asset/Blueprint/Object/Scene/BehaviorTree/Editor/Logs toolset으로 현재 열린 에디터 세션을 읽었다.
- `.uasset`을 수정하거나 저장하지 않았다.
- `Lvl_ThirdPerson`에서 PIE를 직접 시작하고 종료했으며, 현재 세션 로그를 조회했다.

### 확인 완료 — Game Framework와 Blueprint defaults

- 현재 레벨: `/Game/ThirdPerson/Lvl_ThirdPerson`.
- World Settings `DefaultGameMode`: `BP_ThirdPersonGameMode_C`.
- `BP_ThirdPersonGameMode` parent: `/Script/Retry.RetryGameMode`.
- GameMode defaults:
  - Default Pawn: `BP_ThirdPersonCharacter_C`
  - Player Controller: `BP_ThirdPersonPlayerController_C`
  - HUD: native `AHUD`
  - GameState: native `AGameStateBase`
  - Seamless Travel: false
- `BP_ThirdPersonPlayerController` parent는 `ARetryPlayerController`이며 Inventory/Loot/Mobile widget과 Enhanced Input mapping이 실제 할당돼 있다.
- `BP_RetryNPCCharacter` parent는 `ARetryNPCCharacter`; `AIControllerClass=BP_RetryNPCController_C`, `AutoPossessAI=PlacedInWorldOrSpawned`, 세 widget class와 기본 weapon/ammo가 실제 할당돼 있다.
- `BP_RetryNPCController` parent는 `ARetryNPCController`; `BehaviorTreeAsset=BT_LowIntelNPC`가 실제 할당돼 있다.
- `BP_GroupManager` parent는 `AGroupManagerActor`다.
- 위 GameMode/NPC/NPCController/GroupManager Blueprint의 Construction Script와 EventGraph는 모두 비어 있다. 따라서 BP가 C++ BeginPlay를 중복 호출하지 않으며, 리더 사망을 `OnLeaderDied`로 연결하는 BP 경로도 없다.

### 확인 완료 — 레벨 인스턴스

| Actor | Team/Group | Leader | Patrol |
|---|---|---:|---|
| `기회주의자` | Team 1 / Group A | yes | 2 points |
| `지원가` (NPCName `아나`) | Team 1 / Group A | no | 2 points |
| `화난놈` | Team 2 / Group B | yes | none |
| `쫄보` | Team 2 / Group B | no | none |

- `BP_GroupManager_A`: GroupID `A`, threshold `0.6`.
- `BP_GroupManager_B`: GroupID `B`, threshold `0.6`.
- NavMeshBoundsVolume world bounds는 X/Y `-2000..2000`, Z `-500..500`; 네 NPC의 초기 X/Y 위치는 모두 bounds 안이다.
- NavMesh 생성 로그에는 serialized maxTiles 200과 calculated 75 불일치로 RecastNavMesh 재생성 경고가 있다. 초기 전투 이동은 실행됐지만 전체 작전 영역 path coverage가 검증됐다는 뜻은 아니다.

### 확인 완료 — Blackboard와 enum

실제 `BB_NPC` key는 정확히 다음 11개다.

```text
SelfActor, Aggression, Fear, Trust, TargetActor, PatrolIndex,
CoverLocation, LastKnownEnemyLocation, StimulusLocation, bAlerted, CombatState
```

- `TargetActor`/`SelfActor`: Object, base class `Actor`.
- `CoverLocation`/`LastKnownEnemyLocation`/`StimulusLocation`: Vector.
- `CombatState`: Enum `/Game/AI/E_CombatState`.
- enum editor 표시 순서는 C++과 동일하다: `Idle, Patrol, Alert, Search, Attack, TakeCover, Reload, Retreat, Hold, Suppress, Dead`.
- BT decorator의 실제 비교값도 사용 branch에서 C++ ordinal과 일치한다: Patrol 1, Alert 2, Search 3, Attack 4, TakeCover 5, Reload 6, Retreat 7, Suppress 9, Dead 10.

### 설계 확인 — C++ 내부 판단과 최소 Blackboard schema

다음 C++ 사용 key는 `BB_NPC`에 실제로 존재하지 않는다.

```text
bCanSeeTarget, MoveDestination, DistanceToTarget,
bIsLowHP, bIsOutOfAmmo, bIsReloading,
TrustBias, Loyalty
```

사용자 확인 결과, 기존 Blackboard 중심 판단에서 C++ 내부 판단으로 이전했고 Blackboard에는 상태와 현재 BT 실행에 필요한 값만 전달하도록 바꾼 구조다. 따라서 위 key가 asset에 없는 사실만으로 schema 결함이나 Phase 2 blocker로 취급하지 않는다.

Phase 2에서는 누락 key를 일괄 추가하지 않는다. 남아 있는 C++ write가 실제로 불필요한 legacy write인지 확인하고, 새 mission 기능이 소비하는 최소 key만 근거가 있을 때 추가한다.

### 확인 완료 — Behavior Tree graph

- Blackboard: `BB_NPC`.
- root: Selector; `BTService_Decision`은 root selector 범위에 있고 interval `0.1`, deviation `0.02`다.
- 실제 branch: Dead, Reload, Retreat, TakeCover, Attack, Suppress, Search, Alert, Patrol, Idle.
- `Hold` branch는 없다.
- 실제 task: Reload, Retreat, native MoveTo(CoverLocation), FireAtTarget, SuppressiveFire, MoveToLastKnown, Overwatch, SearchArea, TurnToStimulus, CallOut, MoveToPatrolPoint, LookAround, Wait.
- 모든 CombatState Blackboard decorator의 `FlowAbortMode`가 `None`이다. 상태가 바뀌어도 실행 중 branch를 observer abort하지 않으므로 latent/task lifecycle 위험이 분석 문서의 추측이 아니라 에셋에서 확인된 사실이다.
- 사용자 확인 결과 현재 구현을 변경하는 중인 알려진 과도기 상태다. 이번 Phase 2 진입을 막지 않고 후속 BT 정리 작업으로 추적한다.

### 현재 워킹트리 PIE 결과

PIE 시작 시각: 2026-08-02 19:19 KST 상당 (`Retry.log` UTC 표기 `10:19`).

- 네 NPC 모두 `BT_LowIntelNPC` 실행 확인.
- Group A/B 모두 leader 1 + member 1, 총 2명씩 한 번 등록.
- 실제 상태 전이: Idle→Reload, Reload→Patrol/Attack, Attack→TakeCover, 전투 종료 후 Reload/Idle 또는 Patrol.
- 사격, 피해, NPC 사망, 개인/그룹 memory 기록 확인.
- Group B threshold 초과 후 LLM group request가 생성됐다.
- localhost LLM 응답은 `bSuccess:0, RespValid:0`로 실패했고 기존 fallback이 실행됐다. 따라서 `SetOrderForAll` 이후 상태 변화는 이번 실행에서 검증하지 못했다.
- `LogBlueprint`, `LogBehaviorTree`, `LogAIModule` 오류는 없었고 PIE는 MCP로 정상 종료했다.

### Phase 2 진입 판정

**진입 가능(Ready), 아직 시작하지 않음**.

사용자 결정:

1. Blackboard는 C++ 내부 판단 후 상태 중심으로 전달하는 현행 구조를 유지한다. 미존재 key를 자동 추가하지 않는다.
2. `Observer Aborts=None`은 현재 전환 중인 알려진 상태이며 후속 BT 수정 작업으로 이관한다.

LLM 서버 성공 응답과 `SetOrderForAll` 적용은 Phase 2 계획 작성 자체의 필수 선행 조건은 아니지만, 기존 order adapter를 수정하기 전 회귀 테스트 항목으로 고정한다.

현재 상태는 **Phase 1 Editor Verification Complete / Implementation Not Started / Phase 2 Ready**다.

## 12. MCP 에디터 정리 결과

- enum 확인을 위해 열었던 `E_CombatState`는 최종 `GetOpenAssets=[]`로 닫힌 상태를 확인했다.
- 임시 Slate observer를 `Unobserve`했으며 최종 observer 목록은 `[]`다.
- PIE는 에이전트가 종료했다.
- 선택된 actor/asset은 모두 `[]`이며 최상위 창은 기존 `Retry - 언리얼 에디터` 하나뿐이다.
- 확인한 level/BB/BT/enum/BP asset의 dirty 상태는 모두 false였다.
- 이후 MCP 작업은 저장소 루트 `AGENTS.md`의 Unreal Editor MCP 작업 절차를 따른다.
