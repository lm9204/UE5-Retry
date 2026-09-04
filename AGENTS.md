# Repository Agent Instructions

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

## 5. Agent 기준 문서와 설계 결정 기록

- 작업 시작 시 `Docs/LLM_Command_System_Docs/Generated/BASELINE_STATUS.md`를 먼저 읽고 `git status --short`와 최근 커밋으로 최신성을 확인한다.
- 모든 설계 문서를 처음부터 읽지 않는다. 현재 작업에 필요한 상세 문서만 `Docs/LLM_Command_System_Docs/README.md`의 routing 표에 따라 조회한다.
- 진행 상태와 다음 우선순위의 단일 최신 기준은 `BASELINE_STATUS.md`다. 같은 상태를 여러 문서에 반복 기록하지 않는다.
- 기능 배치 종료 시 `Code Complete`, `Automated Verified`, `Integrated Complete`를 구분해 기준 문서의 상태와 작업 일지를 갱신한다.
- 사용자가 tradeoff를 비교해 방향을 선택하면 구현 전에 가장 중요한 선택 이유, 감수하는 단점, 결정을 다시 검토할 조건을 확인한다.
- 확인한 설계 결정은 `BASELINE_STATUS.md`의 Agent 작업 일지에 날짜별로 기록한다.
- 새 문서는 기존 문서로 수용할 수 없을 때만 만든다.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.



## Unreal Editor MCP 작업 절차

Unreal MCP를 사용하는 작업은 에디터 상태를 변경할 수 있다. 읽기 전용 조사도 asset editor, 독립 창, 선택 상태, PIE, observer를 남길 수 있으므로 아래 절차를 따른다.

### 1. 작업 전 상태 기록

- 현재 레벨, PIE 실행 여부, 열린 asset 목록, 선택된 actor/asset을 먼저 조회한다.
- 창이나 asset을 열 작업이면 기존에 열려 있던 항목을 기록한다. 사용자가 원래 열어 둔 창은 임의로 닫지 않는다.
- 수정 가능성이 있는 asset은 작업 전 dirty 상태를 기록한다.
- 현재 PIE가 사용자가 시작한 세션이면 명시적 요청 없이 중지하거나 재시작하지 않는다.

### 2. 최소 변경 원칙

- 검증 요청은 read-only tool을 우선하며 asset, Blueprint, level을 저장하지 않는다.
- asset editor나 독립 창은 필요한 대상만 열고, 한 번에 불필요한 창을 여러 개 열지 않는다.
- 선택, Content Browser 경로, 카메라 위치를 바꿀 필요가 없으면 변경하지 않는다.
- Slate `Observe`를 사용했다면 observer ID를 보관한다.

### 3. 열린 창과 asset 정리

- 에이전트가 연 asset editor, 독립 창, modal, tab은 해당 확인이 끝나는 즉시 닫는다.
- 작업 종료 전 `GetOpenAssets`와 Slate 최상위 창 목록을 다시 조회한다.
- 작업 전 목록에 없었고 에이전트가 연 항목만 닫는다. 기존 사용자 창과 tab은 보존한다.
- 일반 asset close API가 없으면 Slate snapshot으로 정확한 tab/window를 식별해 닫는다. 전체 Unreal Editor 창을 닫는 버튼과 혼동하지 않는다.
- 창을 안전하게 식별하거나 닫을 수 없으면 다른 창을 추측해 닫지 말고 남은 창을 사용자에게 보고한다.

### 4. PIE와 임시 상태 정리

- 에이전트가 시작한 PIE/Simulate는 검증과 로그 수집 후 반드시 중지한다.
- 에이전트가 만든 Slate observer는 작업 후 `Unobserve`하고 observer 목록에서 제거됐는지 확인한다.
- 에이전트가 바꾼 선택 상태, Content Browser 경로, 카메라 위치는 작업 목적상 필요하지 않으면 기록한 원래 상태로 복원한다.
- 에이전트가 임시로 조정한 log verbosity, console variable 또는 디버그 표시도 원래 값으로 복원한다.

### 5. 저장 및 dirty 검증

- 사용자가 asset 수정을 요청하지 않은 경우 `.uasset`, level, Blueprint를 저장하지 않는다.
- 작업 종료 전 열거나 조사한 주요 asset과 현재 level의 dirty 상태를 확인한다.
- read-only 작업으로 새 dirty asset이 생겼다면 원인을 확인하고, 사용자 변경을 덮어쓰지 않는 방법으로 원복한다. 안전한 원복이 불가능하면 저장하지 말고 보고한다.
- 기존 dirty 상태와 에이전트가 만든 dirty 상태를 구분한다.

### 6. 완료 보고 전 필수 체크

- [ ] 에이전트가 시작한 PIE/Simulate가 종료됨
- [ ] 에이전트가 연 asset/tab/window가 닫힘
- [ ] 사용자 기존 창과 선택 상태가 보존됨
- [ ] Slate observer가 모두 해제됨
- [ ] 의도하지 않은 dirty asset이 없음
- [ ] 실제로 변경한 asset과 저장 여부가 보고에 포함됨

위 정리가 끝나기 전에는 Unreal MCP 작업을 완료로 보고하지 않는다.

## Unreal 학습 협업 규칙

이 프로젝트의 목적은 기능 완성뿐 아니라 사용자가 Unreal Engine과 게임 아키텍처를 이해하면서 직접 만들어 보는 것이다. 작업 속도만을 이유로 설명과 사용자 경험을 생략하지 않는다.

### 1. 새로운 개념을 먼저 설명한다

- 새로운 Unreal 개념, 디자인 패턴 또는 아키텍처를 도입하기 전에 쉬운 말로 뜻과 필요성을 설명한다.
- 기술 용어만 나열하지 말고 현재 프로젝트의 실제 Actor, Level, Component, Subsystem 사례와 연결한다.
- 해당 개념이 일반적인 Unreal 프로젝트와 실무에서 언제 사용되는지 설명한다.
- 사용자가 지금 알아야 할 내용과 나중에 알아도 되는 세부사항을 구분한다.
- 설명하지 않은 새 용어가 연속해서 등장하지 않도록 하며, 등장하면 즉시 짧게 정의한다.

### 2. 설계 결정을 이해 가능한 형태로 질문한다

- 결정되지 않은 사항을 임의로 확정하지 않는다.
- `A 권장 / B`만 제시하지 않는다. 질문 전에 아래 내용을 설명한다.
  1. 무엇을 결정하는가
  2. 왜 지금 결정해야 하는가
  3. 관련 Unreal 또는 C++ 개념
  4. 현재 프로젝트 상태
  5. 각 선택을 했을 때 실제 게임과 코드가 어떻게 달라지는가
  6. 실무에서 흔히 쓰는 방식
  7. 권장안의 이유, 단점, 나중에 변경할 때의 비용
- 사용자가 선택의 의미를 이해한 뒤 결정할 수 있도록 질문한다.

### 3. 구현은 기능 단위, 테스트와 문서는 개념 단위로 나눈다

- 사용자에게 의미 있는 end-to-end 기능을 하나의 구현 배치로 묶는다.
- 배치 내부 코드는 개념별로 분리하고 자동화 테스트도 작은 유닛 단위로 작성하지만, 각 유닛마다 사용자에게 Live Coding·빌드·테스트를 요구하지 않는다.
- 기능 배치의 소스와 세밀한 테스트를 모두 작성한 뒤 사용자 반영과 검증을 한 번 요청한다.
- 배치 중간에는 구현을 바꾸는 설계 결정, 사용자 작업이 필요한 Asset schema, 안전 위험이 있을 때만 대화를 중단한다.
- 학습 목표, 개념, 관련 파일과 tradeoff는 `LEARNING_GUIDE.md`와 `IMPLEMENTATION_PLAN.md`에 계속 누적한다.

### 4. 사용자 실습과 검증은 기능 체크포인트에 모은다

- 사용자 에디터 작업과 실습은 각 유닛마다 요구하지 않고 기능 배치의 통합 체크포인트에 모은다.
- 체크포인트에서 에이전트 작업, 사용자 Asset 작업, 자동화 테스트 목록, PIE 통합 절차를 순서대로 한 번에 제공한다.
- 자동화 테스트는 유닛 단위 이름을 유지하되 사용자가 한 번의 코드 반영 후 목록 전체를 순차 실행하도록 한다.
- 사용자가 직접 할지 에이전트가 수행할지 확인하되, 반복적인 중간 테스트를 학습이라는 이유로 강제하지 않는다.
- 사용자가 작업한 결과에 문제가 있으면 즉시 대신 덮어쓰지 말고 먼저 현상과 원인을 설명한 뒤 수정 방향을 합의한다.

### 5. 학습 문서를 유지한다

- `Docs/LLM_Command_System_Docs/Generated/LEARNING_GUIDE.md`를 학습자용 기준 문서로 유지한다.
- 각 Phase에 다음 내용을 기록한다.
  - 눈에 보이는 목표
  - 핵심 개념과 쉬운 설명
  - 실무에서의 용도
  - 현재 프로젝트에 적용하는 이유
  - 선택하지 않은 대안과 tradeoff
  - 관련 클래스와 파일
  - 사용자 실습 후보
  - 검증 방법
  - 새 용어 정리
- `BASELINE_STATUS.md`를 현재 상태와 다음 작업의 단일 기준으로 유지한다.
- `IMPLEMENTATION_PLAN.md`는 과거 Phase의 기술 상세와 테스트 근거, `LEARNING_GUIDE.md`는 사용자용 쉬운 설명으로 유지한다.
- 일상적인 상태를 세 문서에 반복 추가하지 않고, 상세 문서는 새 영구 근거나 학습 설명이 생긴 경우에만 갱신한다.

### 6. 구현 중 설명과 완료 보고

- 핵심 코드의 소유권·생성·수명·호출 흐름은 문서에 기록한다. 대화에서는 구현을 막는 결정이나 위험만 먼저 설명한다.
- 코드 변경 후에는 파일 목록만 보고하지 말고 입력에서 결과까지의 실행 흐름을 설명한다.
- 기능 배치 종료 시 변경·테스트 결과와 함께 다음을 보고한다.
  - 이번에 사용된 Unreal/C++ 개념
  - 코드에서 개념을 확인할 위치
  - 현재 프로젝트가 그 방식을 선택한 이유
  - 사용자가 직접 수행한 부분
  - 아직 이해하거나 결정해야 할 부분
  - 다음 Phase의 선행 개념
- 사용자가 이해하지 못한 상태를 진행 승인으로 간주하지 않는다. 설명 요청이 있으면 구현을 잠시 멈추고 학습 문서를 먼저 보완한다.

### 7. 기능 배치 검증 순서

1. 에이전트가 기능 배치 전체와 세밀한 유닛 테스트를 작성한다.
2. 에이전트는 빌드하지 않고 정적 검사와 문서 갱신을 마친다.
3. 사용자가 Live Coding 또는 필요 시 사용자 주도 전체 빌드를 한 번 수행한다.
4. 에이전트가 제공한 자동화 테스트 목록을 사용자가 순차 실행한다.
5. 자동화가 모두 통과한 뒤 필요한 Asset/Blueprint/Level 병합과 PIE 실제 동작 검증을 수행한다.
6. 실패가 있으면 해당 실패만 수정하고 기능 체크포인트를 다시 검증한다.
