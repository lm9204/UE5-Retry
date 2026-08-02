# LLM Command System — Codex Handoff

이 디렉터리는 현재 게임 코드베이스에 계층형 LLM 지휘 시스템을 추가하기 위한 초기 문서 집합이다.

## 우선순위

1. `01_PROJECT_GOAL.md` — 프로젝트의 큰 목표와 설계 원칙
2. `02_CODEBASE_ANALYSIS.md` — Codex가 먼저 수행할 코드 흐름 분석
3. `03_TECHNICAL_SPIKE.md` — 첫 수직 슬라이스 기술 스파이크
4. `04_SCENARIO_LEVEL_LOADER.md` — 반복 가능한 시나리오 실행을 위한 간단한 레벨 로더
5. `05_WORK_ORDER.md` — 실제 작업 순서, 산출물, 중단 기준
6. `06_EDITOR_INTEGRATION.md` — C++ 구현과 Unreal Editor/BP 연결 책임 분리

## 작업 원칙

- 첫 단계에서는 기존 코드를 수정하지 않고 흐름을 분석한다.
- 분석 결과가 나온 뒤 최소 침습적인 인터페이스를 설계한다.
- LLM보다 먼저 하드코딩된 구조화 명령으로 전체 실행 파이프라인을 검증한다.
- 기존 `NPCDecisionComponent`, `CombatState`, Behavior Tree는 가능한 한 유지한다.
- LLM은 상위 의도와 임무를 결정하며, BT Task를 직접 호출하지 않는다.
- 모든 판단·명령·실행 결과는 ID로 연결 가능한 구조를 전제로 한다.
- `.uasset`, Blueprint, Widget, 레벨 배치는 완료로 추측하지 않고 `06_EDITOR_INTEGRATION.md`에 따라 사용자 작업으로 분리한다.
- 각 구현 Phase 종료 시 `Generated/EDITOR_ACTIONS.md`를 갱신한다.
- 학습자용 설명은 `Generated/LEARNING_GUIDE.md`에 누적하고, 기술 계획의 용어와 결정을 현재 프로젝트 사례로 풀어 쓴다.

## 첫 번째 Codex 프롬프트

```text
Docs/LLM_Command_System_Docs/README.md와 연결된 문서들을 모두 읽어라.

우선 02_CODEBASE_ANALYSIS.md의 지시만 수행하라.
아직 코드를 수정하지 말고 현재 코드 흐름, 의존성, 변경 후보 지점, 회귀 위험을 분석하라.

분석 결과는 문서에서 지정한 경로에 작성하고, 확인되지 않은 내용은 추측하지 말고 미확인으로 표시하라.
```
