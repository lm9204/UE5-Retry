# 5. Codex 작업 순서 및 산출물

## 5.1 전체 원칙

- 분석과 구현을 한 번에 수행하지 않는다.
- 각 단계는 독립적으로 빌드·검증한다.
- 기존 기능 회귀를 막기 위해 작은 커밋 단위로 작업한다.
- 코드에서 확인되지 않은 가정을 문서 설계만 보고 구현하지 않는다.
- 명령 시스템보다 먼저 LLM을 연결하지 않는다.

### 현재 우선순위 예외 — Cached LLM Replay

2026-08-10 현재 다음 구현 배치는 [`Generated/BASELINE_STATUS.md`](Generated/BASELINE_STATUS.md)에 정리된 **Cached LLM Replay**다. 이는 Phase 7의 Structured LLM Command 생성이나 Long-term Worker architecture를 앞당기는 작업이 아니다. 이미 존재하는 개인/그룹 LLM request의 생성형 변동을 통제하기 위한 실험 인프라다.

```text
Immediate Plan: Cached LLM Replay — 이번 주 최우선
Technical Spike: 기존 Command / Mission / Team Memory 순서 유지
Long-term Architecture: Multiplayer / Distributed Worker — 이번 주 제외
```

Cached Replay 완료 뒤 기존 Phase 순서로 돌아간다. 범용 Cache Framework, Multiplayer, Worker Scheduler를 이 예외에 끼워 넣지 않는다.

## 5.2 Phase 0 — 기준 상태 확인

작업:

- 프로젝트 빌드 방법 확인
- 현재 빌드 성공 여부 기록
- 현재 테스트 레벨과 주요 AI 흐름 실행 확인
- 변경 전 기준 로그 또는 영상 확보 가능 여부 확인

산출물:

```text
Docs/LLM_Command_System/Generated/BASELINE_STATUS.md
```

완료 조건:

- 개발 환경과 빌드 명령이 문서화됨
- 기존 오류와 새 오류를 구분할 수 있음

## 5.3 Phase 1 — 코드 흐름 분석

입력:

- `02_CODEBASE_ANALYSIS.md`

작업:

- 문서의 조사 항목 수행
- 아직 소스 코드 수정 금지

산출물:

```text
Docs/LLM_Command_System/Generated/CODEBASE_FLOW_ANALYSIS.md
```

완료 조건:

- Command 및 Mission Resolver 삽입 후보가 근거와 함께 제시됨
- 변경 대상 파일과 회귀 위험이 식별됨
- 레벨 로더 구현에 사용할 기존 GameMode/GameInstance 구조가 확인됨

## 5.4 Phase 2 — 구현 계획 확정

작업:

- 분석 결과를 기반으로 구체적인 변경 계획 작성
- 실제 클래스명과 파일명 사용
- 새 파일과 수정 파일 구분
- 기존 시스템 재사용 방법 명시

산출물:

```text
Docs/LLM_Command_System/Generated/IMPLEMENTATION_PLAN.md
```

문서에 포함할 내용:

- 제안 아키텍처
- 클래스별 책임
- 데이터 흐름
- 수정 파일 목록
- 신규 파일 목록
- 단계별 테스트
- 롤백 전략
- 아직 결정하지 않은 항목

이 단계에서는 인터페이스 스텁 정도 외에는 구현하지 않는다.

## 5.5 Phase 3 — 시나리오 레벨 로더

입력:

- `04_SCENARIO_LEVEL_LOADER.md`

구현 범위:

- 등록된 테스트 시나리오 목록
- 레벨 선택 UI
- Scenario ID 및 Seed 전달
- Start / Restart / Return
- Scenario Initializer
- Run ID 생성과 초기화 로그

이 기능을 먼저 구현하는 이유:

- 이후 모든 기술 스파이크를 반복 가능하게 만든다.
- NPC 수동 배치와 상태 재설정 비용을 줄인다.
- 회귀 테스트의 공통 진입점이 된다.

완료 조건:

- 동일 시나리오를 UI에서 반복 실행 가능
- 이전 Run 상태가 남지 않음
- 등록된 기술 스파이크 레벨 로드 성공

## 5.6 Phase 4 — Command 데이터 구조 및 로그

구현 범위:

- Command enum과 구조체
- Command 상태 수명주기
- MissionContext
- Command 검증 결과 코드
- Run ID / Command ID 기반 이벤트 로그

아직 하지 않을 것:

- LLM 호출
- 범용 동사 시스템
- 모든 CombatState와의 연결

완료 조건:

- 하드코딩 Command를 생성·검증·상태 전이 가능
- 로그에서 하나의 Command 생명주기를 추적 가능

## 5.7 Phase 5 — ReconArea 기술 스파이크

입력:

- `03_TECHNICAL_SPIKE.md`

구현 범위:

- 하드코딩된 ReconArea 명령 주입
- Mission Resolver 최소 구현
- 관측 후보 선택
- 기존 AI 이동과 연결
- Operational Fact 생성
- Report 전송
- Team Operational Memory 반영
- 성공·실패 판정

완료 조건:

- ReconArea 전체 파이프라인 통과
- HQ 메모리에는 Report 수신 후에만 정보 반영
- 반복 실행 시 로그 비교 가능

## 5.8 Phase 6 — SecureArea 기술 스파이크

구현 범위:

- 하드코딩된 SecureArea 명령
- 두 그룹에 다른 접근 목표 또는 Route 제공
- 기존 전투 AI와 연결
- 목표 지역 점유 및 위협 상태 판정
- 완료·실패 보고

완료 조건:

- 기존 BT를 전면 재작성하지 않고 수행 가능
- 긴급 전투 대응과 임무 목표가 함께 유지됨
- 실패 이유가 구조화되어 기록됨

## 5.9 Phase 7 — LLM 연결

선행 조건:

- 하드코딩 명령으로 Recon → Report → Secure 전체 성공

구현 범위:

- 구조화 출력 스키마
- Command Validator
- HQ 한 명만 LLM 판단
- 사용 Fact ID, 대안, 신뢰도, 성격 영향 로그
- 응답 실패 시 Doctrine 폴백

초기 LLM 역할:

- Recon 필요 여부 판단
- 적절한 그룹 배정
- Secure 명령의 Route 또는 임무 분배 선택

LLM이 하지 않는 것:

- 좌표 생성
- BT Task 직접 선택
- 월드 상태 검증
- 성공 판정

## 5.10 커밋 권장 단위

```text
1. docs: add codebase analysis output
2. feat: add scenario registry and selection UI
3. feat: add scenario initializer and restart flow
4. feat: add command data types and validation
5. feat: add command lifecycle logging
6. feat: add minimal mission context integration
7. feat: implement ReconArea spike
8. feat: add operational report and team memory
9. feat: implement SecureArea spike
10. feat: connect structured LLM command generation
```

## 5.11 공통 테스트

각 구현 단계마다 확인한다.

- Development Editor 빌드
- 기존 NPC 전투 동작 회귀
- 레벨 전환 후 상태 초기화
- 동일 시나리오 반복 실행
- 잘못된 Command 입력 거부
- 경로 없음 상황 실패 처리
- 그룹 전멸 또는 통신 실패 처리
- 로그 ID 연결성

## 5.12 Codex 작업 규칙

Codex는 각 단계 시작 전에 다음을 출력한다.

1. 이해한 목표
2. 확인한 관련 파일
3. 수정 예정 파일
4. 예상 위험
5. 검증 방법

각 단계 종료 후 다음을 작성한다.

1. 실제 변경 파일
2. 핵심 변경 내용
3. 빌드 및 테스트 결과
4. 남은 위험
5. 다음 단계로 넘어가도 되는지 판단

## 5.13 즉시 중단 조건

다음 상황에서는 임의로 대규모 리팩터링하지 말고 보고한다.

- 문서의 가정과 실제 코드 흐름이 근본적으로 다름
- 기존 GroupManager가 계층형 그룹을 지원하지 못함
- MissionContext 추가에 주요 AI 시스템 전면 교체가 필요함
- 레벨 전환 시 핵심 상태가 안전하게 초기화되지 않음
- 기존 BT가 목표 위치와 임무 상태를 수용할 인터페이스가 없음
- 빌드 기준 상태가 이미 깨져 있음

## 5.14 첫 실행용 Codex 지시문

```text
Docs/LLM_Command_System_Docs/README.md와 모든 연결 문서를 읽어라.

현재는 Phase 0과 Phase 1만 수행한다.
아직 게임 로직이나 UI 코드를 수정하지 마라.

1. 현재 프로젝트의 빌드 및 실행 기준 상태를 확인하라.
2. 02_CODEBASE_ANALYSIS.md의 모든 조사 항목을 실제 코드로 검증하라.
3. 결과를 지정된 Generated 문서에 작성하라.
4. 확인된 사실, 추론, 미확인 항목을 명확히 구분하라.
5. 마지막에 최소 침습 구현 후보를 제안하되 구현하지 마라.
```

## Editor Integration Gate

모든 구현 Phase는 다음 두 상태로 보고한다.

- `Code Complete`: C++ 빌드 성공, API와 검증 로직 구현, `Generated/EDITOR_ACTIONS.md` 작성
- `Integrated Complete`: 사용자가 BP/DataAsset/레벨 연결 후 PIE 검증 완료

다음 Phase가 에디터 연결 결과에 의존하면 `Integrated Complete` 이전에 진행하지 않는다. `.uasset` 변경을 추측하거나 에디터 작업을 완료했다고 보고하지 않는다.

각 Phase 종료 산출물에 반드시 포함한다.

- 변경 파일 목록
- Blueprint 노출 API 목록과 노출 이유
- 사용자 에디터 작업 순서
- PIE 검증 절차
- 미연결 상태에서의 안전한 오류 처리
