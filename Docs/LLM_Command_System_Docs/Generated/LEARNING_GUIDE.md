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
- 옵션을 읽고 후속 Command 시스템이 자동 시작할 준비

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

### 선행 안전 단위 — 비동기 HTTP 요청의 수명

HTTP 요청은 결과가 즉시 돌아오지 않는 비동기 작업이다. 요청을 보낸 PIE World가 먼저 사라져도 네트워크 작업은 잠시 뒤 성공, 실패 또는 timeout callback을 전달할 수 있다. 그래서 callback 안에서 `GetWorld()`가 항상 유효하다고 가정하면 안 된다.

현재 프로젝트에서는 `ULLMRequestQueue`가 요청의 전체 수명주기를 소유한다.

```text
NPC/Group이 요청을 queue에 추가
→ queue가 ActiveRequest 하나를 시작
→ HTTP 자체 timeout 또는 응답 callback
→ 현재 generation과 active request가 맞을 때만 결과 적용
→ 완료 후 다음 pending request 시작

World cleanup / Restart / Return
→ generation 증가
→ callback delegate 해제
→ ActiveRequest 취소
→ pending request 폐기
```

여기서 generation은 “어느 실행에서 시작된 요청인가”를 구분하는 번호다. 이전 실행의 응답이 늦게 와도 번호가 다르면 무시할 수 있다. `TWeakObjectPtr`는 callback이 실행될 때 queue 자체가 아직 살아 있는지 확인하는 약한 참조다.

실무에서도 서버 요청, 비동기 asset load, 타이머처럼 나중에 끝나는 작업은 소유 객체의 `EndPlay`, subsystem `Deinitialize`, level transition과 함께 취소하거나 결과를 무효화한다. 단순히 대상 Actor를 weak pointer로 저장하는 것만으로는 callback 내부의 World 접근과 중복 완료를 막을 수 없다.

관련 코드:

- `Source/Retry/LLMRequestQueue.h`: active request와 generation 소유
- `Source/Retry/LLMRequestQueue.cpp`: world cleanup/reset, HTTP timeout, late callback guard

사용자 실습 후보는 로컬 LLM 서버를 끈 상태에서 요청 직후 PIE를 중단해 editor가 유지되는지 확인하고, 로그에서 `전환 정리`가 한 번 기록되는지 보는 것이다.

### 사용자 실습 후보

- 에디터에서 `DA_TS_ReconSecure_001`을 직접 만들고 property 설정
- `Lvl_ScenarioMenu` 생성과 World Settings 확인
- `WBP_ScenarioSelect`의 버튼과 목록 레이아웃 제작
- Project Settings에 Scenario 등록
- 테스트 레벨에 `AScenarioInitializer` 배치 후 validation 실행
- C++ 학습을 원할 경우 작은 validation 함수 한 개를 함께 구현

어느 항목을 직접 할지는 Phase 3 시작 때 사용자와 정한다.

### Phase 3 진행 기록 — Unit 1

상태: **Integrated Complete**

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

사용자가 `DA_TS_ReconSecure_001`을 생성해 필수 property와 전용 Level을 연결했고, Level이 정상적으로 열리고 동작하는 것을 확인했다. 이어서 요청 중 PIE를 종료하는 LLM 수명주기 회귀 검증에서 활성 요청 취소와 editor 유지를 확인했다.

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
- 닫힌 Editor 상태에서 `RetryEditor Win64 Development` 빌드가 성공했다. 다시 연 Editor에서 새 `UCLASS`를 사용해 DataAsset을 만든다.

첫 기능 검증 Level은 기존 `Lvl_ThirdPerson`을 `Lvl_TS_ReconSecure_001`로 복제한다. 이 Level은 Scenario 로더와 수직 슬라이스를 좁은 환경에서 검증하는 용도다. 넓은 거리와 복수 Route가 필요한 규모 검증은 Phase 5~6에서 `Lvl_TS_ReconSecure_Large_001` 같은 별도 Level로 분리하여, 시스템 오류와 맵 규모 문제를 구분한다.

### Phase 3 진행 기록 — Unit 2

상태: **Integrated Complete**

학습 목표는 Project Settings가 실행 가능한 Scenario 카탈로그를 소유하게 만드는 것이다.

생성·수정된 파일:

- `Source/Retry/Scenario/ScenarioRegistrySettings.h/.cpp`
- `Source/Retry/Tests/ScenarioRuntimeTests.cpp`
- `Source/Retry/Retry.Build.cs`

`UScenarioRegistrySettings`는 `UDeveloperSettings`를 상속한다. 레벨에 배치하거나 직접 Spawn하지 않으며 Unreal 설정 시스템이 기본 객체를 만들고 `DefaultGame.ini`의 값을 읽는다. 다음 Unit의 `UScenarioRuntimeSubsystem`은 `GetDefault<UScenarioRegistrySettings>()`로 이 목록을 조회한다.

```text
Project Settings > Game > Scenario Registry
→ RegisteredScenarios에 Soft Reference 저장
→ IsRegistryValid가 각 Definition 로드 및 필수값 검사
→ 중복 ScenarioId 검사
→ 다음 Unit의 RuntimeSubsystem만 유효한 목록을 메뉴에 제공
```

폴더 자동 검색을 선택하지 않은 이유는 테스트용·미완성 DataAsset까지 메뉴에 노출될 수 있고, 에디터와 패키징 결과의 목록이 달라질 가능성이 있기 때문이다. 명시적 등록은 Scenario를 추가할 때 한 번 더 설정해야 하지만, 어떤 항목이 제품에 포함되는지가 분명하다.

검증은 첫 오류에서 멈추지 않고 빈 참조, 로드 실패, Definition 필수값 누락, 중복 ID를 모두 수집한다. 자동화 테스트는 빈 참조/Level 누락과 중복 ID를 별도로 재현한다.

사용자가 빌드와 에디터 재시작을 수행한 뒤 Project Settings의 `Registered Scenarios`에 `/Game/Scenarios/Definitions/DA_TS_ReconSecure_001` 하나를 등록했다. `DefaultGame.ini`의 `/Script/Retry.ScenarioRegistrySettings` 섹션에 같은 Soft Reference가 저장됐고 Registry 자동화 테스트 두 개도 통과했다.

### Phase 3 진행 기록 — Unit 3

상태: **C++ + Automation Verified / OpenLevel Integration Deferred**

학습 목표는 Level 전환을 넘어 선택값을 전달하는 GameInstance 수명을 이해하는 것이다.

생성·수정된 파일:

- `Source/Retry/Scenario/ScenarioTypes.h/.cpp`
- `Source/Retry/Scenario/ScenarioRuntimeSubsystem.h/.cpp`
- `Source/Retry/Tests/ScenarioRuntimeTests.cpp`

`UScenarioRuntimeSubsystem`은 Unreal이 GameInstance마다 자동으로 만들고 GameInstance 종료 때 제거한다. 메뉴 Widget은 이 객체를 호출하지만 소유하지 않는다. 전투 Level의 `AScenarioInitializer`도 다음 Unit에서 현재 Run Context를 읽을 뿐 소유하지 않는다.

```text
메뉴 호출자
→ Registry 전체 검증
→ ScenarioId로 등록 Definition 검색
→ 새 Run GUID와 Launch Options를 FScenarioRunContext에 저장
→ LLM queue 전환 정리
→ Soft Level로 OpenLevel
→ 새 World에서도 같은 RuntimeSubsystem의 Context 조회
```

Restart는 현재 Scenario ID와 옵션을 복사한 뒤 같은 Start 경로를 재사용한다. 따라서 Seed와 옵션은 유지하지만 Run ID는 새로 생성된다. Return은 계획에서 확정한 `/Game/Scenarios/Maps/Lvl_ScenarioMenu`가 실제로 존재할 때만 Context를 지우고 이동한다. 메뉴가 없으면 현재 상태를 보존한 채 실패한다.

Run Context 생성과 `OpenLevel`을 분리한 이유는 자동화 테스트가 에디터 World를 임의로 전환하지 않고도 ID, 옵션, Level, GUID 보존을 확인하게 하기 위해서다. 실제 Level 전환은 후속 Widget 연결 뒤 PIE에서 검증한다.

실무에서는 로그인 세션, 선택한 게임 모드, 매치 참가 정보처럼 맵 이동을 넘어야 하는 작은 상태를 GameInstance 수명에 둔다. 반대로 적 위치나 현재 임무 진행처럼 새 World에서 초기화해야 하는 데이터까지 넣으면 stale state가 되므로 World/Actor 수명으로 분리한다.

사용자가 코드를 반영한 뒤 `Automation RunTests Retry.Scenario.Runtime`을 실행했고, `PreservesLaunchData`와 `RejectsInvalidDefinition` 두 테스트가 모두 통과했다. 실제 `StartScenario → OpenLevel`은 메뉴 Widget 연결 단계에서 확인한다.

### Phase 3 진행 기록 — Unit 4

상태: **Integrated Complete**

학습 목표는 Level에 배치되는 World Actor가 DataAsset 및 GameInstance 상태와 어떤 경계에서 만나는지 이해하는 것이다.

생성·수정된 파일:

- `Source/Retry/Scenario/ScenarioInitializer.h/.cpp`
- `Source/Retry/Scenario/ScenarioTypes.h`
- `Source/Retry/AI/GroupManagerActor.h/.cpp`
- `Source/Retry/Components/MemoryComponent.h/.cpp`

`AScenarioInitializer`는 레벨에 하나 배치되는 Actor다. Actor를 새로 Spawn하지 않고, 저장된 Group/NPC 배치가 실행 가능한지 검사한다. `ValidateScenarioSetup()`은 PIE 없이 Details 패널에서 실행하는 `CallInEditor` 함수다.

```text
에디터 검증
→ Definition과 현재 Level 일치 확인
→ Group ID / NPC Name 중복 확인
→ NPC의 Group 참조와 Team ID 확인
→ Group마다 NPC와 Leader가 정확히 구성됐는지 확인

정상적인 Scenario 진입
→ GameInstance의 활성 Run Context 확인
→ Scenario/Level이 Definition과 일치하는지 확인
→ Seed 적용
→ LLM queue, 그룹 기억·명령, NPC 기억 초기화
```

레벨을 Content Browser에서 직접 열어 PIE하면 활성 Run Context가 없다. 이 경우 기존 테스트 방식과 호환되도록 경고만 기록하고 초기화를 건너뛴다. 반대로 메뉴의 `StartScenario()`를 통해 들어온 실행에서는 Context 불일치가 설정 오류이므로 초기화를 거부한다.

`ResetGroupRuntimeState()`가 `Leader`와 `Members`를 비우지 않는 이유도 Actor 수명과 시작 순서 때문이다. NPC가 그룹에 등록되는 `BeginPlay`와 Initializer의 `BeginPlay` 순서는 의존하면 안 된다. 따라서 저장된 배치 관계는 보존하고, 실행 중 생긴 기억·감정 누적치·명령만 지운다.

현재 Seed는 기존 코드가 사용하는 Unreal 전역 난수 스트림에 적용한다. 이후 개별 시스템에서 완전한 재현성이 필요하면 각 시스템이 `FRandomStream`을 소유하도록 발전시키는 편이 더 안전하다.

사용자 실습은 `Lvl_TS_ReconSecure_001`에 Initializer를 배치하고 Definition을 연결한 뒤, 두 Group Manager의 Team ID를 소속 NPC와 맞추고 `Validate Scenario Setup`을 실행하는 것이다.

사용자가 Team ID 불일치 시 `Invalid Actor Configuration`을 확인했고, Group A/B를 각각 소속 NPC와 같은 Team ID 1/2로 복원한 뒤 성공 검증까지 완료했다.

레벨 회귀 테스트에서 사망 Capsule 대신 Ragdoll Mesh가 바닥을 통과하는 문제가 발견됐다. 투사체를 `WorldDynamic`으로 구분하고 시체가 해당 채널 전체를 무시한 것이 원인이었다. 사용자가 Project Settings에서 `Projectile` 전용 object channel을 만들고 C++이 이를 사용하게 하여, Ragdoll은 `WorldStatic/WorldDynamic` 바닥을 막고 투사체만 무시하도록 수정한다. 이는 object channel이 “그 물체가 무엇인가”, response가 “그 종류와 어떻게 상호작용하는가”를 구분하는 사례다.

사용자가 수정 후 Ragdoll 바닥 충돌과 시체를 통과하는 투사체를 모두 확인했다.

### Phase 3 진행 기록 — Unit 5

상태: **Integrated Complete**

학습 목표는 C++ Widget 부모와 Widget Blueprint의 역할을 분리하고, 메뉴 World에서 선택한 값을 GameInstanceSubsystem을 통해 전투 World로 전달하는 것이다.

생성된 파일:

- `Source/Retry/UI/ScenarioSelectWidget.h/.cpp`

`UScenarioSelectWidget`은 화면 모양을 만들지 않는다. Registry 목록, 현재 선택과 실행 옵션을 보관하고 등록 목록 밖의 선택을 거부하며 `UScenarioRuntimeSubsystem::StartScenario()`를 호출한다. Blueprint는 다음 세 event를 구현한다.

- `Refresh Scenario List`: Entry Widget을 동적으로 생성한다.
- `Scenario Selection Changed`: 상세 이름·설명·옵션을 갱신한다.
- `Scenario Start Failed`: 사용자에게 실패 메시지를 보여준다.

```text
BP_MenuPlayerController가 WBP_ScenarioSelect 생성
→ NativeConstruct가 Registry 목록 조회
→ Blueprint가 WBP_ScenarioEntry를 항목 수만큼 생성
→ Entry 버튼이 SelectScenario 호출
→ 상세 패널에서 Seed/옵션 편집
→ Start 버튼이 옵션 저장 후 StartSelectedScenario 호출
→ RuntimeSubsystem이 Context 생성 후 OpenLevel
```

Level Blueprint에서 Widget을 생성하지 않는 이유는 메뉴 화면의 입력과 UI 소유권이 PlayerController 책임이기 때문이다. 레벨 교체나 메뉴 화면 수정 시 Level Blueprint에 로직이 흩어지는 것을 피할 수 있다.

동적 목록은 현재 Scenario가 하나일 때 작업량이 조금 더 크지만 Registry에 항목을 추가하면 메뉴 코드를 다시 만들지 않아도 되는 실무적인 구조다.

사용자가 메뉴 Level, 전용 PlayerController/GameMode, 동적 Entry와 상세 Widget을 구성했다. 메뉴에서 지정한 Seed와 Scenario ID가 전투 Level까지 전달됐고, `AScenarioInitializer`의 성공 로그와 기존 레벨 동작을 확인했다. Project Settings의 Game Default Map은 메뉴로 변경하고 Editor Startup Map은 유지했다.

### Phase 3 진행 기록 — Unit 6

상태: **Integrated Complete**

학습 목표는 같은 GameInstance 안에서 World와 PlayerController가 교체될 때 어떤 상태를 유지하고 어떤 상태를 폐기해야 하는지 확인하는 것이다.

생성·수정된 파일:

- `Source/Retry/UI/ScenarioDebugWidget.h/.cpp`
- `Source/Retry/RetryPlayerController.h/.cpp`
- `Source/Retry/Scenario/ScenarioRuntimeSubsystem.cpp`

`UScenarioDebugWidget`은 전투 Level에서 현재 Run ID, Scenario ID, Seed를 표시하고 Restart/Return을 RuntimeSubsystem에 요청한다. 화면 배치는 Widget Blueprint가 담당한다. 기존 `ARetryPlayerController`는 Widget을 생성해 숨겨두고 지정 Enhanced Input Action이 들어오면 F12 패널을 토글한다.

```text
Restart
→ 현재 Scenario ID와 Launch Options 복사
→ 새 Run ID 생성
→ LLM request 취소/queue reset
→ 같은 전투 Level OpenLevel
→ 이전 World/Controller/Widget/NPC 폐기
→ 새 World의 Initializer가 새 Context 적용

Return
→ LLM request 취소/queue reset
→ Run Context inactive
→ 메뉴 Level OpenLevel
→ 메뉴 전용 PlayerController와 선택 Widget 생성
```

여기서 stale state는 이미 끝난 World나 Run의 데이터가 다음 실행에 남는 상태다. Run ID가 바뀌고 NPC/Group memory가 새 World와 함께 초기화되며 이전 HTTP callback이 무시되는지를 함께 확인한다.

F12 패널을 기존 HUD에 항상 붙이지 않고 독립 Widget으로 둔 이유는 개발용 기능을 실제 플레이 UI와 분리하고, Phase 4 이후 Command ID·상태·실패 원인을 같은 패널에 확장하기 위해서다. F9는 Editor screenshot, F10/F11은 Level/Blueprint Editor 명령과 충돌하므로 현재 UE 5.8 기본 바인딩이 없는 F12를 사용한다.

사용자가 F12로 Input Mapping을 변경하고 패널 토글, Restart, Return to Menu가 모두 정상 동작하는 것을 확인했다. Phase 3 완료 전에는 활성 LLM 요청이 있는 Restart/Return에서도 이전 Run callback이 무효화되는지 마지막으로 확인한다.

이후 활성 LLM 요청 중 Restart와 Return도 모두 통과했다. queue가 active request를 취소했고 editor crash나 이전 Run의 늦은 결과 적용이 없었다. Phase 3 기능은 통합 완료이며, 사용자 주도 package 검증만 별도 최종 게이트로 남긴다.

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

모든 비종료 상태에서는 `Cancelled`로 이동할 수 있게 결정했다. HQ가 아직 검증 중인 명령을 철회하거나, 그룹 배정 뒤 상황이 바뀌거나, 실행 중 상위 명령이 취소되는 경우를 같은 규칙으로 표현한다. 반대로 `Completed`, `Failed`, `Cancelled`는 terminal 상태라 다시 전이하지 않는다.

### GUID

GUID는 실행마다 거의 겹치지 않는 고유 ID다. “이 보고가 어느 명령에서 생겼는가?”를 연결할 때 사용한다.

Run ID, Command ID, Event ID를 연결하면 한 번의 테스트가 실패했을 때 원인을 역추적할 수 있다.

### Phase 4 진행 기록 — Unit 1

상태: **Integrated Complete**

학습 목표는 “작전 목표”, “NPC의 순간 전투 성향”, “LLM 전송 요청”을 서로 다른 데이터 계약으로 구분하고 상태 전이를 순수 함수로 검증하는 것이다.

생성된 파일:

- `Source/Retry/AI/CommandTypes.h/.cpp`
- `Source/Retry/Tests/CommandValidationTests.cpp`

`FCommandIntent`는 Actor가 아니라 값 데이터다. Command ID, 발행자, 담당 Group, Verb, Target, Priority, Constraint, 정보 요구사항, 완료 조건, 현재 상태를 함께 보관한다. `FMissionContext`는 이후 Resolver가 유효한 Command를 NPC가 실행할 구체 데이터로 번역한 결과다.

```text
FCommandIntent: 무엇을 달성할 것인가
→ Phase 5 MissionResolver
→ FMissionContext: 어디로 가고 무엇을 제한할 것인가
→ 기존 ENPCOrder/utility: 지금 전투에서 어떻게 행동할 것인가
```

Constraint와 Information Requirement는 아직 특정 Actor나 Blackboard key를 참조하지 않는다. `FName` semantic ID를 사용해 Phase 5의 marker registry가 실제 Actor/Location으로 해석하게 한다. LLM이 임의 좌표나 UObject pointer를 직접 만드는 것도 방지한다.

상태 전이 함수는 World나 Actor 없이 실행되는 순수 규칙이다. 그래서 Level을 열지 않고 Automation Test로 정상 경로, 취소 경로, 잘못된 역전이와 terminal 재전이를 빠르게 확인할 수 있다.

사용자가 `Retry.Command.Status` 자동화 테스트 3개를 실행했고 모두 통과했다.

### Phase 4 진행 기록 — Unit 2

상태: **Integrated Complete**

학습 목표는 외부 데이터가 실행 계층에 들어오기 전에 모든 문제를 구조화된 결과로 수집하는 validation boundary를 이해하는 것이다.

생성·수정된 파일:

- `Source/Retry/AI/CommandValidation.h/.cpp`
- `Source/Retry/Tests/CommandValidationTests.cpp`

`FCommandValidator::Validate()`는 입력 Command를 수정하지 않는다. 유효한지 판단하고 `FCommandValidationIssue` 배열만 반환한다. 호출자는 결과가 성공했을 때만 상태를 `Validated`로 전이한다.

```text
외부/하드코딩 Command
→ Validate: 내용 검사, 상태 변화 없음
├→ issue 존재: Proposed 유지 + 모든 오류 반환
└→ issue 없음: 호출자가 Proposed → Validated 수행
```

첫 오류에서 멈추지 않는 이유는 에디터 설정이나 향후 LLM 응답의 여러 문제를 한 번에 고칠 수 있게 하기 위해서다. 각 issue는 enum 오류 코드와 사람이 읽는 메시지를 함께 가진다. 게임 로직은 문자열 비교 대신 안정적인 오류 코드를 사용한다.

Position `(0,0,0)`은 실제 좌표일 수 있으므로 거부하지 않는다. NaN과 무한대만 잘못된 좌표로 처리한다. 반면 Area/Route/Unit/Information은 marker나 registry가 해석할 semantic Target ID가 반드시 필요하다.

Completion의 Timeout `0`은 제한 없음이다. 양수 Timeout보다 Minimum Hold가 길면 완료 전에 항상 timeout되므로 모순으로 거부한다.

사용자가 `Retry.Command.Validation` 자동화 테스트 4개를 실행했고 모두 통과했다.

### Phase 4 진행 기록 — Unit 3

상태: **Integrated Complete**

학습 목표는 Output Log의 문장과, 게임이 다시 조회하고 연결할 수 있는 구조화 실행 기록의 차이를 이해하는 것이다.

생성·수정된 파일:

- `Source/Retry/Scenario/ScenarioExecutionLogSubsystem.h/.cpp`
- `Source/Retry/Scenario/ScenarioRuntimeSubsystem.h/.cpp`
- `Source/Retry/Tests/ScenarioExecutionLogTests.cpp`

`UScenarioExecutionLogSubsystem`은 `UGameInstanceSubsystem`이다. GameInstance는 레벨을 다시 열어도 유지되므로, Restart로 이전 World가 사라진 뒤에도 방금 끝난 Run의 기록을 보존할 수 있다. 반면 실제 NPC와 Group 상태는 World에 속하므로 새 레벨에서 초기화된다.

```text
GameInstance 세션
├─ 완료 Run A: RunStarted → Command events → RunEnded(Restarted)
└─ 활성 Run B: RunStarted → ...
```

Run ID는 한 번의 Scenario 실행, Command ID는 그 안의 명령, Event ID는 한 번의 기록을 식별한다. 이벤트에는 Run 내부 순번도 둔다. UTC 시각이 같은 프레임에 겹쳐도 순번으로 정확한 기록 순서를 알 수 있기 때문이다.

기록 API는 전달받은 Run ID가 현재 활성 Run과 같은지 검사한다. 이것을 stale write guard라고 한다. 이미 끝난 HTTP callback이나 이전 World의 객체가 늦게 결과를 보내도 새 Run의 로그에 섞이지 않는다.

상태 전이는 일반 이벤트 API로 기록할 수 없다. 전용 API가 `이전 상태 → 새 상태`의 유효성을 검사한 뒤 두 상태를 함께 남긴다. 따라서 로그와 실제 상태 머신 규칙이 서로 다른 이야기를 하는 것을 막는다.

이번 단위는 세션 메모리 안의 기록과 조회 경계까지만 만든다. JSON 파일 export, 디스크 저장 위치, 최대 보존 Run 수는 실제 디버그 UI나 저장 요구가 정해질 때 결정한다. 지금 미리 정하면 사용하지 않는 정책과 코드만 늘어나기 때문이다.

검증은 Level이나 Asset 없이 `Retry.Scenario.ExecutionLog` 자동화 테스트 3개로 수행한다. ID 연결, 이전 Run의 늦은 기록 거부, Restart 시 완료 Run 보존을 각각 확인한다.

첫 사용자 실행에서는 테스트가 `UGameInstanceSubsystem`을 transient package 아래에 직접 생성해 `ClassWithin GameInstance` ensure가 발생했다. `Outer`는 UObject의 소유·수명 계층이다. 실제 게임에서는 엔진이 Subsystem을 GameInstance 아래에 생성하지만, 자동화 fixture도 같은 규칙을 지켜야 한다. 테스트는 임시 `UGameInstance`를 먼저 만들고 이를 Outer로 지정하도록 수정했다. 기능 코드의 결함은 아니지만 ensure가 발생한 테스트 결과는 성공 표시 여부와 관계없이 유효한 통과로 간주하지 않는다.

수정 반영 후 사용자가 `Retry.Scenario.ExecutionLog` 자동화 테스트 3개를 다시 실행했고, ensure 없이 모두 통과했다.

### Phase 4 진행 기록 — Unit 4

상태: **Integrated Complete**

학습 목표는 “현재 명령의 진짜 값은 누가 소유하는가”와 “이력 기록은 누가 소유하는가”를 분리하는 것이다.

생성·수정된 파일:

- `Source/Retry/AI/GroupManagerActor.h/.cpp`
- `Source/Retry/Scenario/ScenarioExecutionLogSubsystem.h/.cpp`
- `Source/Retry/Tests/GroupCommandAuthorityTests.cpp`

`AGroupManagerActor`는 실제 NPC 구성원과 함께 World에 존재한다. 따라서 현재 그룹이 수행할 Command의 권위 상태를 소유한다. `UScenarioExecutionLogSubsystem`은 GameInstance에 남아 그 상태가 어떻게 변했는지 기록하지만, 로그 자체가 현재 게임 상태를 결정하지는 않는다.

```text
AGroupManagerActor.CurrentCommand
→ 현재 게임이 믿는 권위 상태

UScenarioExecutionLogSubsystem.Events
→ 그 상태가 변해 온 감사·디버그 이력
```

이 구분이 없으면 로그에는 Assigned라고 남았는데 GroupManager는 다른 Command를 실행하거나, 이전 Command가 끝나지 않았는데 새 Command가 덮어쓰는 문제가 생긴다.

`AssignCommand()`은 단순 setter가 아니라 aggregate boundary다. 여기서 aggregate는 관련 상태와 규칙을 한 객체가 책임지는 묶음이라는 뜻이다. 현재 프로젝트에서는 GroupManager가 다음 규칙을 함께 지킨다.

1. 활성 Scenario Run과 그 Run의 실행 로그가 있어야 한다.
2. GroupManager의 Group ID가 유효해야 한다.
3. 비종료 CurrentCommand가 없어야 한다.
4. `FCommandValidator`의 모든 규칙을 통과해야 한다.
5. Command의 Assigned Group ID가 자신의 Group ID와 같아야 한다.
6. 성공한 Command만 `Proposed → Validated → Assigned`로 전이하고 소유한다.

결과를 `bool` 하나로만 반환하지 않고 `ECommandAssignmentOutcome`과 `FCommandAssignmentResult`로 반환한다. `GroupMismatch`, `ValidationRejected`, `ActiveCommandExists`처럼 호출자가 실패 종류를 안정적으로 구분하고, validation의 전체 issue도 그대로 확인할 수 있다.

Clear와 Cancel은 다르다. Cancel은 상태 머신을 통해 `Cancelled`로 이동하며 로그가 남는다. Clear는 terminal Command를 GroupManager의 현재 슬롯에서 제거하는 보관 정리다. 비종료 Command를 바로 Clear하지 못하게 하여 상태 전이와 로그를 우회하지 못하게 한다.

기존 `SetOrderForAll(ENPCOrder, Weight)`은 수정하지 않았다. `FCommandIntent`는 상위 작전 목표이고, `ENPCOrder`는 현재 NPC 전투 성향이므로 Phase 5의 MissionResolver가 연결되기 전까지 두 계층은 독립적으로 유지한다.

자동화 테스트는 임시 Editor Preview World에서 `AGroupManagerActor`를 `SpawnActor`로 생성한다. UObject 소유 규칙을 지키면서도 사용자의 현재 레벨, 선택, Asset을 변경하지 않는다. World는 오디오·물리·Navigation·AI 없이 생성되고 테스트 종료 시 World Context와 함께 폐기된다.

사용자가 `Retry.Command.GroupAuthority` 자동화 테스트 3개를 실행했고 모두 통과했다. 이로써 Phase 4의 Command 상태 머신, validation, 실행 로그, GroupManager 권위 수명이 연결된 상태로 검증됐다.

## 8. Phase 5 — ReconArea

### Phase 5 진행 기록 — Unit 0 Preflight

상태: **Complete / Marker Architecture Decided**

학습 목표는 기존 전투 AI에서 재사용할 안전한 부분과 새 Mission 전용 경계가 필요한 부분을 구분하는 것이다.

현재 Blackboard의 11개 key는 전투 상태와 전투 실행 데이터를 전달한다. 특히 `TargetActor`는 인지한 적, `LastKnownEnemyLocation`은 마지막 적 위치, `CoverLocation`은 엄폐 위치다. 이 값을 Recon 목적지로 재사용하면 적을 관측하러 가는 것과 적을 공격하러 가는 의미가 섞인다. 따라서 Mission은 별도 key를 사용해야 한다.

현재 이동 Task도 lifecycle이 서로 다르다.

- `BTTask_MoveToTarget`: 이동 요청 직후 성공하므로 도착 여부를 뜻하지 않는다.
- `BTTask_MoveToCover`: 이동 중 `InProgress`지만 완료 callback이 없어 끝나지 않을 수 있다.
- native `Move To`: Blackboard key를 대상으로 이동 완료·실패를 BT가 관리한다.

첫 Recon Mission branch에는 native `Move To`가 가장 작은 안전한 선택이다. 커스텀 이동 Task의 일반적인 lifecycle 개선은 별도 회귀 범위로 남긴다.

Marker는 레벨 디자이너가 “이곳은 작전 목표 지역”, “이곳은 관측 후보”라고 의미를 부여해 배치하는 Actor다. Command는 `ObjectiveA` 같은 semantic ID만 가지며 MissionResolver가 같은 ID의 Marker를 찾아 실제 Actor와 Location으로 변환한다. 첫 수직 슬라이스에서는 Objective Area와 Observation Point만 필요하고 Route는 아직 소비되지 않으므로 만들지 않는다.

사용자는 공통 기반 Actor와 전문 Objective/Observation Actor를 사용하는 1번 구조를 선택했다. 이 구조는 미래 도시 거점 같은 영구 Landmark를 배치 또는 Spawn된 Actor로 표현할 수 있다. 반면 EQS/NavMesh가 계산하는 다수의 임시 관측 후보는 가벼운 값 데이터로 평가하고, Selector가 배치 Actor 후보와 합치는 방식으로 확장한다. 따라서 “확장 가능하다”는 것이 모든 후보를 Actor로 만들어야 한다는 뜻은 아니다.

### Phase 5 진행 기록 — Unit 1 Marker Foundation

상태: **Integrated Complete**

학습 목표는 Unreal의 Actor 상속으로 공통 장소 identity와 장소별 전문 데이터를 분리하는 것이다.

생성·수정된 파일:

- `Source/Retry/AI/ScenarioMarkerTypes.h`
- `Source/Retry/AI/ScenarioMarkerActor.h/.cpp`
- `Source/Retry/AI/ObjectiveAreaActor.h/.cpp`
- `Source/Retry/AI/ObservationPointActor.h/.cpp`
- `Source/Retry/Scenario/ScenarioInitializer.cpp`
- `Source/Retry/Tests/ScenarioMarkerTests.cpp`

`AScenarioMarkerActor`는 직접 배치하지 않는 abstract 기반 Actor다. 모든 Marker가 공유하는 `MarkerId`와 전체 set 검증을 제공한다. `AObjectiveAreaActor`와 `AObservationPointActor`만 Place Actors에 나타나 실제 역할별 property를 가진다.

Objective의 Sphere와 Observation의 Arrow는 에디터에서 범위와 방향을 이해하기 위한 Component다. 충돌과 Navigation에는 참여하지 않고 게임에서는 숨겨진다. 이 Actor들은 전투 대상이 아니라 레벨의 작전 의미를 설명하는 metadata다.

Marker ID는 모든 타입이 공유하는 하나의 namespace다. Objective와 Observation이 같은 ID를 쓰면 로그와 Resolver가 무엇을 가리키는지 모호해지므로 거부한다. Observation의 `ObjectiveId`는 단순 거리 검색 대신 어느 지역용 후보인지 명시한다.

기존 `Validate Scenario Setup` 버튼은 Marker가 배치된 경우 ID, 반경, 연결을 함께 검사한다. Marker가 없는 기존 테스트 레벨은 그대로 유효하여 새 기능이 기존 Scenario를 강제로 깨지 않는다.

사용자가 marker 자동화 테스트 3개를 통과한 뒤 `Lvl_TS_ReconSecure_001`에 `ReconArea_A`, `ReconObs_A1`, `ReconObs_A2`를 배치했다. 잘못된 Objective ID의 의도적 실패와 복원 후 최종 validation 성공까지 확인했다.

### Phase 5 진행 기록 — Unit 2 Mission Overlay와 Blackboard 투영

상태: **Integrated Complete**

학습 목표는 C++의 판단 상태와 Behavior Tree의 실행 입력을 분리하는 것이다.

`UNPCDecisionComponent`는 AIController가 생성하고 Controller와 함께 살아 있는 Actor Component다. 이 컴포넌트가 현재 `FMissionContext`를 소유하므로 C++이 권위 원본이다. Behavior Tree의 Blackboard는 그 원본을 복사해 보관하는 두 번째 데이터베이스가 아니라, 이번 tick에 실행할 최소 신호만 받는 작업판이다.

- `MissionTargetLocation` (Vector): native `Move To`가 향할 위치다. 배치된 Observation Actor의 위치뿐 아니라 미래 EQS/NavMesh가 계산한 임시 후보 위치도 같은 형식으로 받을 수 있다.
- `bMissionMovementAllowed` (Bool): 지금 Mission branch를 실행해도 되는지를 나타낸다. 위치 `(0, 0, 0)`도 정상 좌표일 수 있으므로 Vector의 기본값만으로 임무 유무를 판정하지 않는다.

활성 임무가 있어도 NPC 상태가 `Idle` 또는 `Patrol`일 때만 이동을 허용한다. Alert, Search, Attack, TakeCover, Reload, Retreat, Hold, Suppress, Dead에서는 false가 되어 전투 판단이 우선한다. Mission decorator의 `Observer Aborts=Self`는 이 Bool이 false로 바뀌는 순간 실행 중인 Mission branch만 취소한다. 기존 전투 branch들의 abort 정책은 건드리지 않는다.

Mission branch는 root Selector에서 Alert 다음, Patrol 앞에 둔다. 그래서 경계 상태인 Alert까지는 기존 전투 대응이 우선하고, 안전한 상태에서는 일반 Patrol보다 명시적 Mission 이동이 우선한다. 이동은 완료와 실패 lifecycle을 엔진이 처리하는 native `Move To`를 사용한다.

AIController가 Pawn을 놓으면 `OnUnPossess()`가 Mission Context를 지운다. Controller나 Component가 Pawn보다 오래 남거나 다른 Pawn을 다시 소유할 수 있기 때문에, 이 생명주기 경계에서 이전 임무를 제거해야 stale state가 다음 NPC로 새지 않는다.

이 단위에는 Mission Resolver와 Group dispatch가 아직 없다. 따라서 자동화 테스트와 Blackboard/BT asset 배선까지 검증하고, 실제 NPC 이동은 후속 단위에서 임무를 주입한 뒤 확인한다.

사용자가 `BB_NPC`의 두 key와 `BT_LowIntelNPC`의 Mission Sequence, Blackboard decorator, native `Move To` 배선을 완료하고 자동화 테스트 3개의 통과를 확인했다.

관련 파일:

- `Source/Retry/Components/NPCDecisionComponent.h/.cpp`
- `Source/Retry/RetryNPCController.cpp`
- `Source/Retry/Tests/MissionOverlayTests.cpp`

새 용어:

- **권위 원본(authoritative state)**: 같은 의미의 데이터가 여러 곳에 있을 때 최종적으로 참이라고 간주하는 한 곳이다.
- **투영(projection)**: 권위 원본에서 소비자가 필요한 일부 값만 계산해 전달하는 것이다.
- **Observer Aborts Self**: decorator 조건이 실행 중 거짓이 되면 그 decorator가 속한 branch를 중단하는 Behavior Tree 설정이다.

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
