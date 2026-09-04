# Retry LLM Command System — Agent Working Baseline

마지막 정리: 2026-09-04 (Asia/Seoul)

구현·검증 기록 기준일: 2026-08-10

목적: 다음 작업을 시작하는 에이전트가 **이 문서 하나로 현재 상태와 다음 순서를 파악**하도록 한다.

## 1. 작업 시작 규칙

1. 이 문서를 먼저 읽는다.
2. `git status --short`와 최근 커밋을 확인하여 이 문서 이후 변경을 찾는다.
3. 현재 작업과 직접 관련된 경우에만 상세 문서를 읽는다.
4. 계획을 구현 완료로 간주하지 않는다. 아래 상태 표기를 사용한다.

| 상태 | 의미 |
|---|---|
| `Planned` | 설계 또는 작업 순서만 있음 |
| `Code Complete` | 소스와 자동화 테스트 작성 완료 |
| `Automated Verified` | 사용자가 Automation 통과를 확인 |
| `Integrated Complete` | Asset 연결과 PIE 회귀까지 확인 |

## 2. 현재 한 줄 상태

Scenario 선택과 반복 실행, 구조화 Command, Recon/Secure/Defend Mission, Team Operational Memory, 규칙 기반 Commander Planner까지 연결됐다. 다음 독립 기능 배치는 기존 개인·그룹 LLM 요청의 결과를 재현하는 **Cached LLM Replay MVP**다.

## 3. 완료된 기능 배치

| 날짜 | 기능 배치 | 상태 | 결과 |
|---|---|---|---|
| 2026-08-04 | Scenario 실행·Command 기반 | `Integrated Complete` | 메뉴 선택, Seed/Run Context, Restart/Return, validation과 실행 기록 |
| 2026-08-05 | Recon Area vertical slice | `Integrated Complete` | 관측 후보 선택, 그룹 이동, 보고, `AreaObserved` 저장과 Command 완료 |
| 2026-08-06 | Secure Area와 Follow Up | `Integrated Complete` | `AreaObserved → Secure → AreaSecured`, 점유 판정과 수명 정리 |
| 2026-08-06 | Scenario `Use LLM` 정책 | `Integrated Complete` | 비활성 Scenario의 개인·그룹 HTTP 요청 이중 차단 |
| 2026-08-10 | Operational Objective와 Commander Planner | `Integrated Complete` | `AreaSecured → MaintainAreaControl → Defend Position` 지속 임무 |
| 2026-08-10 | AI Mission Debug Snapshot | `Automated Verified` | Command/Mission/Blackboard 동기화 UI와 테스트; 실제 PIE 화면 확인만 남음 |

## 4. 현재 실행 흐름

```text
Scenario Menu
→ UScenarioRuntimeSubsystem: Scenario ID / Seed / Run ID / Launch Options
→ AScenarioInitializer: 저장된 레벨 구성 검증과 Opening Order 시작
→ FCommandIntent validation / Group authority
→ Mission Resolver + World Adapter
→ Group 전원에게 FMissionContext 원자 배포
→ UNPCDecisionComponent가 Mission을 Blackboard에 투영
→ Behavior Tree가 Mission 이동과 기존 CombatState 전투를 실행
→ Report / Operational Fact
→ UTeamOperationalMemorySubsystem
→ Scripted Follow Up 또는 Operational Objective 활성화
→ FCommanderPlanner
→ 다음 구조화 Command
```

## 5. 확정된 설계 결정

- `Fact`, `Operational Objective`, `Command`, `Mission`, `CombatState`는 합치지 않는다.
- 상위 Command와 권위 상태는 C++이 소유하고 Blackboard에는 BT 실행용 최소 값만 투영한다.
- LLM보다 결정적인 C++ 규칙을 먼저 만들어 실행 기준선과 자동화 oracle로 사용한다.
- LLM은 Actor pointer나 임의 좌표가 아니라 semantic ID와 제한된 Structured Command만 제안한다.
- 실제 Actor, 위치, Team/Run 권한, NavMesh/path 실행 가능성은 C++에서 다시 검증한다.
- Group Mission은 전원 적용 또는 전원 실패하는 원자적 배포를 사용한다.
- Defend는 임의 타이머로 완료하지 않는 지속 Mission이다. 취소·재계획·Restart/Return이 종료 경계다.
- 후보 선택 우선순위는 `Hard Constraint > Command/Doctrine Weight > Leader Personality Modifier > Deterministic ID Tie-break`다.
- Scenario의 NPC·Group·Marker는 현재 레벨에 저장 배치하고 Initializer가 ID와 연결을 검증한다. DataAsset 기반 전체 Spawn은 아직 도입하지 않는다.
- Team Operational Memory는 World lifetime, Scenario 선택과 Run Context는 GameInstance lifetime을 사용한다.
- Dedicated Server와 분산 inference worker는 장기 방향이며 현재 기능 배치에 끼워 넣지 않는다.

## 6. 다음 작업 순서

### 6.1 직전 배치 마감

AI Mission Debug UI를 PIE에서 확인한다.

- Command/Mission 값과 target 표시
- 전투 중 Mission movement gate 중단과 전투 종료 후 복귀
- Command↔Mission, Mission↔Blackboard sync 색상
- Restart/Return 후 이전 표시 제거

### 6.2 Immediate Plan — Cached LLM Replay MVP

목적은 성능 cache가 아니라 테스트 재현성, A/B 비교, LLM 결정 검증이다.

```text
Record: request → HTTP → schema validation → JSON 저장 → 공통 결과 적용
Replay: request → stable key 조회 → schema/version 검증 → 공통 결과 적용
```

필수 규칙:

- 내부 상태는 `Disabled / Live / Record / Replay`로 구분한다.
- Replay는 HTTP를 만들지 않는다.
- Cache miss, version mismatch, 손상된 JSON은 명시적으로 실패하며 Live로 fallback하지 않는다.
- Record와 Replay는 같은 validation과 result application 경로를 사용한다.
- Scenario generation/Run ID, weak target, NPC death guard를 Replay에서도 적용한다.
- stable key에는 최소한 Scenario ID, Seed, Request Type, stable NPC/Group ID, 실제 prompt 입력 hash, Prompt/Schema Version, Run별 ordinal을 포함한다.
- 첫 A/B Scenario는 동일 초기 조건에서 `Memory 없음`과 `AllyDeath Memory 있음`만 비교한다.

완료 조건:

1. Scenario 시작 전에 Replay 선택 가능.
2. 검증된 개인·그룹 response를 JSON으로 Record.
3. 동일 입력에서 동일 key, Memory/Personality 변경 시 다른 key.
4. Replay HTTP 요청 0건과 miss의 live fallback 0건을 테스트로 증명.
5. Restart/Return과 requester 제거 뒤 stale 결과 적용 차단.
6. 동일 Scenario/Seed Replay 반복과 A/B PIE 비교 완료.

이번 배치에서 만들지 않는 것: 범용 cache browser/editor, migration framework, 다중 model UI, Shipping cache packaging, distributed worker scheduler.

### 6.3 Replay 이후

Phase 7 Structured LLM Command로 돌아간다.

```text
LLM JSON
→ transport DTO parse
→ schema / Command grammar validation
→ Team / Run / Target validation
→ 기존 Group authority
→ Mission Resolver
→ 기존 BT 실행
```

## 7. 구현 전 결정 게이트

Cached Replay를 시작하기 전에 사용자와 다음을 확정한다.

1. UI는 기존 `Use LLM + 캐시 사용` checkbox를 유지할지, 실행 Mode를 직접 노출할지.
2. 현재 없는 `ScenarioVersion`을 첫 cache key에 포함할지.
3. 개인·그룹 response의 필수 JSON 필드, 타입과 허용 범위.
4. 승인된 cache snapshot을 언제 source fixture로 승격할지.

후속 Phase에서 결정할 항목:

- Defend 취소와 재계획 조건
- `Advance`, `Regroup`의 의미와 완료 조건
- 여러 아군 Group의 거리·전투력·성격 candidate score
- 규칙 Planner와 LLM 제안을 조합하는 doctrine/fallback 정책
- BT decorator의 남은 `Observer Aborts=None` 부채
- Game target 및 등록 map cook/package 검증 시점

## 8. 설계 결정 기록 절차

Tradeoff가 생기면 에이전트가 임의로 확정하지 않는다.

1. 결정 대상, 지금 필요한 이유, 관련 Unreal/C++ 개념과 현재 코드 상태를 설명한다.
2. 각 선택지의 실제 게임 동작, 코드 영향, 장점, 단점, 변경 비용을 비교한다.
3. 권장안과 그 단점을 함께 제시하고 사용자 선택을 받는다.
4. 선택 뒤 사용자에게 아래 세 내용을 확인한다.
   - 가장 중요한 선택 이유
   - 알고도 감수하는 단점
   - 결정을 다시 검토할 조건
5. 확인된 결정을 아래 작업 일지에 기록한 뒤 구현한다.

## 9. Agent 작업 일지

### 2026-09-04 — 누적 변경 커밋과 문서 기준선 정리

- 구현 내용: 새 게임 기능 구현 없음.
- 수정 내용: 누적된 Commander/Defend와 AI Mission Debug 변경을 기능별 커밋으로 분리했다. 이 문서를 최신 단일 agent handoff로 갱신했다.
- 설계 선택: 새 handoff 파일을 추가하지 않고 오래된 `BASELINE_STATUS.md`를 재사용한다.
- 선택 이유: 문서 수를 늘리지 않으면서 작업 시작점을 하나로 고정할 수 있다.
- 감수한 단점: 과거 기준선 내용은 Git history에서 조회해야 한다.
- 재검토 조건: 이 문서가 300줄을 넘거나 여러 병렬 작업이 서로 다른 기준선을 요구할 때 분리한다.

## 10. 상세 문서 조회 규칙

- 현재 상태와 다음 작업: 이 문서
- 기술 구현 상세와 과거 Phase: `IMPLEMENTATION_PLAN.md`
- 사용자 학습 설명과 개념: `LEARNING_GUIDE.md`
- 에디터/Automation/PIE 절차: `EDITOR_ACTIONS.md`
- 초기 코드 구조 분석: `CODEBASE_FLOW_ANALYSIS.md`, 문제 조사 때만 사용
- 전체 장기 목표: `../01_PROJECT_GOAL.md`, 범위 결정 때만 사용

상세 문서를 매 세션 처음부터 모두 읽거나 같은 상태를 여러 문서에 반복 기록하지 않는다.
