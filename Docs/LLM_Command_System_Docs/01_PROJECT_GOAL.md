# 1. 프로젝트 큰 목표

## 1.1 목표

LLM을 단순 대화 생성기가 아니라 **계층형 지휘 판단기**로 사용하여, 플레이어가 전장을 관찰했을 때 각 조직이 제한된 정보와 지휘관의 성격에 따라 독립적으로 판단하고 움직이는 것처럼 느끼게 한다.

최종적으로 다음 흐름을 구현한다.

```text
World Event / Observation
→ Personal or Group Knowledge
→ Report / Communication
→ Team Operational Memory
→ Commander Decision
→ Structured Command
→ Subordinate Mission Decomposition
→ Existing NPC Decision / CombatState
→ Behavior Tree Execution
→ Result Report
→ Memory Update / Replanning
```

## 1.2 예시 시나리오

미군과 소련군이 하나의 전략적 요충지를 두고 경쟁한다.

- 소련군은 해당 지역을 통제하고 있다.
- 미군 HQ는 지역의 대략적인 지형만 알고 있다.
- 실제 도로 봉쇄, 바리케이드, 적 주둔 위치 등 세부 상태는 모른다.
- 미군 HQ는 정보 부족을 인식하고 정찰 그룹에 지역 정찰을 명령한다.
- 정찰 그룹은 요구된 정보를 얻기 위해 관측 지점을 선택하고 이동한다.
- 정찰 결과가 무전으로 보고되면 미군 Team Operational Memory가 갱신된다.
- HQ는 갱신된 정보를 근거로 두 개 전투 그룹에 서로 다른 접근 임무를 하달한다.
- 소련군도 정찰 활동이나 적 움직임을 감지하면 증원·방어·차단 명령을 생성한다.
- 하위 지휘관은 상위 의도를 유지하면서 자신이 가진 지역 정보와 성격에 따라 부하에게 세부 임무를 분배한다.

## 1.3 LLM을 사용하는 이유

규칙 기반 AI만으로도 각 행동은 구현할 수 있지만, 다음 요소가 동시에 결합되면 조건 분기가 급격히 증가한다.

- 불완전하고 오래된 정보
- 지휘관별 성격과 위험 선호
- 병력 손실과 피로
- 도로·지역·적 상태의 변화
- 상급자의 목적과 제약
- 여러 대안의 비교
- 예상하지 못한 상황에서의 재계획

LLM은 이 정보를 종합하여 **무엇을 왜 해야 하는지** 결정한다. 실제 이동, 엄폐, 사격, 수색 등은 기존 게임 AI가 수행한다.

## 1.4 핵심 책임 경계

```text
LLM Commander
    └─ 목표, 우선순위, 부대 배정, 대안 선택

Structured Command / Mission
    └─ 동사, 대상, 위치, 제약, 정보 요구, 성공·실패 조건

Mission Resolver
    └─ 명령을 기존 AI의 목표, 타겟, 제약, 가중치로 변환

NPCDecisionComponent / CombatState
    └─ 현재 상황에서 실행할 전술 상태 선택

Behavior Tree
    └─ 이동, 엄폐, 사격, 관측, 보고 등 원자 행동 실행
```

LLM은 BT Task 이름이나 NavMesh 좌표를 직접 생성하지 않는다.

## 1.5 메모리 계층

### Personal Memory

- 개인이 직접 경험한 사건
- 감정 변화
- 관계와 신뢰
- 직접 받은 명령
- 개인의 불완전한 관찰

### Group Memory

- 목격자 또는 보고를 통해 그룹이 공유한 지역 정보
- 현재 그룹 임무
- 그룹원의 상태
- 지역적 위협과 결정

### Team Operational Memory

- HQ가 알고 있는 작전 정보
- 지역별 통제 상태
- 경로 상태
- 적 병력 추정
- 아군 그룹별 임무와 전투력
- 사용 가능한 지원 자산
- 정보 요구사항과 불확실성

개인 기억의 전체 합집합을 HQ에 전달하지 않는다. 보고를 통해 요약·검증된 작전 정보만 상위 계층으로 전파한다.

## 1.6 명령 표현 원칙

명령은 동사와 목적어를 독립적으로 확장할 수 있는 구조로 만든다. 다만 모든 조합을 허용하지 않고 유효 조합을 검증한다.

초기 후보:

```text
Verbs: Recon, Secure, Defend, Block
Targets: Area, Route, Position, Unit, Information
```

초기 유효 조합:

```text
Recon + Area
Recon + Route
Secure + Area
Defend + Position
Block + Route
```

명령은 다음 정보를 포함한다.

- 발행자
- 수행 그룹
- 동사와 대상
- 의미적 Target ID와 물리적 위치
- 우선순위
- Hard / Soft 제약
- 정보 요구사항
- 성공·실패 조건
- 상위 명령 ID
- 현재 상태

## 1.7 성격의 역할

초기 성격 축:

- Aggression
- RiskTolerance
- CasualtyAversion
- Initiative
- InformationSeeking

성격은 불가능한 행동을 가능하게 만들지 않는다. 가능한 대안의 우선순위, 정보 충분성 임계치, 철수 판단, 위험 감수 등에만 영향을 준다.

## 1.8 판단 로그 목표

판단 로그는 다른 AI가 분석하기 쉬운 구조를 우선한다. 자유로운 긴 사고과정보다 다음 항목을 구조화해 기록한다.

- Decision Trigger
- Context Snapshot ID
- 사용한 Fact ID
- 선택한 Command
- 거절한 대안
- 성격의 영향
- 신뢰도
- 정보 공백
- 예상 결과
- 실제 실행 결과

전체 인과관계는 다음처럼 ID로 추적 가능해야 한다.

```text
ObservationEventId
→ ReportId
→ FactId
→ DecisionId
→ CommandId
→ ExecutionId
→ ResultEventId
```

## 1.9 현재 구현에서 유지할 부분

현재 구조:

```text
LLM Order 5종
→ 점수 가중치 보정
→ CombatState 11종 자동 계산
→ BT Task 17종 이상 자동 선택
```

이 하위 실행 구조는 제거하지 않는다. 새 Mission Resolver가 기존 가중치, CombatState, 타겟 정보를 제어하는 상위 계층으로 추가되는 방향을 우선 검토한다.

## 1.10 현재 구현에서 부족한 부분

- LLM 명령이 특정 대상·위치·부대·완료 조건을 표현하지 못한다.
- 그룹보다 상위인 Team Operational Memory가 없다.
- 관찰과 상위 팀 인지 사이의 보고·통신 흐름이 부족하다.
- 명령 수명주기와 성공·실패 판정이 없다.
- 판단과 실행 결과를 연결하는 로그 구조가 없다.
- 현재 폴백은 대사 폴백이며, 명령 실패용 Doctrine 폴백은 별도로 필요하다.

## 1.11 범위 밖

첫 기술 스파이크에서는 다음을 확정하거나 구현하지 않는다.

- 대대 전체 26명 지휘관의 LLM화
- 완전한 군 편제 시스템
- 모든 동사·목적어 조합
- 범용 HTN 플래너
- 최종 LLM 모델 선정
- 최종 프롬프트 튜닝
- 실제 군사 교리의 완전한 재현
