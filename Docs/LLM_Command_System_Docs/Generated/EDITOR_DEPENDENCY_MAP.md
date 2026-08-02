# Phase 0/1 — Editor Dependency Map

이 문서는 코드/설정/파일 존재/과거 로그로 확인한 항목과 Unreal Editor에서만 확정 가능한 연결을 분리한다. `.uasset` 문자열 검출은 참조의 보조 증거일 뿐 저장된 property 값이나 graph semantics의 코드 검증 완료로 취급하지 않는다.

## 완료 상태

- **Code Analysis Complete**
- **Build Verification Complete**
- **Editor Verification Complete** — Unreal MCP, 2026-08-02
- **Implementation Not Started**

## 1. C++ → Blueprint/Asset 관계

| C++ 타입 | 에셋 후보 | 확인된 사실 | Editor에서 확정할 것 |
|---|---|---|---|
| `ARetryGameMode` | `/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode` | config의 global default; 과거 PIE 실제 class | parent, DefaultPawn, PlayerController, HUD, GameState |
| `ARetryPlayerController` | `/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController` | asset 존재; C++이 inventory/loot widget 생성 | parent, widget/input class assignments |
| `ARetryCharacter` | `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` | asset 존재 | parent, controller/mesh/input/widget defaults |
| `ARetryNPCCharacter` | `/Game/AI/Blueprints/BP_RetryNPCCharacter` | asset 문자열에 native class, `RetryNPCController`, UI widgets | parent, AIControllerClass, Auto Possess AI, TeamID, components, weapon, widget classes |
| `ARetryNPCController` | `/Game/AI/Blueprints/BP_RetryNPCController` | asset 문자열에 native class, `BehaviorTreeAsset`, `BT_LowIntelNPC` | parent와 actual property value |
| `AGroupManagerActor` | `/Game/AI/Blueprints/BP_GroupManager` | native parent 문자열; level external actor 2개 보조 증거 | parent, GroupID, threshold, placement |
| `UBehaviorTree` | `/Game/AI/BT_LowIntelNPC` | 과거 PIE에서 네 NPC가 실행; `BB_NPC`/custom nodes 문자열 | full graph, decorators, aborts, services, state branches |
| `UBlackboardData` | `/Game/AI/BB_NPC` | 주요 key 문자열 존재 | key type/default/enum mapping, C++ write key 누락 여부 |
| BP enum 가능성 | `/Game/AI/E_CombatState` | asset 존재 | native `ENPCCombatState` 11개와 순서/값 일치 여부 |
| `UDataTable` | `/Game/UI/DT_FallbackDialogue` | code hard-load path와 asset 존재 | row struct `FFallbackDialogueRow`, personality coverage |
| UI native classes | `/Game/UI/WBP_*` | assets 존재, 일부 NPC BP 문자열 참조 | parents, bindings, visibility/default classes |

## 2. Project Settings와 Maps & Modes

### 확인된 사실

- `GameDefaultMap` / `EditorStartupMap`: `/Game/ThirdPerson/Lvl_ThirdPerson`
- `GlobalDefaultGameMode`: `/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode`
- custom `GameInstanceClass` 설정 없음
- default RHI DX12/SM6
- Enhanced Input은 C++ PlayerController/Character에서 사용
- Runtime module은 `Retry`; HTTP/JSON/AI/Navigation/UMG/StateTree 의존성 포함

### 현재 확인 불가

- `Lvl_ThirdPerson` World Settings의 GameMode Override
- BP GameMode의 DefaultPawn/PlayerController/HUD/GameState
- Project Settings의 packaging map list/cook rules
- standalone/game build에서 PIE와 같은 class chain 사용 여부

## 3. AI 및 Blackboard 의존성

### 코드가 쓰거나 읽는 key

`TargetActor`, `CombatState`, `bAlerted`, `bCanSeeTarget`, `LastKnownEnemyLocation`, `CoverLocation`, `StimulusLocation`, `MoveDestination`, `PatrolIndex`, `DistanceToTarget`, `bIsLowHP`, `bIsOutOfAmmo`, `bIsReloading`, `Aggression`, `Fear`, `Trust`, `TrustBias`, `Loyalty`.

### asset 문자열에서 확인된 BB_NPC key

`Aggression`, `Fear`, `Trust`, `bAlerted`, `CombatState`, `CoverLocation`, `LastKnownEnemyLocation`, `PatrolIndex`, `StimulusLocation`, `TargetActor`.

### Editor에서 반드시 확인할 차이

- C++ 전용으로 보이는 `bCanSeeTarget`, `MoveDestination`, `DistanceToTarget`, low-HP/ammo/reload, `TrustBias`, `Loyalty` key 존재 여부
- `CombatState` key의 enum asset과 ordinal
- `TargetActor` object base class
- vector key가 zero vector를 “unset”으로 사용하는 branch 조건
- `UBTService_Decision` 배치 범위. root가 아닌 일부 branch에만 있으면 Decision update가 중단될 수 있음

## 4. Level placement 의존성

### 확인된 사실

- `Lvl_ThirdPerson` external actor 바이너리 문자열에서 다음이 검출됐다.
  - `BP_GroupManager` 두 external actor 파일
  - `BP_RetryNPCCharacter` 네 external actor 파일
  - `BP_PatrolPoint`
  - `NavMeshBoundsVolume`
- 과거 PIE 로그에서 실제 Group A/B, NPC 네 명, BT 실행이 확인됐다.

### 코드에 근거한 추론

- GroupManager는 C++ 자동 spawn이 없으므로 level 배치 actor일 가능성이 높다.
- NPC의 `MyGroup`은 `EditInstanceOnly`이고 BeginPlay 등록만 하므로 level instance reference가 핵심이다.
- 네 NPC도 main C++에 scenario spawner가 없으므로 level 배치일 가능성이 높다. cheat spawn은 group/patrol 설정을 하지 않는다.

### 현재 확인 불가

- actor label, transform, 정확한 GroupID/TeamID/leader flags
- patrol point array와 경로
- NavMeshBoundsVolume extent 및 green nav coverage
- PlayerStart, DummyTarget, 장애물, objective/route/observation marker 존재 여부
- Level Blueprint event/function 연결

## 5. Widget 생성 및 Viewport 경로

### 확인된 사실

- `ARetryPlayerController::BeginPlay`:
  - mobile control widget -> `AddToPlayerScreen`
  - `InventoryWidgetClass` -> create/add/hide
  - `LootWidgetClass` -> create/add/hide
- `PlayerHUDClass`는 선언되어 있으나 생성 코드가 없다.
- NPC name/debug/dialogue UI는 Character의 world-space `UWidgetComponent`에 class를 설정한다.
- scenario selection/restart/return UI와 OpenLevel 호출은 없다.

### Editor에서 확인

- `BP_ThirdPersonPlayerController`의 Inventory/Loot/Mobile/Input assignments
- `BP_RetryNPCCharacter`의 `WBP_FloatingName`, `WBP_AIDebug`, `WBP_Dialogue` assignments
- GameMode/Level Blueprint가 C++ 외에 HUD나 다른 widget을 생성하는지

## 6. DataAsset/DataTable

### 확인된 사실

- fallback dialogue table은 `ULLMRequestQueue::Initialize`가 hard path로 load한다.
- item definitions는 AssetManager primary assets와 `/Game/Items` scan을 사용한다.
- scenario DataAsset/DataTable은 현재 없다.

### Editor에서 확인

- `DT_FallbackDialogue` row struct와 rows
- Blueprint가 별도 DataTable/DataAsset을 참조하는지
- package/cook에 hard reference된 fallback table이 포함되는지

## 7. Persistent runtime state

| 객체 | lifetime | 현재 상태 | level restart 위험 |
|---|---|---|---|
| `ULLMRequestQueue` | GameInstanceSubsystem | queue, processing flag, RequestID, timer, weak targets | 이전 world request/callback 잔존 가능 |
| `AGroupManagerActor` | World actor | members, group memories, emotion | normal map reload면 폐기; seamless/persistent면 확인 |
| NPC components | Actor | personal memory, personality, decision | normal actor recreation이면 초기화 |
| static command/team state | 없음 | 해당 없음 | Phase 2 이후 설계 필요 |

## 8. Phase 2 전 에디터 게이트

1. Maps & Modes와 `Lvl_ThirdPerson` World Settings를 캡처/기록한다.
2. BP parent/reference/property chain을 확인한다.
3. `BB_NPC` 전체 key와 `E_CombatState` ordinal을 대조한다.
4. `BT_LowIntelNPC` full graph에서 CombatState branch, service, task를 기록한다.
5. Outliner에서 managers/NPCs/NavMesh/patrol actors를 확인한다.
6. 현재 워킹트리 PIE를 실행해 group registration, CombatState, LLM/order path를 확인한다.
7. 결과를 `EDITOR_ACTIONS.md` 체크리스트에 반영한다.

## 9. MCP 검증으로 확정된 의존성

- `Lvl_ThirdPerson` World Settings는 `BP_ThirdPersonGameMode_C`를 사용한다.
- GameMode는 `BP_ThirdPersonCharacter_C`, `BP_ThirdPersonPlayerController_C`, native HUD/GameState를 사용한다.
- NPC Character는 `BP_RetryNPCController_C`와 `PlacedInWorldOrSpawned`를 사용하며 세 NPC widget class가 할당돼 있다.
- NPC Controller의 `BehaviorTreeAsset`은 `BT_LowIntelNPC`다.
- Group A/B actor와 네 NPC의 instance reference가 실제로 연결돼 있고 group마다 leader 1명, member 총 2명이다.
- 관련 GameMode/NPC/NPCController/GroupManager BP graph는 비어 있다. C++ 외 추가 생성/연결 로직과 leader-death hook은 없다.
- `BB_NPC`는 11개 key만 가진다. C++ writer가 사용하는 `bCanSeeTarget`, `MoveDestination`, `DistanceToTarget`, `bIsLowHP`, `bIsOutOfAmmo`, `bIsReloading`, `TrustBias`, `Loyalty`는 없다.
- 사용자 확인: 판단을 Blackboard에서 C++ 내부로 이전한 결과이므로 위 key 부재는 의도된 최소 schema다. 새 기능에서 실제 소비 근거가 생기기 전에는 key를 일괄 추가하지 않는다.
- `E_CombatState` 11개 표시 순서와 BT에서 사용하는 ordinal은 native enum과 일치한다.
- BT는 `Hold` 외 10개 branch를 가지며 root 범위에 Decision service가 있다. 모든 state decorator abort mode는 `None`이다.
- 사용자 확인: abort mode는 현재 구현 전환 중인 알려진 상태로, 후속 BT 수정 작업에서 처리한다.
- NavMesh bounds는 초기 NPC를 X/Y에서 포함하지만 Recast maxTiles mismatch 재생성 경고가 있어 전체 동적 coverage는 후속 PIE 검증 대상이다.

상세값과 PIE 결과는 `EDITOR_ACTIONS.md` 11절을 권위 기록으로 사용한다.
