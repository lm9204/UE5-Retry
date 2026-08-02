# 6. Unreal Editor / Blueprint Integration Contract

## 6.1 목적

Codex CLI는 C++ 소스, 설정 파일, 텍스트 문서를 안정적으로 수정할 수 있지만, `.uasset` 기반 Blueprint, Widget Blueprint, DataAsset, 레벨 배치와 같은 에디터 작업은 프로젝트 환경과 사용자 확인이 필요하다.

따라서 모든 구현 작업은 다음 두 산출물로 나눈다.

1. **Codex 수행 영역**: C++ API, 데이터 구조, 런타임 로직, 로그, 테스트용 진입점
2. **사용자 에디터 작업 영역**: BP 생성·상속·프로퍼티 할당·레벨 배치·입력 연결·에셋 저장

Codex는 에디터 작업이 필요한 변경을 완료했다고 표현해서는 안 된다. C++ 구현 완료와 에디터 통합 완료를 별도 상태로 보고한다.

---

## 6.2 기본 원칙

### C++ 우선

핵심 규칙과 상태 변화는 C++에 둔다.

- Command 상태 전이
- Mission Resolver
- Scenario 선택 상태
- Scenario 초기화
- Run ID / Seed / 로그
- 명령 검증
- 성공·실패 판정

Blueprint는 다음 역할만 담당하는 것을 기본값으로 한다.

- UI 레이아웃
- 에셋 참조 설정
- 테스트 레벨 배치
- 디버그 버튼과 표시
- 시각적 연출

### Blueprint 노출은 최소화

에디터에서 실제로 연결할 값만 `BlueprintReadOnly`, `EditDefaultsOnly`, `EditInstanceOnly`, `BlueprintCallable`, `BlueprintImplementableEvent` 등으로 노출한다.

모든 내부 필드를 무조건 `BlueprintReadWrite`로 열지 않는다.

### BP 연결 실패 시에도 진단 가능

필수 에셋이 할당되지 않았거나 Scenario Definition이 누락된 경우:

- 크래시하지 않는다.
- 명확한 오류 로그를 남긴다.
- 실패한 에셋/프로퍼티 이름을 출력한다.
- 가능하면 안전한 기본 동작으로 복귀한다.

---

## 6.3 Codex가 각 Phase 종료 시 반드시 작성할 문서

각 구현 Phase가 끝날 때 다음 파일을 갱신한다.

```text
Docs/LLM_Command_System_Docs/Generated/EDITOR_ACTIONS.md
```

각 항목은 아래 형식을 따른다.

```text
## [Phase / 기능명]

### Codex가 구현한 것
- 수정 파일
- 추가 클래스/구조체
- 추가된 Blueprint API

### 사용자가 에디터에서 해야 할 것
1. 생성할 에셋의 정확한 타입과 권장 이름
2. 부모 클래스
3. 설정할 프로퍼티와 값
4. 배치할 레벨과 위치
5. 연결할 이벤트/함수
6. 저장·컴파일 순서

### 검증 절차
1. PIE 실행 전 확인
2. PIE에서 수행할 조작
3. 기대 로그와 화면 결과
4. 실패 시 확인할 항목

### 현재 통합 상태
- [ ] C++ 빌드 성공
- [ ] BP 생성
- [ ] BP 프로퍼티 연결
- [ ] 레벨 배치
- [ ] PIE 검증
```

에디터 작업이 없으면 `사용자 에디터 작업 없음`이라고 명시한다.

---

## 6.4 코드베이스 분석 단계에서 조사할 에디터 의존성

Codex는 코드 흐름과 함께 다음을 조사한다.

- C++ 클래스에서 파생된 기존 Blueprint 목록 또는 예상 참조 경로
- GameMode, GameState, PlayerController, HUD, GameInstance의 실제 프로젝트 설정
- AIController, Character BP, Behavior Tree, Blackboard 에셋 연결 위치
- GroupManagerActor가 레벨에 직접 배치되는지 런타임 생성되는지
- Widget 생성과 Viewport 추가 경로
- DataTable / DataAsset이 어떤 BP 또는 설정에서 참조되는지
- 레벨별 World Settings 오버라이드
- Project Settings의 Maps & Modes 및 기본 맵
- Enhanced Input 또는 기존 입력 연결 방식
- NavMeshBoundsVolume과 AI 스폰 가능 조건
- Subsystem, GameInstance처럼 레벨 전환 후 유지되는 상태

분석 결과에는 코드 파일뿐 아니라 **사용자가 열어 확인해야 할 에디터 화면과 에셋**도 기록한다.

---

## 6.5 시나리오 로더의 권장 책임 분리

### Codex가 구현

- `UScenarioDefinition` 또는 프로젝트 관례에 맞는 데이터 구조
- `UScenarioRegistrySubsystem`
- 선택된 Scenario ID / Seed / 옵션 보존
- 레벨 전환 함수
- `AScenarioInitializer` 또는 대응 컴포넌트
- 초기화 성공·실패 로그
- Widget이 호출할 `BlueprintCallable` API

### 사용자가 에디터에서 연결

초기 권장 작업 예시:

1. `DA_TS_ReconSecure_001` Scenario DataAsset 생성
2. 테스트 월드 지정
3. Scenario ID, Seed, 옵션 입력
4. `WBP_ScenarioSelect` 생성
5. Codex가 제공한 C++ Widget 부모가 있다면 해당 클래스를 부모로 지정
6. Scenario 목록, Start, Restart, Return 버튼을 디자이너에서 배치
7. 버튼 이벤트에서 C++ `BlueprintCallable` 함수 호출
8. 메뉴용 GameMode 또는 PlayerController에서 Widget 생성·표시
9. 테스트 레벨에 `BP_ScenarioInitializer` 또는 C++ Actor 배치
10. 필요한 Spawn Point, Objective Area, Route Marker 참조 할당

코드베이스 분석 결과 기존 UI 프레임워크가 있으면 위 구조보다 기존 관례를 우선한다.

---

## 6.6 기술 스파이크의 에디터 작업 단위

기술 스파이크는 한 번에 전체 BP를 요구하지 않고 아래 순서로 연결한다.

### 단계 A — 명령 데이터 구조

- 에디터 작업 없음이 기본
- 필요한 경우 디버그용 DataAsset만 생성

### 단계 B — ReconArea 하드코딩 실행

사용자 작업 가능성:

- 테스트 레벨 준비
- Recon 그룹 BP 또는 기존 NPC BP 배치
- 목표 Area Actor 배치
- Observation Point Actor 2~3개 배치
- Group ID, Team ID, Target ID 설정

### 단계 C — 보고와 Team Memory

사용자 작업 가능성:

- Radio/Communication 컴포넌트가 BP 구성 요소라면 기존 NPC BP에 추가
- 디버그 Widget에 Team Fact 목록 표시 연결

### 단계 D — SecureArea

사용자 작업 가능성:

- Objective Area의 통제 판정 볼륨 배치
- Route A/B Marker와 바리케이드 참조 연결
- 전투 그룹 Spawn Point 지정

Codex는 실제 코드 분석 전에는 BP 이름이나 부모 클래스를 확정하지 않는다.

---

## 6.7 에디터 작업을 줄이기 위한 API 설계

가능하면 다음 방식으로 수동 연결량을 줄인다.

- ID 기반 자동 등록 컴포넌트
- `GetAllActorsWithInterface` 남용 대신 명시적 Registry 사용
- Scenario Definition에서 Soft Object Reference 사용
- Actor Tag는 보조 수단으로만 사용
- 필수 참조는 `EditInstanceOnly`와 검증 함수 제공
- `CallInEditor` 검증 함수 제공 검토
- 테스트용 `ValidateScenario()`와 `DumpScenarioSetup()` 제공

예:

```cpp
UFUNCTION(CallInEditor, BlueprintCallable, Category="Scenario|Validation")
void ValidateScenarioSetup();
```

이 함수는 누락된 그룹, 목표, 관측 지점, 경로 참조를 로그로 출력한다.

---

## 6.8 Codex 작업 지시 규칙

Codex에 구현을 요청할 때 항상 다음 조건을 포함한다.

```text
.uasset 또는 Blueprint 연결이 필요한 부분을 추측하여 완료 처리하지 마라.
C++ 변경 후 사용자가 Unreal Editor에서 수행해야 할 작업을
Generated/EDITOR_ACTIONS.md에 정확한 순서로 작성하라.

각 Blueprint 노출 API에 대해:
- 왜 BP에 노출했는지
- 어느 BP/레벨에서 호출하거나 설정하는지
- 필수인지 선택인지
를 기록하라.

에디터 연결 전에도 빌드 가능한 상태를 유지하고,
누락된 에셋 참조는 크래시가 아니라 명확한 오류로 처리하라.
```

---

## 6.9 완료 정의

기능 완료는 두 단계다.

### Code Complete

- C++ 빌드 성공
- 자동화 가능한 단위 테스트 또는 검증 함수 통과
- Editor Actions 문서 작성

### Integrated Complete

- 사용자가 BP/에셋/레벨 연결 완료
- PIE에서 검증 절차 통과
- 예상 로그와 동작 확인
- 문서 체크박스 갱신

Codex는 `Code Complete`를 `Integrated Complete`로 표현하면 안 된다.
