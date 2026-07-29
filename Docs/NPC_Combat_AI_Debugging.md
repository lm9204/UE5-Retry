# UE5 NPC 전투 AI 디버깅 기록

## 배경

`NPCDecisionComponent`가 0.1초마다 여러 점수(Attack/Cover/Retreat/Reload)를 계산해 `CombatState`를 하나 결정하고, Behavior Tree는 그 상태만 보고 분기 실행하는 구조. "판단은 C++ 컴포넌트, 실행은 BT"라는 설계 원칙 아래 디버깅과 리팩토링을 진행.

## 버그 1 — Patrol 상태인데도 사격이 멈추지 않음

**증상**: CombatState가 Patrol로 바뀐 뒤에도 `FireOnce()`가 계속 호출됨.

**원인**: `WeaponComponent::StopAim()`은 `bIsAiming = false`만 하고 `StopFire()`를 호출하지 않았음. FullAuto 무기는 `StartFire()`가 `FireTimerHandle`이라는 반복 타이머를 걸어 `FireOnce()`를 직접 호출하는데, 이 타이머 콜백은 `bIsAiming`을 전혀 참조하지 않음. `Fire()` 진입 시점의 `bIsAiming` 체크는 최초 1회만 걸리는 가드였고, 한 번 타이머가 걸리면 탄약이 떨어지거나 `Reload()`가 명시적으로 호출되기 전까지 CombatState와 무관하게 영원히 발사됨.

**수정**: `StopAim()` 내부에서 `StopFire()`를 함께 호출. "조준 중이 아니면 발사 불가"라는 `Fire()`의 불변조건을 타이머 쪽에도 동일하게 적용.

## 버그 2 — 타겟을 바라보지 않고 사격

**증상**: 엄폐 상태에서 사격 시 회전하지 않음.

**원인**: `BTTask_FireFromCover`가 `LookAtRotation`을 계산만 해두고 `NPC->SetActorRotation()` 호출 자체가 코드에 없었음(동일 계열 `BTTask_FireAtTarget`에는 있었음). 타이밍 문제가 아니라 단순 누락.

**수정**: 계산된 회전값을 실제로 적용하는 한 줄 추가. 나중에 `BTTask_SuppressiveFire`에서도 동일한 패턴의 누락을 발견해 같이 수정.

## 버그 3 — 완전 엄폐 상태에서도 탄창 3개(90발)를 다 쏠 때까지 사격 지속

**증상**: 플레이어가 시야 완전 차단 상태로 은폐해도 NPC가 계속 조준사격.

**1차 시도(실패)**: `BTTask_FireAtTarget`/`BTTask_FireFromCover`에 `bCanSeeTarget` Blackboard 체크를 추가 → "판단이 실행 계층(BTTask)에 들어감"이라는 아키텍처 원칙 위반 지적을 받고 되돌림. 또한 이 과정에서 `bCanSeeTarget`이 애초에 Blackboard에 키로 등록조차 안 되어 있었다는 것도 발견 (그래서 사격이 아예 안 되는 회귀가 발생하기도 함).

**진짜 원인**: `CalcAttackScore()`가 `bCanSeeTarget`을 필수 조건이 아니라 보너스(+0.4)로만 취급해서, Aggression/Courage/Dominance 수치만으로도 시야 없이 Attack 점수가 1등을 할 수 있었음. `ShouldForceExit(Attack)`에 `!bCanSeeTarget` 조건이 있었지만 이건 "재평가 기회"만 줄 뿐 강제로 다른 상태를 고르게 하지는 않아서, 점수 계산 자체가 시야를 요구하지 않는 한 근본적으로 못 막는 구조였음.

**수정**: `CalcAttackScore`에 `if (!Ctx.bCanSeeTarget) return 0.f;` 하드 게이트 추가. 판단(점수 함수) 층위에서 해결해서 BT/Blackboard를 우회 통로로 안 쓰는 원칙을 지킴.

## 아키텍처 확장 — TakeCover와 "제압사격" 분리

기존엔 TakeCover 하나가 "이동+은신"과 "사격"을 동시에 담당(`BTTask_FireFromCover`가 무조건 3발 사격). 그런데 `CalcCoverScore`는 원래 시야 여부와 무관하게 계산되어야 정상(안 보여도 무서우면 숨는 게 맞음) — 그래서 버그3와 같은 방식(점수 게이트)으로는 해결이 안 되는 케이스였음.

**설계 변경**: `ENPCCombatState`에 `Suppress`를 신규 추가.
- `CoverScore`가 이겼을 때, 이미 엄폐 중이고(`bIsInCover`) 동시에 타겟이 보이면(`bCanSeeTarget`) → `Suppress` (기존에 있던 `BTTask_SuppressiveFire` 실행)
- 그 외(이동 중이거나 안 보임) → `TakeCover` (이동/은신 전용, 사격 없음)

즉 "숨을지 말지"는 여전히 `CalcCoverScore` 하나가 판단하고, "지금 쏠지 말지"는 그 위에 완전히 결정된 `bIsInCover && bCanSeeTarget` 값으로만 갈리도록 분리. BT Task 코드에는 조건을 전혀 넣지 않고, BT 그래프의 Decorator(Blackboard 조건 비교)로만 분기 — 판단은 여전히 C++에만 있고 BT는 실행만 하는 원칙 유지.

## 탄약/재장전 로직 개선

1. **평시 자동 재장전 부재**: `DecideState()`가 타겟이 없을 때(`!Ctx.Target`)는 Reload 여부 자체를 검토하지 않아서, 적을 발견하기 전까지 절대 장전을 안 하던 문제. → 타겟 없음 분기 맨 앞에 "무장 중이고, 재장전 중이 아니고, 탄창이 안 찼고, 예비탄약이 있으면" Reload로 가는 조건 추가.
2. **부수적으로 발견한 버그**: `FNPCContext.MaxAmmo`가 기본값 30에 고정된 채 실제 무기의 `MagSize`로 채워진 적이 없었음(`CollectContext()`에서 누락). `WeaponComponent`에 `GetReserveAmmo()`/`GetMagSize()` getter를 새로 추가해서 실제 값을 채우도록 수정.
3. **탄약+예비탄약 완전 고갈 시 Retreat 유도**: `CalcAttackScore`/`CalcReloadScore`는 탄약(예비탄약 포함)이 없으면 0점으로 완전히 배제, `CalcRetreatScore`엔 "더 싸울 방법이 없음" 조건에 +0.6 보너스 추가. (단, `CalcCoverScore`는 안 건드려서 개인 성격에 따라 Retreat 대신 TakeCover가 이길 가능성은 남아있음)

## 메모리 시스템 오검출 수정

**증상 1**: NPC가 타겟을 발견하자마자 "교전 후 적을 제압함"을 기록하는 경우가 있었음.
**원인**: 기록 조건이 `CurrentState==Attack && Stable==Idle && !Target`라는 **상태 전이 모양**만 보고 "죽였다"고 추측하는 방식이었음. 타겟이 실제로 죽은 경우든, 단순히 시야를 3초 이상 놓쳐 타겟을 잃은 경우든 똑같이 이 조건을 만족.
**수정**: `FNPCContext`에 `bTargetJustDied` 플래그를 추가하고, `CollectContext()`가 `HealthComponent::IsDead()`로 실제 사망을 확인해 타겟을 지우는 그 순간에만 세팅. 메모리 기록 조건을 이 플래그로 교체.

**증상 2(추가 발견)**: 탄약이 없어 4명 전원이 Retreat 상태인 상황에서, 플레이어가 아군 NPC 하나를 직접 처치했더니 그 아군을 타겟팅 중이던 적군 NPC가 "제압함"으로 기록.
**원인**: `bTargetJustDied`는 "내 타겟이 진짜 죽었는가"만 검증하고 "내가 그 순간 실제로 교전 중이었는가"는 검증하지 않음. 이미 후퇴해서 발을 뺀 NPC도 타겟이 (다른 무언가에 의해) 죽기만 하면 무조건 크레딧을 받아버림.
**수정**: 기록 조건에 `CurrentState`가 실제 교전 상태(Attack/TakeCover/Suppress/Reload)였는지를 추가로 검사. 후퇴/순찰/탐색 중에 타겟이 죽은 건 "내가 제압한 것"으로 치지 않음.

## 로깅 인프라 정비

디버깅 도중 로그에 "어떤 NPC"의 로그인지가 안 나와서 여러 NPC가 겹칠 때 원인 추적이 어려웠음. `Debug/CombatLogging.h`에 `GetCombatLogName(AActor*)` 헬퍼를 추가해 — NPC는 부여된 이름(`NPCName`, 예: "Aggressive"), 플레이어는 "Player", 그 외는 엔진 기본 이름으로 통일 — 전투/AI 관련 로그(AI 상태 전환, 무기 발사/재장전, 데미지/사망, 메모리 기록, 성격 적용, BT MoveToCover, 치트 명령 등) 전체에 적용.
