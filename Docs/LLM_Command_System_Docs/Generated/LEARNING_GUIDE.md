# LLM Command System — Learning Guide

작성일: 2026-08-02 (Asia/Seoul)  
대상: Unreal Engine과 C++ 구조를 배우면서 이 프로젝트를 직접 만들고 싶은 개발자  
연결 문서: [`IMPLEMENTATION_PLAN.md`](IMPLEMENTATION_PLAN.md)

## 1. 이 문서가 필요한 이유

`IMPLEMENTATION_PLAN.md`는 무엇을 어느 파일에 구현할지 정확하게 통제하기 위한 기술 문서다. 그래서 이미 `Subsystem`, `DataAsset`, 객체 수명, 상태 전이 같은 개념을 아는 사람에게는 유용하지만 처음 배우는 사람에게는 클래스 이름 목록처럼 보일 수 있다.

이 문서는 같은 계획을 다음 질문에 맞춰 다시 설명한다.

- 게임 화면에서는 무엇이 달라지는가?
- 이 기능을 만들면서 어떤 Unreal 개념을 배우는가?
- 왜 이 방식이 필요한가?
- 실무에서는 어디에 사용하는가?
- 다른 방법은 무엇이며 왜 지금은 선택하지 않았는가?
- 실제 코드는 어디에서 확인할 수 있는가?

기술 계획을 전부 외울 필요는 없다. 각 Phase를 시작할 때 해당 절만 읽고, 구현한 뒤 다시 확인하는 용도로 사용한다.

## 2. 우리가 최종적으로 만들려는 것

현재 NPC는 대략 다음처럼 움직인다.

```text
그룹이 단순 명령을 전달한다
→ NPC가 공격/엄폐/후퇴 점수를 계산한다
→ CombatState를 고른다
→ Behavior Tree가 이동·사격 같은 행동을 실행한다
```

이 구조는 NPC 한 명이 지금 무엇을 할지 고르는 데 적합하다. 하지만 “A 지역을 정찰하고, 막힌 길을 보고한 뒤, 다른 두 그룹이 서로 다른 길로 진입한다” 같은 긴 목적은 표현하기 어렵다.

새 시스템은 기존 AI 위에 한 층을 추가한다.

```text
상위 목적: A 지역을 정찰하라
→ 구체적인 임무 정보로 변환
→ 기존 NPC 판단에 목표와 제한을 전달
→ 기존 CombatState와 BT가 실제 행동
→ 결과를 보고
→ 다음 명령 결정
```

핵심은 기존 전투 AI를 버리는 것이 아니다. 기존 AI가 잘하는 “지금 싸울지, 숨을지, 재장전할지”는 그대로 두고, 새 시스템은 “왜 그 장소에 가는지, 무엇을 알아내야 하는지, 언제 끝난 것인지”를 담당한다.

## 3. 전체 Phase를 게임 화면 기준으로 보기

| Phase | 플레이어에게 보이는 결과 | 배우는 핵심 개념 |
|---|---|---|
| 3 | 메뉴에서 테스트 시나리오를 골라 시작·재시작·복귀 | DataAsset, Subsystem, 레벨 전환, UMG, Soft Reference |
| 4 | 아직 큰 화면 변화는 없지만 명령의 생성부터 종료까지 로그로 추적 | `USTRUCT`, `UENUM`, 검증, 상태 전이, GUID |
| 5 | 정찰 그룹이 관측 지점을 골라 이동하고 정보를 HQ에 보고 | Mission Resolver, WorldSubsystem, Utility Score, 정보 범위 |
| 6 | 두 전투 그룹이 다른 경로로 목표 지역을 확보 | 제약 조건, 목표 판정, 기존 AI와 상위 임무 결합 |
| 7 | 하드코딩 명령 대신 HQ LLM이 구조화 명령을 제안 | JSON schema, 비동기 요청, validation, fallback |

Phase 3을 먼저 만드는 이유는 이후 실험을 같은 조건으로 반복하기 위해서다. AI가 좋아졌는지 비교하려면 매번 같은 인원, 위치, 장애물, Seed에서 시작할 수 있어야 한다.

## 4. 먼저 알아둘 Unreal의 기본 객체들

### Actor

레벨 안에 존재하거나 배치할 수 있는 객체다. NPC, 그룹 관리자, 목표 지역, 관측 지점이 Actor가 될 수 있다.

현재 프로젝트 예시:

- `ARetryNPCCharacter`: 레벨에서 움직이는 NPC
- `AGroupManagerActor`: 눈에는 보이지 않지만 레벨에 배치되어 그룹을 관리
- 앞으로 만들 `AScenarioInitializer`: 레벨에 배치되어 해당 레벨의 시나리오 구성을 확인하고 시작

실무에서는 위치가 있거나 World에 존재해야 하는 대상에 Actor를 사용한다.

### ActorComponent

Actor에 붙여서 책임을 나누는 객체다. 혼자 레벨에 배치되지 않고 소유 Actor와 함께 존재한다.

현재 프로젝트 예시:

- `UNPCDecisionComponent`: NPC의 현재 전술 상태 계산
- `UMemoryComponent`: NPC 개인 기억 보관
- `UHealthComponent`: 체력 관리

NPC 클래스 하나에 모든 코드를 넣지 않고 판단, 체력, 기억을 분리할 때 사용한다.

### GameInstance

게임 실행 동안 유지되는 큰 상자라고 생각하면 된다. 레벨 A에서 레벨 B로 이동해도 같은 GameInstance가 남는다.

따라서 메뉴에서 선택한 Scenario ID와 Seed처럼 레벨 전환 후에도 필요한 값을 보관하기 적합하다.

주의할 점은 오래 살아남는 만큼 이전 레벨의 Actor 주소를 보관하면 위험하다는 것이다. 이전 World가 사라졌는데 그 Actor를 계속 사용하려 할 수 있기 때문이다.

### World

현재 로드된 레벨과 그 안의 Actor들이 활동하는 실행 공간이다. 다른 레벨을 열면 기존 World는 사라지고 새 World가 만들어진다.

전장에서 부대가 알아낸 정보는 새 전장으로 이동했을 때 자동으로 사라지는 편이 안전하다. 그래서 Team Operational Memory는 World에 수명을 맞춘다.

### Subsystem

Unreal이 적절한 시점에 자동으로 하나 만들어 주는 관리 객체다. 직접 전역 singleton을 만드는 대신 특정 범위에 하나뿐인 서비스를 만들 때 사용한다.

- `UGameInstanceSubsystem`: 레벨 전환을 넘어 유지
- `UWorldSubsystem`: 현재 World 동안만 유지

실무에서는 저장 상태, 세션 관리, 월드 단위 registry처럼 여러 객체가 함께 사용하는 기능을 한 곳에서 제공할 때 자주 쓴다. 모든 기능을 Subsystem으로 만드는 것은 좋지 않으며, 명확하게 공유되는 수명과 책임이 있을 때 사용한다.

## 5. “수명과 소유권”을 쉬운 말로 이해하기

기술 문서의 수명과 소유권은 다음 두 질문이다.

1. 누가 이 데이터를 가지고 있는가?
2. 그 데이터는 언제 사라져야 하는가?

예를 들어 메뉴에서 Seed `1001`을 선택하고 전투 레벨을 연다고 하자.

- Seed가 메뉴 World Actor에 저장되어 있으면 메뉴 레벨이 닫힐 때 사라진다.
- GameInstanceSubsystem에 저장하면 전투 레벨에서도 읽을 수 있다.
- 전투 중 발견한 적 위치를 같은 GameInstanceSubsystem에 넣으면 재시작 후에도 남을 수 있다.
- 그 정보는 WorldSubsystem에 넣으면 전투 레벨이 닫힐 때 함께 사라진다.

현재 선택:

| 정보 | 저장 장소 | 이유 |
|---|---|---|
| 선택 Scenario, Seed, 실행 옵션 | GameInstanceSubsystem | 레벨 전환 후에도 필요 |
| Team Operational Memory | WorldSubsystem | 새 실행에 이전 전장 정보가 남으면 안 됨 |
| 그룹의 현재 Mission | 레벨의 GroupManager Actor | 그 그룹과 같은 World에서만 유효 |

실무에서 세이브 데이터, 매치 정보, 현재 레벨 상태가 서로 섞이지 않게 만드는 기본적인 설계 판단이다.

## 6. Phase 3 — Scenario Level Loader

### 눈에 보이는 목표

```text
게임 시작
→ Scenario 메뉴 표시
→ 테스트 Scenario 선택
→ Start
→ 지정된 전투 레벨 열기
→ 같은 구성을 Restart
→ 메뉴로 Return
```

NPC와 그룹은 매 실행마다 다시 손으로 배치하지 않는다. 테스트 레벨에 한 번 배치하고 저장해 둔 구성을 `AScenarioInitializer`가 찾아 검증하고 초기화한다.

### 6.1 DataAsset

DataAsset은 코드를 다시 컴파일하지 않고 에디터에서 설정값 묶음을 만들기 위한 Unreal asset이다.

`UScenarioDefinition`에는 다음처럼 “어떤 테스트를 열 것인가”를 저장한다.

- Scenario ID
- 화면 표시 이름과 설명
- 열 전투 레벨
- 기본 Seed
- LLM 사용 여부
- 자동 시작 여부

DataAsset이 NPC를 직접 움직이는 것은 아니다. 설정을 담는 문서에 가깝다.

실무 사용 예:

- 아이템 정의
- 무기 설정
- 퀘스트 정의
- 적 종류 설정
- 게임 모드/시나리오 설정

현재 프로젝트에도 `UItemDefinition`, `UWeaponDataAsset`이 있어 같은 관례를 재사용할 수 있다.

### 6.2 Primary DataAsset

Primary DataAsset은 일반 DataAsset에 “게임이 식별하고 로드할 수 있는 공식 ID”를 더한 형태다. Asset Manager와 연결하여 패키징 대상과 로딩을 관리하기 좋다.

이번 프로젝트에서는 Scenario를 명시적으로 등록하고 Soft Reference로 레벨을 열기 때문에 `UPrimaryDataAsset`을 사용한다.

처음에는 “ID가 있는 DataAsset” 정도로 이해하면 충분하다. Asset Manager의 전체 기능은 지금 외울 필요가 없다.

### 6.3 Developer Settings

`UScenarioRegistrySettings`는 Project Settings 화면에 “실행 가능한 Scenario 목록”을 제공한다.

자동으로 폴더의 모든 asset을 검색하지 않고 목록을 직접 등록하는 이유:

- 테스트용이 아닌 레벨이 메뉴에 노출되는 것을 방지
- 에디터와 패키징 빌드가 같은 목록 사용
- 누락된 asset을 시작 전에 검증 가능

단점은 Scenario를 만들 때 목록에 한 번 등록해야 한다는 점이다. 초기 프로젝트에서는 이 명시성이 자동 검색보다 디버깅하기 쉽다.

### 6.4 Soft Object Reference

일반적인 강한 참조는 해당 asset을 함께 로드하게 만들 수 있다. Soft Reference는 asset 경로만 기억하고 필요할 때 로드한다.

Scenario 메뉴에서 모든 전투 레벨을 한꺼번에 메모리에 올릴 필요가 없으므로 `TSoftObjectPtr<UWorld>`로 레벨을 가리킨다.

실무에서는 큰 맵, 캐릭터 외형, 아이템 asset처럼 필요할 때 로드하고 싶은 대상에 자주 사용한다. 다만 경로가 등록되지 않거나 cook에서 빠지면 로드에 실패할 수 있으므로 패키징 테스트가 필요하다.

### 6.5 Scenario Runtime Subsystem

`UScenarioRuntimeSubsystem`은 메뉴 Widget과 전투 레벨 사이에서 선택 정보를 전달한다.

예상 흐름:

```text
WBP_ScenarioSelect가 StartScenario 호출
→ 등록된 Scenario인지 검사
→ Scenario ID와 Seed 저장
→ OpenLevel
→ 새 레벨의 AScenarioInitializer가 저장값 조회
```

Widget이 직접 모든 상태를 보관하지 않는 이유는 Widget과 메뉴 World가 레벨 전환 때 사라지기 때문이다.

### 6.6 Scenario Initializer

`AScenarioInitializer`는 테스트 레벨에 배치하는 시작 담당 Actor다.

역할:

- 현재 열린 레벨이 선택 Scenario와 일치하는지 확인
- 새로운 Run ID 생성 또는 활성화
- 레벨에 배치된 Group/NPC/Marker의 ID와 참조 확인
- 이전 실행에서 남을 수 있는 시스템 상태 초기화
- 옵션에 따라 하드코딩된 테스트 시작

`ValidateScenarioSetup()`을 `CallInEditor`로 제공하면 PIE를 시작하지 않고도 Details 패널에서 누락된 연결을 검사할 수 있다.

### 6.7 UMG와 C++ Widget 부모

UMG는 Unreal의 UI 제작 시스템이다. 버튼 배치, 글자, 목록의 시각적 구성은 Widget Blueprint가 담당한다.

C++은 다음처럼 규칙과 안전 검사를 담당한다.

- 등록된 Scenario 목록 제공
- Start/Restart/Return API
- 잘못된 Scenario 시작 거부

이 분리는 UI 디자인을 Blueprint에서 빠르게 바꾸면서 중요한 실행 규칙은 C++에서 일관되게 유지하기 위한 것이다.

### Phase 3 학습 단위

Phase 3은 다음 순서로 한 단위씩 구현하고 확인한다.

1. `UScenarioDefinition` — DataAsset과 Primary Asset ID
2. `UScenarioRegistrySettings` — Project Settings와 명시적 등록
3. `UScenarioRuntimeSubsystem` — GameInstance 수명과 레벨 전환 데이터
4. `AScenarioInitializer` — World Actor와 레벨 구성 검증
5. `UScenarioSelectWidget` — UMG와 C++ 연결
6. Restart/Return/reset — 여러 객체의 수명 차이와 stale state 방지

각 단위 시작 전에 에이전트 작업, 함께 진행할 작업, 사용자 실습 후보를 구분한다.

### 사용자 실습 후보

- 에디터에서 `DA_TS_ReconSecure_001`을 직접 만들고 property 설정
- `Lvl_ScenarioMenu` 생성과 World Settings 확인
- `WBP_ScenarioSelect`의 버튼과 목록 레이아웃 제작
- Project Settings에 Scenario 등록
- 테스트 레벨에 `AScenarioInitializer` 배치 후 validation 실행
- C++ 학습을 원할 경우 작은 validation 함수 한 개를 함께 구현

어느 항목을 직접 할지는 Phase 3 시작 때 사용자와 정한다.

### Phase 3 진행 기록 — Unit 1

상태: **C++ Code Complete / Editor Integration Pending**

생성된 파일:

- `Source/Retry/Scenario/ScenarioTypes.h`
- `Source/Retry/Scenario/ScenarioDefinition.h`
- `Source/Retry/Scenario/ScenarioDefinition.cpp`

이번 단위에서 실제로 적용한 개념:

- `FScenarioLaunchOptions`는 Seed와 실행 옵션을 한 값 묶음으로 만드는 `USTRUCT`다.
- `UScenarioDefinition`은 에디터에서 Scenario 설정 asset을 만들 수 있게 하는 `UPrimaryDataAsset`이다.
- `TSoftObjectPtr<UWorld>`는 전투 Level을 즉시 로드하지 않고 경로로 가리킨다.
- `IsDefinitionValid()`는 ID, 표시 이름, Level이 비어 있는 잘못된 설정을 거부한다.
- `GetPrimaryAssetId()`는 Scenario를 `ScenarioDefinition:ScenarioId` 형태로 식별한다.

호출 흐름:

```text
사용자가 에디터에서 Scenario DataAsset 생성
→ Details 패널에서 UScenarioDefinition property 입력
→ 이후 Registry가 IsDefinitionValid 호출
→ 유효하면 GetPrimaryAssetId로 목록에 등록
```

현재는 Registry와 메뉴가 아직 없으므로 DataAsset을 실행하지는 못한다. Unit 1은 “실행할 설정표의 형식”만 만든 단계다.

검증 결과:

- Unreal Header Tool이 `USTRUCT`, `UCLASS`, `UPROPERTY`, `UFUNCTION` reflection 코드를 정상 생성했다.
- `Retry Win64 Development` 컴파일과 링크가 성공했다.
- 실행 중인 Editor에는 새 `UCLASS`가 아직 로드되지 않았으므로 Editor 재빌드와 재시작 후 DataAsset을 만든다.

## 7. Phase 4 — Command Data와 상태 전이

### 눈에 보이는 목표

화면 변화보다 “명령이 지금 어느 단계인지 코드와 로그가 정확히 안다”가 목표다.

```text
명령 제안
→ 내용 검증
→ 그룹 배정
→ 실행 시작
→ 성공 / 실패 / 취소
```

### UENUM과 USTRUCT

- `UENUM`: 미리 정한 선택지 목록. 예: Recon, Secure, Defend.
- `USTRUCT`: 서로 관련된 값을 하나로 묶는 데이터 형태. 예: Command ID, 발행자, 대상, 우선순위.

Unreal reflection에 등록하면 Blueprint 노출, 직렬화, 로그/에디터 연동 같은 엔진 기능을 사용할 수 있다.

### Validation

Validation은 데이터가 규칙에 맞는지 실행 전에 검사하는 과정이다.

예:

- `Recon + Area`는 허용
- 존재하지 않는 Group ID는 거부
- 목표가 필요한 명령에 Target ID가 없으면 거부
- 완료된 명령을 다시 실행하면 거부

실무에서는 외부 데이터, 네트워크, 에디터 설정, LLM 응답을 곧바로 믿지 않고 경계에서 검증한다.

### 상태 전이

명령 상태가 아무 방향으로나 바뀌지 않게 허용된 이동을 정한다.

```text
Proposed → Validated → Assigned → Executing → Completed
                                        ├→ Failed
                                        └→ Cancelled
```

완료된 명령이 다시 실행 중으로 돌아가면 로그와 게임 상태가 꼬일 수 있으므로 거부한다. 이런 구조를 상태 머신의 간단한 형태라고 볼 수 있다.

### GUID

GUID는 실행마다 거의 겹치지 않는 고유 ID다. “이 보고가 어느 명령에서 생겼는가?”를 연결할 때 사용한다.

Run ID, Command ID, Event ID를 연결하면 한 번의 테스트가 실패했을 때 원인을 역추적할 수 있다.

## 8. Phase 5 — ReconArea

### 눈에 보이는 목표

정찰 그룹이 목표 지역에 바로 달려가는 대신 레벨에 배치된 관측 후보를 평가하고, 한 곳으로 이동해 필요한 정보를 관측한 뒤 HQ에 보고한다.

### Mission Resolver

상위 명령은 추상적이다.

```text
“A 지역을 정찰하라”
```

기존 NPC AI는 더 구체적인 정보가 필요하다.

```text
어느 관측 지점으로 갈 것인가
교전을 얼마나 피할 것인가
어떤 정보를 얻어야 하는가
언제 완료인가
```

Mission Resolver는 추상 명령을 기존 AI가 사용할 수 있는 `FMissionContext`로 번역한다. BT Task를 직접 선택하지는 않는다.

실무에서는 상위 시스템의 표현과 실행 시스템의 표현이 다를 때 이런 adapter/resolver 계층을 둔다.

### Mission Overlay

Overlay는 기존 판단을 없애지 않고 추가 조건이나 가중치를 겹쳐 적용한다는 뜻이다.

예:

- 기존 공격 점수 계산은 유지
- Recon 임무라면 불필요한 교전 점수를 낮춤
- 적이 바로 앞에 있거나 재장전이 필요하면 긴급 판단은 유지

임무 때문에 NPC가 생존 판단을 완전히 잃지 않도록 하기 위한 구조다.

### Utility Score

여러 후보에 점수를 매기고 가장 높은 후보를 선택하는 방식이다.

관측 지점 후보는 다음 요소로 점수를 받을 수 있다.

- 목표가 잘 보이는가
- 이동 가능한가
- 거리가 가까운가
- 적에게 노출될 위험이 낮은가
- 보고할 통신 상태가 좋은가

복잡한 조건문을 계속 추가하는 대신 비교 가능한 숫자로 선택한다. 가중치가 결과에 큰 영향을 주므로 테스트와 디버그 출력이 중요하다.

### Operational Fact와 Report

NPC가 적을 봤다고 HQ가 즉시 아는 것은 아니다.

```text
NPC가 관측
→ Local Fact 생성
→ Report 생성
→ Report 수신
→ Team Operational Memory 갱신
```

이 구분이 있어야 통신 실패, 오래된 정보, 불완전한 정보를 나중에 표현할 수 있다.

## 9. Phase 6 — SecureArea

### 눈에 보이는 목표

Recon 결과를 받은 뒤 Combat Group A와 B가 서로 다른 Route 또는 Target을 받고 목표 지역에 진입한다.

### Hard Constraint와 Soft Constraint

- Hard Constraint: 반드시 지켜야 하는 규칙. 예: 작전 구역 밖 장거리 추격 금지.
- Soft Constraint: 가능하면 지키되 상황에 따라 양보할 수 있는 선호. 예: 피해 최소화.

Hard Constraint를 단순 점수 감점으로만 만들면 다른 점수가 높을 때 금지 행동이 선택될 수 있다. 따라서 불가능 판정이나 명시적 gate가 필요하다.

실무에서도 “선호”와 “절대 금지”를 같은 가중치 체계에 섞지 않는 것이 중요하다.

### 완료 판정

BT Task 하나가 Success를 반환했다고 “지역 확보 완료”가 되는 것은 아니다.

SecureArea 완료에는 다음 같은 여러 조건이 필요하다.

- 지정 그룹이 목표 지역 도달
- 주요 위협이 제거·후퇴·무력화
- 일정 시간 아군 점유 유지

그래서 그룹 단위 `CommandExecutionMonitor`가 World 상태를 관찰한다.

## 10. Phase 7 — LLM 연결

### 왜 마지막인가

LLM을 먼저 연결하면 문제가 명령 시스템인지, AI 실행인지, LLM 응답인지 구분하기 어렵다.

Phase 5와 6을 하드코딩 명령으로 성공시킨 뒤 LLM은 명령 내용을 제안하는 역할만 맡는다.

### JSON Schema와 Validation

LLM 출력은 문자열이므로 잘못된 필드, 존재하지 않는 ID, 허용되지 않은 조합이 나올 수 있다.

```text
LLM JSON 응답
→ 형식 parse
→ schema 검사
→ 게임 규칙 validation
→ semantic Target ID를 실제 Actor로 조회
→ 유효할 때만 적용
```

LLM이 생성한 좌표나 Actor 주소를 직접 사용하지 않는다. 게임이 알고 있는 ID를 통해 실제 World 대상을 찾는다.

### 비동기 요청과 늦은 응답

HTTP 요청을 보낸 뒤 레벨이 재시작될 수 있다. 이전 Run의 응답이 늦게 도착해 새 Run에 적용되면 안 된다.

그래서 요청이 어느 Run에서 시작됐는지 확인하고, 레벨 전환 때 queue/timer를 취소하며, 완료된 요청이 두 번 적용되지 않게 막는다.

## 11. 현재 확정된 선택을 이해하기

### 레벨 고정 배치를 먼저 사용하는 이유

초기에는 NPC Spawn 시스템까지 만들지 않고 테스트 레벨에 NPC와 그룹을 배치한다.

장점:

- 에디터에서 위치와 연결을 눈으로 확인 가능
- 기존 `Lvl_ThirdPerson` 배치 방식을 재사용
- 기술 스파이크의 위험을 Command/Report 흐름에 집중

단점:

- Scenario마다 별도 레벨 구성이 필요할 수 있음
- DataAsset만 바꿔 완전히 다른 병력 구성을 만들 수는 없음

나중에 반복 배치 비용이 실제 문제가 될 때 Spawn Definition으로 확장할 수 있다.

### 별도 Scenario 메뉴 레벨을 사용하는 이유

전투 레벨과 메뉴 책임을 분리한다. 기존 전투 레벨 위에 메뉴를 겹치면 NPC가 이미 시작되거나 입력과 카메라가 충돌할 수 있다.

단점은 메뉴용 Level, GameMode, Controller 설정이 추가된다는 점이다.

### Project Settings 등록을 사용하는 이유

Custom GameInstance Blueprint를 하나 더 만들어 registry asset을 연결하는 대신 프로젝트 설정에 목록을 둔다. 설정 위치가 명확하고 GameInstance BP 연결 누락을 줄일 수 있다.

단점은 새 Scenario 추가 때 Project Settings 등록 단계를 기억해야 한다는 점이다.

### C++ 판단과 최소 Blackboard 데이터를 사용하는 이유

Blackboard는 BT가 실행에 필요한 현재 값만 공유하고, 복잡한 판단과 권위 상태는 C++ Component에 둔다.

장점:

- 판단 로직을 C++ 테스트로 검증하기 쉬움
- Blackboard key가 무분별하게 늘어나는 것을 방지
- 기존 프로젝트 방향과 일치

단점:

- 에디터에서 모든 판단 데이터를 한눈에 보기는 어려움
- 디버그 UI와 로그를 별도로 잘 만들어야 함

## 12. 구현하면서 반복할 학습 흐름

각 학습 단위는 다음 순서로 진행한다.

1. 이번에 게임에서 달라질 한 가지를 확인한다.
2. 새 Unreal/C++ 개념을 현재 프로젝트 예시로 설명한다.
3. 선택할 설계가 있으면 대안과 tradeoff를 설명한다.
4. 에이전트 작업, 함께 할 작업, 사용자 실습 후보를 나눈다.
5. 작은 범위만 구현한다.
6. 빌드 또는 에디터에서 결과를 확인한다.
7. 실제 호출 흐름을 코드 파일과 함께 다시 설명한다.
8. 이 문서에 배운 내용과 남은 질문을 갱신한다.

## 13. 지금 외우지 않아도 되는 것

- 모든 Unreal reflection macro의 세부 옵션
- Asset Manager의 전체 동작
- Subsystem 종류 전체
- Behavior Tree node lifecycle의 모든 callback
- HTTP 비동기 구현 세부사항
- LLM JSON schema의 최종 형태

필요한 Phase에서 실제 코드와 함께 배운다. 지금은 각 객체가 왜 존재하고 언제 사라지는지만 이해하면 충분하다.

## 14. 다음 단계 전에 확인할 것

Phase 3 첫 단위는 `UScenarioDefinition`이다. 시작하기 전에 다음을 함께 정한다.

- DataAsset C++ 뼈대를 에이전트가 먼저 만들고 사용자가 에디터에서 asset을 만들지
- C++ 클래스 일부를 사용자와 함께 작성할지
- 설명 깊이를 Unreal 입문, C++ 입문, 또는 둘 다에 맞출지

이 선택도 단순 옵션만 제시하지 않고 첫 단위의 코드 크기와 실제 결과를 먼저 설명한 뒤 질문한다.
