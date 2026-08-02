# 2. 현재 코드 흐름 분석 요청

## 2.1 목적

새 시스템을 구현하기 전에 현재 코드가 실제로 어떻게 연결되어 있는지 확인한다. 문서의 가정과 코드가 다를 수 있으므로, 이 단계에서는 코드를 수정하지 않는다.

분석의 핵심 질문은 다음과 같다.

> 구조화된 Command와 Mission Resolver를 기존 AI 구조에 최소 침습적으로 삽입할 수 있는 위치는 어디인가?

## 2.2 반드시 조사할 항목

### A. 현재 LLM Order 흐름

1. `ENPCOrder`의 선언 위치와 전체 enum 값
2. `SetOrder(ENPCOrder, float)` 또는 유사 인터페이스 선언·구현 위치
3. 해당 함수의 모든 호출자
4. Order가 그룹 전체에 전달되는 과정
5. Order가 해제·갱신·중첩되는 규칙
6. Order가 NPC별 점수 가중치에 미치는 영향

### B. 개인 의사결정 흐름

1. `NPCDecisionComponent` 또는 대응 컴포넌트의 책임
2. 입력으로 사용하는 월드 상태, 메모리, 성격, 그룹 상태
3. CombatState 11종의 선언과 선택 과정
4. 점수 계산식과 가중치 적용 위치
5. 타겟 또는 위치가 결정되는 시점
6. 강제 상태, 오버라이드, 긴급 행동 경로 존재 여부
7. CombatState 변경 이벤트와 디버그 기능

### C. Behavior Tree 흐름

1. CombatState에서 Behavior Tree 또는 StateTree로 연결되는 과정
2. BT Task 17종 이상의 목록과 책임
3. 타겟 액터, 위치, 영역이 Blackboard에 기록되는 위치
4. 경로 실패, 타겟 소실, 전투 중단 처리
5. 임무 종료 후 기존 행동으로 복귀하는 경로
6. 정찰·관측·보고에 재사용 가능한 Task 또는 Service 존재 여부

### D. 그룹 시스템

1. `GroupManagerActor` 또는 대응 객체의 책임
2. 그룹 생성·해제·멤버 관리
3. 그룹장 또는 리더 개념의 존재 여부
4. 그룹 단위 명령·타겟·상태 저장 위치
5. 그룹과 개인 컴포넌트 간 의존 방향
6. 그룹별로 서로 다른 임무를 동시에 유지할 수 있는지
7. 상위 Team 또는 Faction 식별자가 존재하는지

### E. 메모리 시스템

1. `UMemoryComponent` 구조와 최대 20개 제한 처리
2. 개인 기억 타입 전체 목록
3. `FGroupMemoryEvent` 구조
4. 목격자 판정과 그룹 기억 생성 과정
5. 감정 점수 누적 및 LLM 평가 트리거
6. 기억 삭제·만료·중복 병합 규칙
7. 기억에 Source, Confidence, Timestamp 개념이 존재하는지
8. 개인 기억이 그룹 또는 다른 NPC로 전파되는 경로

### F. LLM 통합과 폴백

1. LLM 요청 생성 위치
2. 프롬프트 구성 요소
3. 응답 파싱과 유효성 검사
4. 타임아웃과 실패 처리
5. DataTable 기반 대사 폴백 흐름
6. 구조화 출력 또는 JSON 스키마 사용 여부
7. 하나의 NPC 요청과 그룹 요청의 차이

### G. 레벨 및 시나리오 실행 흐름

1. 현재 GameMode, GameInstance, Subsystem 구조
2. 레벨 전환에 사용 중인 함수와 UI
3. PIE 및 패키징 환경에서 레벨 이름을 열거할 수 있는 방법
4. 테스트 NPC가 배치되는 현재 방식
5. BeginPlay에서 자동 생성되는 관리자·NPC·그룹
6. 시나리오 상태 초기화가 필요한 전역 객체

## 2.3 산출물

분석 결과는 다음 파일에 작성한다.

```text
Docs/LLM_Command_System/Generated/CODEBASE_FLOW_ANALYSIS.md
```

문서 구조:

```text
1. Executive Summary
2. Current LLM Order Flow
3. NPC Decision and CombatState Flow
4. Behavior Tree Execution Flow
5. Group and Memory Flow
6. LLM Integration and Fallback
7. Level Loading and Scenario Initialization
8. Candidate Insertion Points
9. Files Requiring Modification
10. Regression Risks
11. Unknowns Requiring Runtime Verification
12. Recommended Minimal Change Plan
```

## 2.4 필수 표

### 호출 흐름 표

| 단계 | 클래스/파일 | 함수 | 입력 | 출력 | 다음 단계 |
|---|---|---|---|---|---|

### 변경 후보 표

| 후보 지점 | 현재 책임 | 추가할 책임 | 장점 | 위험 |
|---|---|---|---|---|

### 파일 영향 표

| 파일 | 변경 가능성 | 변경 이유 | 회귀 위험 | 테스트 방법 |
|---|---:|---|---|---|

## 2.5 분석 규칙

- 확인되지 않은 클래스명이나 흐름을 추측하지 않는다.
- 코드에서 확인한 사실과 설계 제안을 분리한다.
- 대규모 리팩터링보다 기존 구조를 보존하는 확장안을 우선한다.
- 아직 코드를 수정하지 않는다.
- 빌드가 가능한 환경이라면 분석 후 현재 기준 빌드 상태를 기록한다.
- 런타임 확인이 필요한 항목은 별도 목록으로 남긴다.

## 2.6 분석 완료 판정

다음 질문에 구체적으로 답할 수 있어야 한다.

1. `FCommandIntent`는 어느 모듈·파일에 두는 것이 적절한가?
2. `MissionContext`는 어떤 객체가 소유해야 하는가?
3. 그룹 명령은 어떤 호출 경로로 개인에게 전달되는가?
4. 목표 위치·타겟은 기존 Blackboard에 어떻게 주입할 수 있는가?
5. 기존 점수 가중치 시스템을 재사용할 수 있는가?
6. 명령 성공·실패를 어디에서 감시할 수 있는가?
7. Team Operational Memory는 Actor, Component, Subsystem 중 무엇이 가장 자연스러운가?
8. 레벨 변경 시 어떤 상태를 초기화해야 하는가?

## 2.x Unreal Editor / Blueprint Integration Analysis

`06_EDITOR_INTEGRATION.md`를 기준으로 다음 에디터 의존성을 반드시 조사한다.

- C++ 클래스와 파생 Blueprint/에셋의 연결 관계
- Maps & Modes, GameInstance, GameMode, PlayerController, HUD 설정
- Character BP, AIController, Behavior Tree, Blackboard 연결 위치
- 레벨에 수동 배치되는 Manager/Initializer/Volume/Marker 목록
- Widget 생성 및 Viewport 추가 경로
- DataAsset/DataTable 생성·참조 위치
- NavMesh와 NPC 스폰에 필요한 레벨 구성
- 신규 기능 구현 후 사용자가 에디터에서 해야 할 작업 후보

분석 산출물에는 `Generated/EDITOR_DEPENDENCY_MAP.md`를 추가하고, 코드 파일과 `.uasset`/레벨 의존성을 구분해 기록한다.
