# Phase 1 — Codebase Flow Analysis

검증 기준: 2026-08-02 현재 워킹트리. 문서 설계가 아니라 C++/설정/기존 로그를 기준으로 작성했다.  
분류 표기: **[확인된 사실]**, **[코드에 근거한 추론]**, **[현재 확인 불가]**.

## 완료 상태

- **Code Analysis Complete**
- **Build Verification Complete** — Development Editor target
- **Editor Verification Complete** — Unreal MCP, 2026-08-02
- **Implementation Not Started**

## 1. Executive Summary

### 확인된 사실

- 현재 실제 상위 명령은 목표·위치·수명주기를 가진 Mission이 아니라 5값 `ENPCOrder`와 `float Weight`다.
- 활성 코드 경로는 `AGroupManagerActor` 중심이다. `UNPCGroupComponent`는 유사 책임을 중복 선언하지만 명령 전달과 LLM 연결이 구현되지 않아 현재 경로로 볼 수 없다.
- 현재 데이터 흐름은 다음과 같다.

```text
Perception / death / combat transition
  -> personal FNPCMemory or group FGroupMemoryEvent
  -> emotion score threshold
  -> ULLMRequestQueue (UGameInstanceSubsystem)
  -> HTTP chat-completions style response
  -> personality delta and/or ENPCOrder
  -> AGroupManagerActor::SetOrderForAll
  -> UNPCDecisionComponent::SetOrder
  -> next UBTService_Decision tick
  -> utility score / ENPCCombatState
  -> Blackboard CombatState and context keys
  -> BT asset branch and atomic BT Tasks
```

- `ENPCCombatState`는 11개지만 전투 중 utility 경쟁 대상은 Attack/Cover/Retreat/Reload 네 점수뿐이다. Cover 승자는 조건에 따라 `TakeCover` 또는 `Suppress`가 된다. 나머지는 우선 규칙이나 비전투 규칙으로 선택되며 `Alert`와 `Hold`은 현재 `DecideState`에서 반환되지 않는다.
- 명령은 누적·중첩·만료되지 않는다. `SetOrder`가 단일 pending pair를 덮어쓰며, 리더 사망 때 `HoldFire, 0`으로 실질 해제한다.
- TargetActor는 AI perception과 Blackboard에서, cover/last-known/stimulus/move destination은 여러 Decision/Service/Task에서 기록한다. 임무 목표 전용 key, Command ID, MissionContext, 성공/실패 monitor는 없다.
- 개인 및 그룹 메모리는 각각 별도 배열이며 상호 일반 전파나 Team Operational Memory는 없다.
- LLM queue는 GameInstanceSubsystem이라 레벨 전환 후 유지된다. queue/RequestID/timer를 명시적으로 reset하는 API가 없어 시나리오 재시작 설계 시 주의가 필요하다.

### 코드에 근거한 추론

- 최소 침습 지점은 `AGroupManagerActor`에 그룹별 `FMissionContext`를 소유시키고, 기존 `SetOrderForAll`을 유지한 채 명령 해석 결과를 각 `UNPCDecisionComponent`의 별도 mission overlay로 전달하는 것이다.
- `FCommandIntent`는 특정 Actor가 아닌 공용 데이터이므로 `Source/Retry/AI` 아래 독립 타입 헤더가 가장 자연스럽다. `LLMTypes.h`에 두면 HTTP/LLM에 불필요하게 결합된다.
- Team Operational Memory는 레벨 전환과 team 단위 조회가 필요하므로 Actor나 개인 component보다 `UWorldSubsystem`이 자연스럽다. 단, 레벨을 넘어 유지해야 한다면 별도 persistence snapshot만 GameInstance 계층으로 올려야 한다.

### 현재 확인 불가

- BT/Blackboard `.uasset`의 실제 graph 연결과 key type은 문자열 스캔 및 과거 로그로만 일부 확인했다.
- 현재 PIE에서 `SetOrder` 이후 어떤 BT branch가 선택되는지는 실행 검증이 필요하다.

## 2. Current LLM Order Flow

### 2.1 ENPCOrder 전체 조사

**[확인된 사실]** 선언: `Source/Retry/AI/NPCOrderTypes.h:7`

1. `HoldFire`
2. `FreeFire`
3. `Charge`
4. `Retreat`
5. `TakeCover`

전체 코드 검색 결과 사용 위치:

- `Source/Retry/NPCContext.h`: `FNPCContext::CurrentOrder`, 기본 `TakeCover`, weight 0
- `Source/Retry/Components/NPCDecisionComponent.h`: `PendingOrder`, 기본 `HoldFire`; `SetOrder`
- `Source/Retry/Components/NPCDecisionComponent.cpp`: attack/cover/retreat score 보정과 setter
- `Source/Retry/AI/GroupManagerActor.cpp`: 리더 사망 해제, 멤버 전파
- `Source/Retry/LLMRequestQueue.cpp`: 그룹 JSON 문자열을 enum으로 변환, 전체 전파
- `Source/Retry/Components/NPCGroupComponent.cpp`: 해제 enum만 사용하나 멤버 전달 본문은 미구현

### 2.2 SetOrder 선언, 구현, 모든 호출자

**[확인된 사실]**

- 선언: `UNPCDecisionComponent::SetOrder(ENPCOrder, float)`, `Source/Retry/Components/NPCDecisionComponent.h:64`
- 구현: 같은 class cpp line 730. 단순히 `PendingOrder`와 `PendingOrderWeight`를 대입한다.
- 직접 호출자는 `AGroupManagerActor::SetOrderForAll` 한 곳(`GroupManagerActor.cpp:119`)이다.
- 해당 group broadcast의 호출자는 두 곳이다.
  - `ULLMRequestQueue::ParseAndApplyGroupResponse` (`LLMRequestQueue.cpp:382`), 고정 weight `0.5`
  - `AGroupManagerActor::OnLeaderDied` (`GroupManagerActor.cpp:102`), `HoldFire, 0`
- 별도 `UNPCGroupComponent::SetOrderForAll`은 이름만 같고 DecisionComponent를 호출하지 않는다.

### 2.3 호출 흐름 표 — 시작부터 최종 상태 반영까지

| 단계 | 클래스/파일 | 함수 | 입력 | 출력 | 다음 단계 |
|---|---|---|---|---|---|
| 1 | `ARetryNPCCharacter` / `RetryNPCCharacter.cpp` | `OnDeath` | 사망 NPC, 같은 팀 NPC, LOS/거리 | `AllyDeath` group/personal memory | group emotion 누적 |
| 1a | `UNPCDecisionComponent` / `NPCDecisionComponent.cpp` | `CollectContext` | 새 visible target | `CombatStart` group memory | group emotion 누적 |
| 2 | `AGroupManagerActor` / `GroupManagerActor.cpp` | `AddGroupMemory` | witness/type/location/weight/text | event append, accumulated score | threshold 검사 |
| 3 | `AGroupManagerActor` | `AddGroupMemory` | score >= threshold | `EnqueueGroupRequest(this)` | LLM queue |
| 4 | `ULLMRequestQueue` / `LLMRequestQueue.cpp` | `EnqueueGroupRequest` -> `Enqueue` -> `ProcessNext` | weak group, prompt | serial queue item + RequestID | HTTP request |
| 5 | `ULLMRequestQueue` | `SendRequest` | chat prompt | async HTTP callback or timer fallback | group parse 또는 fallback |
| 6 | `ULLMRequestQueue` | `ParseAndApplyGroupResponse` | outer JSON, inner JSON | personality deltas, parsed `ENPCOrder` | group broadcast |
| 7 | `AGroupManagerActor` | `SetOrderForAll` | enum, fixed 0.5 | each valid member controller's DecisionComponent setter | pending pair overwrite |
| 8 | `UNPCDecisionComponent` | `SetOrder` | enum, weight | `PendingOrder`, `PendingOrderWeight` | next Decision service tick |
| 9 | `UBTService_Decision` / `BTService_Decision.cpp` | `TickNode` | DeltaSeconds | `DC->Update` every about 0.1s | context/decision |
| 10 | `UNPCDecisionComponent` | `CollectContext` | pending pair + world/components/BB | `FNPCContext` | scoring |
| 11 | `UNPCDecisionComponent` | `DecideState` | context | candidate state + score snapshot | stability gate |
| 12 | `UNPCDecisionComponent` | `GetStableState`/`UpdateCombatState` | candidate/current/time | stable current state, weapon aim start/stop, transition log | Blackboard |
| 13 | `UNPCDecisionComponent` | `WriteBlackboard` | current state/context | `CombatState` enum and context keys | BT graph branch |
| 14 | `BT_LowIntelNPC.uasset` + BT Tasks | asset graph | Blackboard | movement/fire/search/etc. | world state |

### 2.4 갱신·해제·중첩 규칙

**[확인된 사실]**

- 갱신: 마지막 `SetOrder` 호출이 이전 값을 즉시 덮어쓴다.
- 해제: 별도 clear flag가 없고 `HoldFire, 0`이 사실상 무효 명령이다.
- 중첩/priority/issuer/target/expiration/acknowledgement는 없다.
- weight clamp가 없다. score 최종값만 `[0,1]` clamp된다.
- order 값은 state를 직접 강제하지 않고 전투 점수만 보정한다.
- 전투 타겟이 없으면 order는 `DecideState` 비전투 분기에 영향을 주지 않는다.

### 2.5 점수 영향

**[확인된 사실]**

- `Charge`, `FreeFire`: Attack `+OrderWeight`
- `HoldFire`: Attack `-OrderWeight`, Cover `+OrderWeight` (cover location이 있을 때만)
- `TakeCover`: Cover `+OrderWeight` (cover location이 있을 때만)
- `Retreat`: Retreat `+OrderWeight`
- Reload에는 order 영향이 없다.
- `HoldFire`는 발사를 hard-block하지 않는다. attack score가 여전히 이길 수 있다.

## 3. NPC Decision and CombatState Flow

### 3.1 책임과 입력

**[확인된 사실]** `UNPCDecisionComponent`는 AIController의 default subobject이며 자체 tick은 꺼져 있다. BT의 `UBTService_Decision`이 `Update`를 호출해야 동작한다.

입력:

- AI perception cache: `ARetryNPCController::LastPerceivedActor`
- Blackboard: `TargetActor`, `bAlerted`, `LastKnownEnemyLocation`, `CoverLocation`
- Health: HP ratio/dead
- Weapon: ammo/reserve/reload/armed
- Personality: Aggression, Fear, Trust, Courage, FearSensitivity, CoverPreference, TacticalSkill, TrustBias, Loyalty, Dominance, Patience, Stress
- Character: patrol point 존재
- Navigation/visibility: cover 후보, line trace, distance
- Group command: pending order + weight

직접 사용되지 않는 입력:

- 개인 `FNPCMemory` 내용은 점수 계산에 들어가지 않는다. 메모리가 LLM을 거쳐 Personality를 바꾼 뒤 간접 영향만 줄 수 있다.
- `AGroupManagerActor`의 인원수/리더/group memory 자체는 개인 score 입력이 아니다.
- `Trust`, `TacticalSkill`, `TrustBias`, `Patience`, `Stress`는 context에 수집되지만 현재 네 score 식에는 쓰이지 않는다.

### 3.2 ENPCCombatState 11종과 선택

**[확인된 사실]** 선언: `Source/Retry/NPCContext.h:8`

| 상태 | 선택 경로 | 비고 |
|---|---|---|
| `Idle` | target 없음, alert/patrol/reload 아님 | 활성 |
| `Patrol` | target 없음, patrol points 있음 | 활성 |
| `Alert` | 반환 경로 없음 | 선언됐으나 C++ decision에서 미사용 |
| `Search` | target 없음, `bAlerted` | 활성 |
| `Attack` | 전투 score winner 기본 | 활성 |
| `TakeCover` | cover score winner, 아직 cover/LOS 조건 미충족 | 활성 |
| `Reload` | 비전투 선장전 또는 전투 reload score winner | 즉시 전환 |
| `Retreat` | retreat score winner | 활성; 진입 시 5초 후 NPC 비활성화 예약 |
| `Hold` | 반환 경로 없음 | 선언됐으나 C++ decision에서 미사용 |
| `Suppress` | cover score winner + in cover + target visible | 활성 |
| `Dead` | HP ratio <= 0 | 즉시 전환. 실제 사망 시 Health delegate가 BT를 먼저 중단할 수 있음 |

### 3.3 점수식

**[확인된 사실]** 모든 score는 계산 후 개별 weight를 곱하고 `[0,1]`로 clamp한다. 동점 우선순위는 코드 비교 순서상 `Reload > Retreat > Cover > Attack`이다.

```text
Attack = 0.4
       + Aggression*0.3 + Courage*0.2 + Dominance*0.15
       - (1-HP)*(1-Courage)*0.3 - CoverPreference*0.1
       + order modifier
prerequisite: armed, ammo>0, visible target

Cover = CoverPreference*0.4 + FearSensitivity*0.2 + Fear*0.2
      + (1-HP)*0.2 + order modifier
prerequisite: non-zero CoverLocation

Retreat = (1-HP)*0.4 + Fear*0.2 + FearSensitivity*0.2
        - Courage*0.3 - Aggression*0.2 - Loyalty*0.15
        + Retreat order modifier
        + 0.6 if armed and all ammo exhausted

Reload = 1.0 if magazine empty and reserve exists
       = (1-ammo ratio)*0.5 otherwise
prerequisite: armed and reserve ammo>0
```

### 3.4 타겟/위치 결정 시점

**[확인된 사실]**

- target: `ARetryNPCController::OnTargetPerceptionUpdated`가 `LastPerceivedActor`를 설정한다. `UNPCDecisionComponent::UpdateAIState`가 Blackboard `TargetActor`에 쓴다.
- target loss: perception lost callback은 actor를 실제로 clear하지 않는다. 주석과 달리 `LastPerceivedActor`가 null이 되지 않으므로 `UpdateAIState`의 lost timeout else branch가 실행되지 않는 경로가 존재한다. 사망 타겟은 `CollectContext`가 명시적으로 clear한다.
- last known location: visible일 때 `WriteBlackboard`와 `UBTService_UpdateCombatState`가 기록한다.
- cover: DecisionComponent와 `UBTService_FindCover`가 각각 독립적으로 랜덤 NavMesh 후보를 계산해 같은 `CoverLocation`을 쓸 수 있다.
- move destination: `UBTTask_SelectCombatRoute`가 성격과 current target으로 기록한다.
- stimulus: `UBTTask_CallOut`이 반경 내 **팀 필터 없이** 모든 `ARetryNPCCharacter`의 Blackboard에 기록한다.

### 3.5 강제/긴급/안정화

**[확인된 사실]**

- `Dead`, `Reload` candidate는 min-duration을 무시하고 즉시 전환한다.
- Attack/Suppress는 target 없음, ammo 없음, LOS 없음이면 현재 state 강제 종료.
- TakeCover는 target 없음, Retreat는 target 없음 + HP>0.5이면 강제 종료.
- 그 외 전환은 기본 1초 `StateMinDuration`을 거친다.
- 외부 API로 CombatState를 강제하는 override는 없다. Order도 soft score modifier다.
- Retreat 진입은 memory 기록, Fear=1, 5초 뒤 actor hidden/collision/tick disabled라는 비가역성 높은 특수 경로다.
- mission 복귀 stack이나 interrupted state는 없다. 긴급 행동 후 매 tick 재평가할 뿐이다.

### 3.6 이벤트와 디버그

**[확인된 사실]**

- state 변경 delegate는 없다.
- 최근 10개의 `FNPCStateTransition`을 `TransitionHistory`에 저장하고 score snapshot/reason/time을 남긴다.
- `ToggleAIDebug` cheat가 모든 NPC DecisionComponent의 debug flag와 world-space `WBP_AIDebug` visibility를 전환한다.
- 로그는 `[AI] NPC: From -> To | A/C/R/Rl` 형식이다.

## 4. Behavior Tree Execution Flow

### 4.1 연결

**[확인된 사실]**

1. `ARetryNPCController::OnPossess`가 `BehaviorTreeAsset`이 있으면 `RunBehaviorTree`를 호출한다.
2. 과거 PIE 로그는 네 NPC 모두 `BT_LowIntelNPC`를 실행했음을 증명한다.
3. 해당 asset binary 문자열에는 `BB_NPC`, `UBTService_Decision`, 여러 custom task 참조가 있다.
4. `UBTService_Decision`이 0.1초 interval로 DecisionComponent를 갱신한다.
5. DecisionComponent가 native `ENPCCombatState` ordinal을 Blackboard `CombatState`에 기록한다.
6. 이후 어느 branch/task가 실행되는지는 BT asset graph 책임이다.

**[현재 확인 불가]** `CombatState` key가 native enum인지 `Content/AI/E_CombatState.uasset`인지, 값 순서가 동일한지, decorator abort policy가 무엇인지는 Editor 확인이 필요하다.

### 4.2 C++ BT Task 18종

| Task | 책임 | 즉시 결과/비동기 주의 | 현재 asset 사용 여부 |
|---|---|---|---|
| `UBTTask_AimAtTarget` | selector target으로 회전 | `InProgress`를 반환하지만 tick/finish 구현 없음 | 미확인 |
| `UBTTask_CallOut` | last-known 위치를 주변 NPC stimulus로 전파 | 즉시 success; team/group filter 없음 | binary 참조 확인 |
| `UBTTask_ChargeEnemy` | speed 600, target으로 이동 요청 | 이동 완료를 기다리지 않고 success | 미확인 |
| `UBTTask_CloseDistance` | target 앞 지정 거리까지 이동 | 이동 완료를 기다리지 않고 success | 미확인 |
| `UBTTask_FireAtTarget` | target 검증, 조준점/Fire | 즉시 success/fail | binary 참조 확인 |
| `UBTTask_FireFromCover` | cover에서 여러 Fire 호출 | 즉시 success; 실제 burst timing 아님 | 미확인 |
| `UBTTask_LookAround` | random focal point | 즉시 success | binary 참조 확인 |
| `UBTTask_MoveToCover` | `CoverLocation`으로 MoveTo | moving이면 `InProgress`, 완료 callback 없음 | 미확인 |
| `UBTTask_MoveToLastKnown` | last-known으로 이동 요청 | 요청 후 즉시 success | binary 참조 확인 |
| `UBTTask_MoveToPatrolPoint` | patrol point 이동/인덱스 증가 | bounds 검증 없이 index 사용, 즉시 success | binary 참조 확인 |
| `UBTTask_MoveToTarget` | selector actor/vector로 이동 | destination zero 검증 없음, 즉시 success | 미확인 |
| `UBTTask_Overwatch` | stimulus 방향 focus | alert clear timer 주석 처리, 즉시 success | binary 참조 확인 |
| `UBTTask_Reload` | weapon reload 호출 | BB flag를 같은 호출에서 true->false, 즉시 success | binary 참조 확인 |
| `UBTTask_Retreat` | target 반대 NavMesh 지점 이동 | 이동 요청 후 즉시 success | binary 참조 확인 |
| `UBTTask_SearchArea` | last-known 주변 random points를 timer 이동 | task는 즉시 success; timer가 BT lifecycle 밖에서 계속됨 | binary 참조 확인 |
| `UBTTask_SelectCombatRoute` | personality로 `MoveDestination` 선정 | 즉시 success | 미확인 |
| `UBTTask_SuppressiveFire` | target에 burst 반복 Fire | spread 변수를 계산하지만 aim에 적용하지 않음 | binary 참조 확인 |

추가 node:

- Services: `Decision`, `FindCover`, `UpdateCombatState`, `UpdatePerception`
- Decorator: `HasPatrolPoints`
- **[확인된 사실]** `BT_LowIntelNPC.uasset` 문자열 스캔은 `BTService_Decision`과 위 표의 “binary 참조 확인” task를 보여준다. 이것은 graph 배치의 보조 증거이며 branch semantics나 실행을 확정하지 않는다.
- **[코드에 근거한 추론]** 참조가 확인되지 않은 C++ task들은 현재 main BT에서 미사용일 가능성이 높다. 다른 asset 또는 Blueprint 동적 참조 여부는 미확인이다.

### 4.3 Blackboard write map

| Key | Writer | Reader |
|---|---|---|
| `TargetActor` | Decision `UpdateAIState` | Decision, perception/combat services, fire/move/retreat/route tasks |
| `CombatState` | Decision `WriteBlackboard` | BT decorators/branches (asset) |
| `bAlerted` | Decision, perception service, CallOut | Decision |
| `bCanSeeTarget` | perception service | combat service; Decision은 자체 LOS 사용 |
| `LastKnownEnemyLocation` | Decision, combat service | MoveToLastKnown, SearchArea, CallOut |
| `CoverLocation` | Decision, FindCover service | Decision, MoveToCover |
| `StimulusLocation` | CallOut | TurnToStimulus, Overwatch |
| `MoveDestination` | SelectCombatRoute | MoveToTarget selector path (asset 설정 필요) |
| `PatrolIndex` | MoveToPatrolPoint | same task |
| `DistanceToTarget` | combat service | asset/unknown |
| `bIsLowHP`, `bIsOutOfAmmo`, `bIsReloading` | combat service/task | asset/unknown |
| personality BB keys | Personality/Decision | route task reads component directly; asset/unknown |

`BB_NPC.uasset` 문자열에서 확인되는 key는 `Aggression`, `Fear`, `Trust`, `bAlerted`, `CombatState`, `CoverLocation`, `LastKnownEnemyLocation`, `PatrolIndex`, `StimulusLocation`, `TargetActor`다. C++이 쓰는 다른 key가 실제 Blackboard에 모두 있는지는 Editor에서 확인해야 한다.

### 4.4 실패·소실·중단·복귀

**[확인된 사실]**

- path request가 즉시 `Failed`인 경우를 명시적으로 검사하는 task는 `MoveToCover`뿐이다.
- 대부분 이동 task는 request 결과/완료를 기다리지 않고 success를 반환한다.
- target null/dead/ammo 부족은 fire/retreat 등 각 task가 fail한다.
- target 사망은 Decision이 Blackboard와 controller cache를 clear한다.
- perception lost actor가 cache에서 실제 clear되지 않는 결함 후보가 있다.
- BT 전체는 NPC death에 `BrainComponent->StopLogic("Dead")`로 중단된다.
- mission completion 또는 기존 행동 복귀 API는 없다. target이 없어진 뒤 Decision이 reload/search/patrol/idle 중 하나로 재선택한다.
- 정찰 재사용 후보는 `MoveToTarget`(generic location), `LookAround`, `SearchArea`; 보고/전파 후보는 `CallOut`이다. 그러나 `CallOut`은 structured report나 team memory가 아니라 인접 NPC Blackboard broadcast다.

## 5. Group and Memory Flow

### 5.1 주요 클래스별 책임

| 클래스 | 확인된 현재 책임 | 현재 한계 |
|---|---|---|
| `AGroupManagerActor` | GroupID, leader/member registry, group event list, emotion threshold, group prompt, group-wide order | 생성/해제 API 없음, team/faction 없음, current mission/target/state 저장 없음 |
| `UNPCGroupComponent` | 같은 데이터 구조의 component 초안 | LLM/Decision 전달 미구현, 실제 owner 생성/사용 검색 결과 없음 |
| `ARetryNPCCharacter` | 개인 components, group pointer/leader flag, death witness scan, memory threshold callback | `MyGroup` 수동 참조, group deregistration 없음 |
| `UMemoryComponent` | 개인 기억 최대 개수, emotion accumulation/delegate | expiration/merge/source/confidence 없음 |
| `ULLMRequestQueue` | 개인/그룹 요청 직렬화, HTTP, parse/apply/fallback | queue reset/cancel/schema validation/command fallback 없음 |

### 5.2 그룹 생성·등록·리더·명령

**[확인된 사실]**

- C++에는 GroupManager 자동 spawn/registry factory가 없다.
- `Content/AI/Blueprints/BP_GroupManager.uasset`가 존재하며 native `GroupManagerActor` 문자열 참조가 있다.
- `ARetryNPCCharacter::MyGroup`과 `bIsGroupLeader`는 `EditInstanceOnly`; BeginPlay 때만 `RegisterMember`한다.
- `RegisterMember`는 leader pointer를 덮어쓸 수 있고 member unique add를 한다. deregister/destroy/disband는 없다.
- `OnLeaderDied`가 있지만 `ARetryNPCCharacter::OnDeath`에서 호출하지 않는다. 전체 검색상 호출자 없음. 따라서 현재 자동 리더 사망 처리 경로는 연결되지 않았다.
- GroupManager가 각 member controller를 찾아 DecisionComponent로 order를 push하므로 의존 방향은 group -> character/controller -> decision이다. Character도 `MyGroup`을 통해 group으로 event를 push하므로 양방향 참조다.
- Group마다 독립 Actor와 member array/order push가 가능하므로 서로 다른 order를 동시에 보낼 데이터 격리는 가능하다. 하지만 명령 자체를 manager에 보관하지 않아 현재 명령 조회/수명주기/충돌 해결은 불가능하다.
- team ID는 NPC에 `uint8 TeamID`가 있으며 controller attitude에 쓰인다. GroupManager에는 Team/Faction ID가 없다.
- 과거 PIE 로그는 Group A/B 각각 2명 등록을 확인한다.

### 5.3 개인 기억

**[확인된 사실]**

`EMemoryEventType` 전체:

1. `AllyDeath`
2. `EnemyKilled`
3. `CombatDefeat`
4. `PlayerCommand`
5. `ValuableItemFound`

`FNPCMemory`: EventType, Location, Timestamp(world seconds), EmotionWeight, Description.

- 기본 최대 20개. append 후 초과하면 oldest index 0 하나를 제거한다.
- emotion weight는 부호 포함 그대로 누적된다. `>= threshold` 때 delegate broadcast 후 0 reset.
- duplicate merge, time expiration, manual delete, source ID, confidence, observed/received 구분은 없다.
- 실제 생성 위치:
  - Decision state가 Retreat로 바뀔 때 `CombatDefeat` 0.5
  - target just died와 이전 firing/reload 조건일 때 `EnemyKilled` 0.2
  - group 없는 allied witness가 death를 볼 때 `AllyDeath` 0.2
  - cheat `TestCommandMemory`: 모든 NPC에 `PlayerCommand` 0.1
  - cheat `TriggerMemory`: nearest high-intelligence NPC에 test `AllyDeath`
- `ValuableItemFound` 생성자는 검색되지 않았다.
- high-intelligence NPC만 threshold delegate를 `OnMemoryThresholdReached`에 연결한다.

### 5.4 그룹 기억과 목격

**[확인된 사실]** `FGroupMemoryEvent`는 WitnessID, EventType(string), Location, Timestamp, EmotionWeight, Description만 가진다.

생성 경로:

1. 새 visible target 감지: Decision `CollectContext` -> own `MyGroup->AddGroupMemory("CombatStart", 0.1)`.
2. NPC death: 모든 NPC actor scan -> same TeamID -> visibility trace + 3000 range.
3. 같은 group witness: dead NPC의 `MyGroup`에 `AllyDeath`, 0.3.
4. 다른 allied group witness: witness NPC의 `MyGroup`에 `AllyDeath`, 0.2.
5. group 없는 allied witness: 개인 memory에만 `AllyDeath`, 0.2.

위험/결함 후보:

- 다른 allied group branch는 dead NPC의 `MyGroup`만 null-check하고 `OtherNPC->MyGroup`은 dereference 전에 검사하지 않는다. “dead NPC has group, witness is allied but group-less”이면 null dereference 가능성이 있다.
- group memories는 최대 개수/expiration/merge가 없어 계속 증가한다.
- `bWasTargetSetLastTick`은 DecisionComponent 단일 target set 여부만 보며, target 교체 자체를 별도 event로 구분하지 않는다.
- group event는 witness가 확인한 정보지만 다른 member 개인 memory로 복제되지는 않는다. prompt에서 group 전체 기억으로 소비될 뿐이다.

### 5.5 메모리에서 LLM까지

```text
Personal:
UMemoryComponent::AddMemory
  -> AccumulatedEmotionScore >= threshold
  -> OnMemoryThreshold.Broadcast
  -> ARetryNPCCharacter::OnMemoryThresholdReached (high-intelligence only)
  -> recent 5 memories + Personality prompt
  -> individual FLLMRequest
  -> personality delta + dialogue

Group:
AGroupManagerActor::AddGroupMemory
  -> AccumulatedEmotionScore >= threshold
  -> recent 8 group memories + member traits prompt
  -> group FLLMRequest
  -> member personality delta + group order
```

Team Operational Memory 및 report/communication 상태 머신은 존재하지 않는다.

## 6. LLM Integration and Fallback

### 6.1 요청 생성과 prompt

**[확인된 사실]** `ULLMRequestQueue`는 유일한 project `UGameInstanceSubsystem`이다.

- 개인 요청: memory threshold callback에서 recent 5 memory description, personality current values/type/tone, delta JSON format, Korean dialogue 요구를 prompt로 만든다.
- 그룹 요청: group threshold에서 valid members의 name/5 traits, recent 8 group event descriptions, member delta array와 5-value order를 요구한다.
- HTTP endpoint 기본값: `http://localhost:8080/v1/chat/completions`.
- body: messages `[role=user, content=prompt]`, temperature 0.3, max_tokens 300. model/auth/system message는 없다.
- one-at-a-time `TQueue`; enqueue 때 process 중이 아니면 즉시 시작한다.
- RequestID는 GameInstanceSubsystem lifetime 동안 증가한다.

### 6.2 비동기, timeout, parse

**[확인된 사실]**

- timeout 기본 40초, subsystem 전체에 `FTimerHandle` 하나.
- HTTP callback은 network success/response validity만 확인한다. HTTP status code 범위 검사는 없다.
- outer response는 `choices[0].message.content`, inner는 markdown fence 제거 후 JSON으로 파싱한다.
- 개인 parse: 5 delta field는 optional read, range/schema 검증 없이 `ApplyDelta`; ApplyDelta가 최종 traits를 `[0,1]` clamp. dialogue optional.
- 그룹 parse: `Members` ID를 UObject `GetName()`과 exact match해 delta 적용; `Order` string unknown/missing이면 unknown string은 기본 HoldFire, missing은 order 미적용.
- structured output API/JSON schema는 사용하지 않고 prompt-only JSON 계약이다.

### 6.3 실패와 fallback

**[확인된 사실]**

- request 대상 weak pointer가 invalid이면 skip하고 next.
- timeout 또는 HTTP callback failure면 `ApplyFallback` 후 next.
- 개인 fallback: personality 변화 없이 hard-loaded `/Game/UI/DT_FallbackDialogue`에서 personality type matching row를 random 재생.
- group fallback: TargetActor/TargetPersonality가 없으므로 사실상 로그만 남고 order/doctrine fallback이 없다.
- JSON parse failure, missing choices, missing required nested fields, dead target skip은 fallback을 호출하지 않는다.
- fallback DataTable load failure는 error log 후 dialogue 없이 종료한다.

### 6.4 동시성/수명 위험

**[코드에 근거한 추론]**

- timeout 시 HTTP request를 cancel하거나 request를 completed로 표시하지 않는다. 늦은 callback이 오면 동일 요청을 다시 parse하고 `ProcessNext`를 또 호출할 수 있다.
- timeout 후 next request가 시작되면 늦은 이전 callback이 subsystem 공용 timer handle을 clear해 다음 요청 timeout을 지울 수 있다.
- level transition 후 subsystem queue와 callbacks가 남지만 target weak pointers만 무효화된다. queue/RequestID/timer를 reset하는 public API가 없다.
- parse의 `GetObjectField`/`GetStringField`는 예상 형식이 아닐 때 graceful validation이 충분하지 않다.

## 7. Level Loading and Scenario Initialization

### 7.1 현재 구조

**[확인된 사실]**

- 기본 map/editor startup map: `/Game/ThirdPerson/Lvl_ThirdPerson`.
- global GameMode: `/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode`.
- native `ARetryGameMode`는 abstract stub이며 레벨 전환/initialization이 없다.
- custom GameInstance class 설정은 없다.
- project subsystem은 `ULLMRequestQueue` 하나다.
- `OpenLevel`, `ServerTravel`, `ClientTravel`, `RestartLevel` 호출은 project main C++에 없다.
- `ARetryPlayerController::BeginPlay`는 Inventory/Loot widget을 create/add하고 Enhanced Input을 연결한다. `PlayerHUDClass`는 선언됐지만 C++에서 생성되지 않는다.
- `ARetryNPCCharacter` 생성자에서 개인 components를 자동 생성하고 BeginPlay에서 지정된 group에 등록한다.
- GroupManager/NPC 자동 생성자는 없다. cheat `SpawnNPC`만 디버그 spawn을 제공하며 group 지정은 하지 않는다.
- map files: ThirdPerson, Combat, Platforming, SideScrolling 및 showcase maps. 런타임 등록 scenario list는 없다.

### 7.2 에셋/레벨 보조 증거

**[확인된 사실: 바이너리 문자열/파일 존재 수준]**

- `Content/AI/Blueprints/BP_GroupManager.uasset`는 native GroupManagerActor 참조를 가진다.
- `Lvl_ThirdPerson` external actor 파일에서 BP_GroupManager 두 개, BP_RetryNPCCharacter 네 개, BP_PatrolPoint, NavMeshBoundsVolume 문자열이 검출된다.
- 과거 PIE 로그는 Group A/B와 네 NPC를 확인한다.

**[현재 확인 불가]** 정확한 actor placement, property values, World Settings override, NavMesh bounds, external actor ownership은 Editor Outliner/Details에서 확인해야 한다.

### 7.3 level name 열거

**[확인된 사실]** filesystem에서는 `.umap`을 열거할 수 있지만 packaged runtime API가 아니다.

**[코드에 근거한 추론]** packaged/PIE 공통 동작을 위해서는 자동 폴더 scan보다 soft world reference의 명시 등록이 안전하다. 현재 project에는 그 registry가 없다.

### 7.4 레벨 변경 시 초기화 대상

- 자동 world lifetime: placed GroupManager actors, NPCs, their component memories/state, Blackboard/BT.
- 지속 가능: `ULLMRequestQueue` pending queue, `bIsProcessing`, `NextRequestID`, timeout/callback state, loaded fallback DataTable.
- static/global: 별도 command/team memory/static ID system은 현재 없다.
- 구현 시 reset 필요 후보: LLM pending work 및 request namespace, future Team Memory, command contexts/log buffer/seed. 기존 group/personal memories는 actor destruction 시 사라지지만 seamless travel/persistent actor를 도입하면 재검토해야 한다.

## 8. Candidate Insertion Points

### 변경 후보 표

| 후보 지점 | 현재 책임 | 추가할 책임 | 장점 | 위험 |
|---|---|---|---|---|
| 독립 `AI/CommandTypes.h` | 없음 | `FCommandIntent`, enums/criteria/constraints | LLM/BT와 분리된 공용 계약 | 새 타입의 범위가 너무 커질 수 있음 |
| `AGroupManagerActor` | group registry/event/order broadcast | current `FMissionContext`, assign/cancel/status 조회 | 이미 group별 전달 허브, 최소 침습 | Actor lifetime/Team 계층 부족 |
| `UNPCDecisionComponent` | 개인 utility/state/BB write | optional mission overlay, weight modifiers, objective key write | 기존 score/autonomy 재사용 | hard constraint를 단순 score로만 구현하면 유실 |
| `UBTService_Decision` 경로 | 주기적 decision 진입 | mission completion heartbeat/snapshot 전달 | 기존 cadence 재사용 | BT asset이 service를 빠뜨리면 동작 안 함 |
| Blackboard | tactical context bridge | `MissionObjectiveActor/Location`, `CommandId`, constraints flags 최소 추가 | 기존 BT 연결점 | `.uasset` 수동 변경 필수, key mismatch 위험 |
| 별도 `UWorldSubsystem` | 없음 | team-scoped operational facts/reports | level lifetime/team lookup/actor 비의존 | 신규 subsystem 책임이 과대해질 수 있음 |
| `ULLMRequestQueue` | transport+parse+apply 혼합 | 향후 structured command transport only | 기존 HTTP 재사용 | Phase 7 전 연결 금지; 현재 timeout 결함 선행 처리 필요 |

### 8개 완료 질문에 대한 답

1. **FCommandIntent 위치**: **[추론]** `Source/Retry/AI/CommandTypes.h` 같은 독립 Runtime data header. `LLMTypes.h`는 transport 타입이므로 피한다.
2. **MissionContext 소유자**: **[추론]** authoritative group copy는 `AGroupManagerActor`, 개인 실행 overlay는 `UNPCDecisionComponent`가 command ID와 read-only derived context를 보유.
3. **그룹 명령 전달**: **[사실]** `AGroupManagerActor::SetOrderForAll -> member controller -> UNPCDecisionComponent::SetOrder`.
4. **목표 주입**: **[추론]** DecisionComponent `WriteBlackboard`에서 기존 target/position 패턴을 따라 전용 mission actor/vector key에 기록. 전투 `TargetActor`를 임무 목적지로 오염시키지 않는다.
5. **score 재사용**: **[사실/추론]** 기존 per-state score와 configurable weights를 재사용 가능. Mission modifier map을 네 score에 합성하되 hard constraint는 별도 gate가 필요.
6. **성공/실패 감시**: **[추론]** group-owned mission monitor가 member health, objective distance/occupancy, path request result, report receipt를 관찰. 현재 task success는 이동 완료를 뜻하지 않아 그대로 사용할 수 없다.
7. **Team Operational Memory**: **[추론]** team ID로 partition된 `UWorldSubsystem`이 가장 자연스럽다. Actor 배치나 개인 component 중복을 피하고 level restart 때 자동 폐기된다.
8. **level 변경 reset**: **[사실/추론]** world actors/components/BT는 기본 폐기; GameInstance LLM queue와 future cross-level selection state는 명시 reset/cancel 필요.

## 9. Files Requiring Modification

아래는 **Phase 2 이후 후보일 뿐 현재 수정하지 않았다.**

### 파일 영향 표

| 파일 | 변경 가능성 | 변경 이유 | 회귀 위험 | 테스트 방법 |
|---|---:|---|---|---|
| `Source/Retry/AI/NPCOrderTypes.h` | 낮음 | legacy order 유지/adapter 문서화 | enum ordinal 변경 시 BP/BB 파손 | enum/BT branch regression |
| 신규 `Source/Retry/AI/CommandTypes.h` | 높음 | command/context 공용 타입 | serialization/Blueprint exposure | UHT + validation unit tests |
| `Source/Retry/AI/GroupManagerActor.h/.cpp` | 높음 | group command/context owner/dispatch | existing LLM group order 회귀 | Group A/B independent command test |
| `Source/Retry/Components/NPCDecisionComponent.h/.cpp` | 높음 | mission overlay, objective BB, modifiers | combat scoring/state regression | fixed context score tests + PIE |
| `Source/Retry/NPCContext.h` | 중간 | derived mission inputs | existing default/serialization | score snapshot tests |
| `Source/Retry/LLMTypes.h` | 낮음 | command request reference only in Phase 7 | transport-domain coupling | JSON contract tests |
| `Source/Retry/LLMRequestQueue.h/.cpp` | 중간/후기 | cancel/reset, structured response, doctrine fallback | async race/double apply | timeout/late callback/level travel tests |
| 신규 Team Memory subsystem files | 높음 | report-gated team facts | new lifecycle/identity risk | world reset/team partition tests |
| `Source/Retry/RetryNPCCharacter.h/.cpp` | 중간 | report/leader death hooks 가능 | death witness null path | death/group-less witness tests |
| `Content/AI/BB_NPC.uasset` | 에디터 후보 | mission keys | key type/name mismatch | Editor + PIE |
| `Content/AI/BT_LowIntelNPC.uasset` | 에디터 후보 | mission branch/monitor hookup | existing combat branch regression | BT debugger + PIE |

명시적으로 권장하지 않는 변경:

- `UNPCGroupComponent`를 별도 두 번째 group runtime으로 완성하는 것. 먼저 사용되지 않는 중복 초안으로 격리하거나 Phase 2에서 한 체계만 선택해야 한다.
- 기존 17 BT Task를 새 mission별 task 세트로 복제하는 것.
- LLM이 BT task/좌표를 직접 선택하게 하는 것.

## 10. Regression Risks

1. native `ENPCCombatState`와 BP enum/Blackboard ordinal 불일치.
2. mission target을 기존 `TargetActor`에 넣어 perception enemy와 섞는 오류.
3. `HoldFire`를 hard constraint로 오해; 현재는 soft score라 발사 가능.
4. order weight 범위 미검증과 score clamp로 의도 강도 손실.
5. current BT movement tasks가 이동 완료 전에 success하거나 영구 `InProgress`가 되는 lifecycle 문제.
6. duplicate cover writers가 mission objective를 덮어쓰거나 non-determinism 증가.
7. perception lost cache가 clear되지 않아 mission 복귀를 막는 문제.
8. group-less allied witness null dereference 후보.
9. `OnLeaderDied` 미연결, dead/destroyed member가 Members 배열에 남는 문제.
10. `CallOut` team filter 부재로 적에게 정보 전파 가능.
11. LLM timeout 후 late callback의 double apply/next timer clear.
12. GameInstanceSubsystem queue가 level transition을 넘겨 이전 world 요청을 이어받는 문제.
13. group memory unbounded growth와 stale information 사용.
14. parse failure가 doctrine fallback으로 가지 않는 문제.
15. `Retreat` 진입 5초 후 actor 비활성화가 mission monitor/재사용을 깨뜨리는 문제.
16. 현재 dirty worktree와 `.uasset` 변경이 많아 기준 commit만으로 결과 재현 불가.

## 11. Unknowns Requiring Runtime Verification

### 에디터/에셋

- `BP_RetryNPCCharacter` parent, AIControllerClass, Auto Possess AI, component/widget/weapon defaults
- `BP_RetryNPCController` parent 및 `BehaviorTreeAsset`
- `BT_LowIntelNPC`의 full graph와 C++ task 18종 중 실제 사용 subset
- `BB_NPC`의 모든 key/type 및 `E_CombatState` 값 순서
- `BP_GroupManager` parent와 GroupID/threshold defaults
- `Lvl_ThirdPerson`의 GroupManager/NPC/patrol/NavMesh actor와 instance reference
- GameMode/PlayerController/DefaultPawn/HUD/World Settings override
- DataTable row struct와 dialogue rows

### 런타임

- 현재 워킹트리 PIE에서 group registration과 set-order 이후 state/BT branch 변화
- target loss 후 Search/Patrol 복귀
- path failure와 MoveTo task 완료 semantics
- leader death callback 실제 유무
- LLM timeout/late response race 재현
- package/cook에서 map 및 fallback table 포함 여부

## 12. Recommended Minimal Change Plan

이 절은 구현 제안이며 **실제 코드는 수정하지 않았다.**

1. 기존 `ENPCOrder -> score -> CombatState -> BT` 경로를 legacy execution adapter로 유지한다.
2. 독립 `FCommandIntent` 데이터 타입을 추가하고 LLM transport와 분리한다.
3. 각 `AGroupManagerActor`가 current command와 derived `FMissionContext` 하나를 소유하게 한다. 기존 member registry와 `SetOrderForAll`을 재사용한다.
4. 최소 Mission Resolver는 command를 (a) legacy order/score modifiers, (b) mission target actor/location, (c) hard constraint flags, (d) completion criteria로 변환만 한다.
5. `UNPCDecisionComponent`에는 optional mission overlay만 추가한다. 기존 base score식을 유지하고 modifiers를 합성하며 emergency Dead/Reload/target response는 우선한다.
6. Blackboard에는 전투 target과 분리된 최소 mission keys만 추가한다. 기존 BT task 중 `MoveToTarget`, `LookAround`, `SearchArea`, `CallOut` 재사용 가능성을 Editor graph에서 먼저 검증한다.
7. 성공/실패는 BT task의 immediate result가 아니라 group-level monitor가 objective/health/path/report를 관찰해 판정한다.
8. Team Operational Memory는 report receipt를 gate로 하는 `UWorldSubsystem` 하나로 만들고 개인/group memory 전체를 복제하지 않는다.
9. Phase 7 전에는 LLM command 생성을 연결하지 않는다. 하드코딩 command로 전체 파이프라인을 먼저 검증한다.
10. LLM 연결 전에 queue cancel/reset, request completion guard, schema validation, group doctrine fallback을 보강한다.

### Phase 2 진입 게이트

- `EDITOR_ACTIONS.md`의 asset/PIE 검증을 사용자가 완료한다.
- native/BP CombatState ordinal 일치를 확인한다.
- current BT의 mission target 수용 가능성과 active task subset을 확인한다.
- Group A/B actor 및 member references를 확인한다.
- 현재 PIE baseline에서 fatal 없이 전투/명령 흐름이 동작함을 확인한다.
- 위 결과에 따라 제안 파일/Blackboard key를 Phase 2 계획에서 확정한다.

## 13. Unreal MCP Editor Verification Addendum

Phase 1의 에디터 미확인 항목을 2026-08-02 현재 열린 Unreal Editor 세션에서 직접 검증했다. 세부값은 `EDITOR_ACTIONS.md` 11절과 `EDITOR_DEPENDENCY_MAP.md` 9절에 기록했다.

### 분석을 확정하거나 수정하는 결과

1. BP parent/default와 level instance reference는 분석의 추론과 일치했다.
2. `BTService_Decision`은 root Selector 범위에 있으므로 일부 branch에서만 decision update가 멈추는 구조는 아니다.
3. BT는 37개 node로 구성되며 실제 C++ task subset과 branch가 확정됐다. `Hold` branch는 없다.
4. 모든 CombatState decorator `FlowAbortMode=None`이므로 상태 변경 시 active branch abort가 자동 보장되지 않는다.
5. `BB_NPC`는 11개 key만 가진다. C++ write map에 적힌 8개 key는 asset에 없다. 이 항목은 더 이상 “Editor에서 확인할 차이”가 아니라 확인된 schema mismatch다.
6. native/BP enum 순서는 일치하며 사용 branch의 decorator ordinal도 일치한다.
7. Group A/B는 각각 leader 1 + member 1로 연결돼 있다. 관련 BP graph가 비어 있으므로 `OnLeaderDied` 미연결은 확인된 사실이다.
8. 현재 워킹트리 PIE는 fatal 없이 전투/메모리/LLM 요청 실패 폴백까지 실행됐다. LLM 서버 미응답 때문에 order 성공 적용 경로는 미검증이다.

### Phase 2 계획에 강제할 제약

- Blackboard 확장은 실제 누락 key를 무조건 모두 추가하지 말고 mission과 기존 BT에서 필요한 최소 key만 선정한다.
- mission state 변경은 현재 `Observer Aborts=None`과 task 완료 semantics를 전제로 별도 안전장치/테스트를 계획한다.
- 기존 order adapter 변경 전 localhost LLM 성공 응답 또는 하드코딩 `SetOrderForAll` 테스트 진입점으로 회귀 기준을 만든다.
- Blackboard는 C++ 내부 판단 후 상태 중심으로 전달하는 의도된 구조이므로 미존재 key를 일괄 추가하지 않는다.
- `Observer Aborts=None`은 현재 전환 중인 알려진 BT 부채로 기록하고 후속 수정한다. Phase 2 진입 blocker로 사용하지 않는다.

### 갱신된 Phase 2 진입 판정

Phase 1 조사와 처리 방침 확인이 완료됐다. 현재 판정은 **Phase 1 Complete / Phase 2 Ready / Implementation Not Started**다.
