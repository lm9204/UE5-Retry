# LLM Command System — Agent Document Router

이 디렉터리의 문서는 Retry 프로젝트의 계층형 LLM 지휘 시스템을 위한 설계·구현 기록이다.

## 에이전트 필수 읽기 순서

1. [`Generated/BASELINE_STATUS.md`](Generated/BASELINE_STATUS.md) — 현재 상태, 확정 설계, 다음 작업, 결정 일지
2. `git status --short`와 최근 커밋 — 기준 문서 이후 변경 확인
3. 현재 작업에 직접 필요한 상세 문서만 선택해서 읽기

처음부터 모든 문서를 읽지 않는다. 서로 충돌하면 현재 코드와 검증 기록을 확인한 뒤 `BASELINE_STATUS.md`를 갱신한다.

## 상세 문서 역할

| 문서 | 읽는 시점 |
|---|---|
| `01_PROJECT_GOAL.md` | 장기 범위나 아키텍처 경계를 결정할 때 |
| `02_CODEBASE_ANALYSIS.md` | 초기 분석 절차를 재검토할 때 |
| `03_TECHNICAL_SPIKE.md` | 원래 수직 슬라이스 의도를 확인할 때 |
| `04_SCENARIO_LEVEL_LOADER.md` | Scenario 기반의 초기 제약을 확인할 때 |
| `05_WORK_ORDER.md` | Phase 순서와 중단 기준을 확인할 때 |
| `06_EDITOR_INTEGRATION.md` | C++와 Unreal Editor 작업 책임을 나눌 때 |
| `Generated/IMPLEMENTATION_PLAN.md` | 과거 Phase의 구현 상세와 테스트 이름이 필요할 때 |
| `Generated/LEARNING_GUIDE.md` | 사용자에게 개념과 tradeoff를 설명할 때 |
| `Generated/EDITOR_ACTIONS.md` | Automation, Asset 연결, PIE 절차가 필요할 때 |
| `Generated/CODEBASE_FLOW_ANALYSIS.md` | 기존 호출 흐름을 깊게 조사할 때 |
| `Generated/EDITOR_DEPENDENCY_MAP.md` | Asset·Blueprint 의존성을 확인할 때 |

## 유지 규칙

- 진행 상태와 다음 우선순위는 `BASELINE_STATUS.md` 한 곳에만 최신값을 둔다.
- 상세 문서는 새 사실이나 영구 설계 근거가 있을 때만 갱신한다.
- 기능 배치 종료 시 상태를 `Code Complete / Automated Verified / Integrated Complete`로 구분한다.
- 계획을 구현 완료로 표현하지 않는다.
- 설계 tradeoff를 사용자가 선택하면 선택 이유, 감수한 단점, 재검토 조건을 작업 일지에 기록한다.
- 새 문서를 만들기 전에 기존 문서의 역할로 수용할 수 있는지 먼저 확인한다.
