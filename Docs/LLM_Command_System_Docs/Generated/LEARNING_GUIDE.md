# LLM Command System — Learning Guide

작성일: 2026-08-02 (Asia/Seoul)  
대상: Unreal Engine과 C++ 구조를 배우면서 이 프로젝트를 직접 만들고 싶은 개발자  
연결 문서: [`IMPLEMENTATION_PLAN.md`](IMPLEMENTATION_PLAN.md)

### 2026-08-05 이후 학습·검증 방식

기능 구현은 여러 개념을 묶은 end-to-end 배치로 진행한다. 각 개념의 경계와 작은 자동화 테스트는 유지하지만, 사용자에게 유닛마다 Live Coding과 테스트를 반복하도록 요구하지 않는다. 세부 설명은 이 문서에 누적하고, 사용자는 기능 체크포인트에서 문서를 읽은 뒤 자동화 테스트 목록과 PIE 통합 절차를 한 번에 수행한다.

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

### Phase 5 진행 기록 — Unit 3 Observation Point Selector

상태: **Integrated Complete**

학습 목표는 World에서 후보를 수집하는 일과 후보 중 하나를 결정하는 일을 분리하는 것이다.

`FObservationPointSelector`는 Actor나 UObject가 아니다. 후보 배열을 입력받아 결과값을 반환하는 순수 C++ 규칙이다. 그래서 Level, NavMesh, 현재 선택 상태를 바꾸지 않고 자동화 테스트할 수 있으며, 같은 규칙을 배치 Actor와 미래 EQS 후보 모두에 재사용할 수 있다.

Selector가 받는 후보 값은 다음과 같다.

- `PointId`: 선택 결과를 로그와 Mission Context에 연결할 semantic ID
- `ObjectiveId`: 어느 목표 지역을 관측하는 후보인지 나타내는 연결
- `Location`: 선택 후 Blackboard로 전달할 위치
- `bReachable`: World/NavMesh 조사 단계에서 판정한 hard constraint
- `UtilityScore`: 거리, 가시성, 노출, 통신 같은 항목을 앞 단계에서 계산해 합친 값

도달 불가능은 점수가 낮은 후보가 아니라 선택할 수 없는 후보다. 따라서 먼저 hard constraint로 제외하고, 남은 후보끼리 utility score를 비교한다. 점수가 같으면 Point ID 순으로 선택하여 배열 순서나 Actor 검색 순서 때문에 결과가 바뀌지 않게 한다. 이 성질을 **결정성(determinism)**이라고 한다.

이번 단위는 거리·가시성 등의 가중치를 임의로 정하지 않는다. 수치가 정해지기 전에는 Selector가 이미 계산된 점수를 소비하도록 두고, 후속 단위에서 실제 Level과 gameplay 요구에 맞는 evaluator를 연결한다.

실패도 하나의 false로 뭉치지 않는다.

- `InvalidObjectiveId`: 요청 자체에 목표 ID가 없음
- `NoMatchingCandidates`: 그 Objective에 연결된 후보가 없음
- `NoUsableCandidates`: 연결 후보는 있지만 모두 도달 불가능하거나 데이터가 잘못됨

관련 파일:

- `Source/Retry/AI/ObservationPointSelector.h/.cpp`
- `Source/Retry/Tests/ObservationPointSelectorTests.cpp`

새 용어:

- **hard constraint**: 만족하지 않으면 점수와 관계없이 후보에서 제외되는 필수 조건이다.
- **utility score**: 사용 가능한 후보들 사이의 선호도를 비교하는 숫자다.
- **결정성(determinism)**: 같은 입력이면 실행 순서와 관계없이 같은 결과가 나오는 성질이다.

사용자가 Observation Selector 자동화 테스트 3개의 통과를 확인했다.

### Phase 5 진행 기록 — Unit 4 Mission Resolver

상태: **Integrated Complete**

학습 목표는 상위 Command와 하위 Mission 사이의 번역 경계를 이해하는 것이다.

`FCommandIntent`는 “ReconArea_A를 정찰하라”는 작전 의도다. NPC의 Move To는 “ReconObs_A2 위치로 이동하라”처럼 구체적인 실행 데이터가 필요하다. `FMissionResolver`가 두 표현 사이를 번역한다.

```text
Assigned FCommandIntent (Recon + Area, ReconArea_A)
→ Objective ID 일치 확인
→ Observation Selector 실행
→ FMissionContext (ReconObs_A2, 실제 위치)
```

Resolver는 UObject가 아니라 상태 없는 C++ 함수 집합이다. 생성되거나 파괴되는 객체가 아니며 Command와 후보를 입력받은 호출 동안만 계산한다. 비동기 작업, World 참조, 저장할 상태가 없기 때문에 UObject 생명주기를 추가하지 않는 것이 더 단순하다.

`FCommandValidator`를 Resolver에서 다시 호출하지 않는 이유도 중요하다. Validator는 외부에서 들어온 `Proposed` Command를 검사한다. GroupManager가 이를 통과시키면 상태가 `Assigned`가 된다. Resolver가 같은 Validator를 다시 호출하면 정상 Assigned Command를 `InvalidInitialStatus`로 거부하게 된다. 따라서 Resolver는 자신의 경계인 Assigned 상태와 지원 명령 조합만 확인한다.

선택된 Observation Point는 Mission의 실행 Objective가 된다. 원래 Area ID는 Command에 남아 있고, Mission의 `ObjectiveId`와 `ObjectiveLocation`은 NPC가 지금 향할 구체적 Point를 나타낸다. Command ID를 양쪽에 보존하므로 나중에 실행 로그와 보고를 원래 명령까지 추적할 수 있다.

Command constraint는 `bIsHardConstraint`에 따라 Mission의 Hard/Soft 배열로 분리된다. Information Requirement는 관측 후 무엇을 보고해야 하는지를 나타내므로 그대로 전달한다. Selector가 실패하면 `NoMatchingCandidates`와 `NoUsableCandidates` 같은 상세 원인을 유지한다.

관련 파일:

- `Source/Retry/AI/MissionResolver.h/.cpp`
- `Source/Retry/Tests/MissionResolverTests.cpp`

이번 단위에서 아직 하지 않는 일:

- Level에서 Objective/Observation Actor 검색
- NavMesh를 이용한 실제 도달 가능성 판정
- Group member의 `NPCDecisionComponent`에 Mission 배포
- Command를 Executing으로 전이

이 작업들은 다음 World/Group 연결 단위에서 Resolver 앞뒤에 연결한다.

사용자가 Mission Resolver 자동화 테스트 3개의 통과를 확인했다.

### Phase 5 진행 기록 — Unit 5 Recon Mission World Adapter

상태: **Integrated Complete**

학습 목표는 순수 규칙이 실제 Unreal World와 만나는 Adapter 경계와, 런타임 계산과 Editor bake 데이터의 역할 차이를 이해하는 것이다.

지금까지 Selector와 Resolver는 Actor를 검색하지 않았다. `FReconMissionWorldAdapter`가 처음으로 실제 World를 읽어 다음 변환을 수행한다.

```text
Command.TargetId
→ 같은 Marker ID의 ObjectiveAreaActor 검색
→ Objective에 연결된 ObservationPointActor 수집
→ 시작 위치에서 각 Point까지 Nav 경로 계산
→ Candidate 값 배열
→ Selector와 Mission Resolver
```

World Adapter는 시작 위치를 스스로 고르지 않는다. 호출자가 `StartLocation`과 선택적으로 실제 NPC Actor를 pathfinding context로 전달한다. 따라서 다음 단위에서 Leader를 기준으로 할지 개별 Member를 기준으로 할지 별도로 결정할 수 있다.

사용자 결정으로 첫 선택 기준은 가장 짧은 Nav 경로다. 직선거리가 아니라 NavMesh가 계산한 실제 우회 경로 길이를 사용한다. valid하고 partial이 아닌 경로만 hard constraint를 통과하며, 점수는 `-PathLength`이므로 짧을수록 높은 값이 된다. 가시성과 엄폐 가중치는 아직 섞지 않는다.

자동화 테스트에서는 실제 NavMesh 대신 path evaluator 함수를 전달한다. 이로써 World의 Objective/Observation 검색, 최단 경로 선택, Objective 없음/중복과 모든 후보 도달 불가를 빠르게 검증한다. 생산 코드에서는 같은 경계에 UE Navigation query가 들어간다.

### Baked Spatial/Tactical Data란 무엇인가

사용자가 참고한 방식처럼 맵을 일정 크기의 Cell 또는 미리 배치한 Tactical Point로 나누고, 변하지 않는 공간 특성을 Editor에서 미리 계산해 Asset으로 저장할 수 있다.

미리 계산하기 좋은 정보:

- 여러 방향에서의 엄폐율
- 특정 도시·거점·Objective에 대한 가시성
- 고도와 지형 경사
- 정적인 사격 가능 방향과 노출도
- Nav 영역과 인접 Cell

매 프레임 다시 계산해야 하는 정보:

- 현재 적의 위치와 시야
- 문이 열렸는지 여부
- 파괴된 엄폐물
- 연막과 폭발
- 현재 통신 상태

베이크 결과를 하나의 최종 점수로 저장하면 신중한 리더와 공격적인 리더가 같은 결론만 내리게 된다. 따라서 다음처럼 객관적인 feature를 분리해서 저장해야 한다.

```text
Cover = 0.8
Exposure = 0.2
VisibilityToObjective = 0.7
Elevation = 0.6
```

런타임에는 지휘 교리와 현재 Leader의 성격을 가중치로 적용해 최종 점수를 만든다.

```text
Baked Feature Channels × Doctrine Weights
+ Baked Feature Channels × Leader Personality Modifiers
+ Runtime Nav Path Cost
+ Dynamic Threat / Communication Modifier
= Candidate UtilityScore
```

현재 `FObservationPointCandidate`는 이 최종 점수를 받는 값 경계다. 미래에 베이크 Asset을 추가해도 Selector는 최고 점수를 고르고 Resolver는 Mission Context를 만드는 현재 책임을 유지한다.

### 지휘 계층과 성격은 어디에서 만나는가

상위 사령관이 “ReconArea_A를 정찰하라”고 명령하면 목표와 필수 제한은 이미 정해진다. 하위 Leader의 성격은 명령을 거부하거나 목표를 바꾸는 것이 아니라, 허용된 관측 후보 중 어떤 곳을 선호할지 결정한다.

```text
상위 Command의 Hard Constraint
→ 작전 교리의 기본 선호
→ 현재 Group Leader의 성격 보정
→ 동점이면 안정적인 Point ID 순서
```

예를 들어 신중한 Leader는 `CoverPreference`, `FearSensitivity`, `Patience`가 높다. 같은 Objective를 볼 수 있는 후보 중 Exposure가 낮고 Cover가 높은 지점에 더 큰 점수를 준다. 공격적인 Leader는 `Aggression`과 `Courage`에 따라 Visibility와 유리한 사격 각도를 더 중시할 수 있다.

현재 `UPersonalityComponent`에는 이 확장을 위한 `FPersonalitySnapshot`이 이미 있다. `CoverPreference`, `FearSensitivity`, `Aggression`, `Courage`, `TacticalSkill`, `Patience`, Stress를 후보 feature의 가중치로 변환할 수 있다.

`TacticalSkill`은 단순히 공격적인 점수를 더하는 값보다 평가 품질을 나타내는 편이 자연스럽다. 숙련된 Leader는 위험·가시성 정보를 정확히 반영하고, 숙련도가 낮은 Leader는 일부 feature를 과소평가하거나 확신도가 낮아질 수 있다. 정확한 모델은 실제 플레이 테스트 후 결정한다.

Group Mission에서는 Leader의 성격으로 그룹이 공유할 Observation Point를 한 번 선택한다. 일반 Member의 개별 성격은 선택된 목표로 이동한 뒤 엄폐, 교전, 추격 같은 micro decision에 계속 적용한다. 더 낮은 제대에 명령이 재하달되면 그 하위 Leader가 자신의 성격으로 로컬 후보를 다시 평가할 수 있다.

이 구분이 중요한 이유는 성격이 상위 명령의 hard constraint를 깨면 지휘 시스템이 예측 불가능해지기 때문이다. 성격은 선택 가능한 후보 사이의 우선순위를 바꾸지만 `도달 가능해야 한다`, `금지 구역을 통과하지 않는다` 같은 필수 조건은 바꾸지 않는다.

처음부터 Grid Asset을 만들지 않는 이유는 Cell 크기, 방향 샘플 수, World Partition streaming 단위와 갱신 정책이 아직 결정되지 않았기 때문이다. 지금은 확장 지점만 보존하고 실제 대형 Level에서 요구가 확인된 뒤 데이터 형식을 정한다.

사용자가 World Adapter 자동화 테스트 3개의 통과를 확인했다.

관련 파일:

- `Source/Retry/AI/ReconMissionWorldAdapter.h/.cpp`
- `Source/Retry/Tests/ReconMissionWorldAdapterTests.cpp`

새 용어:

- **World Adapter**: World의 Actor와 Engine Service를 순수 게임 규칙이 소비할 값으로 변환하는 경계다.
- **Bake**: 반복 계산할 정적 데이터를 Editor에서 미리 계산하여 Asset으로 저장하는 과정이다.
- **Partial Path**: 목표까지 완전히 도달하지 못하고 가능한 지점까지만 생성된 Nav 경로다.
- **Tactical Point/Grid**: 엄폐·가시성·고도 같은 전술 공간 정보를 보관하는 Point 또는 Cell 집합이다.

### Phase 5 진행 기록 — Unit 6 Atomic Group Mission Dispatch

상태: **Integrated Complete**

학습 목표는 여러 객체를 함께 변경할 때 “전부 성공하거나 전부 원래대로 돌아가는” 원자적 처리와, 그 책임을 권한 객체에 두는 이유를 이해하는 것이다.

현재 `AGroupManagerActor`는 Command의 권위 원본을 소유하고 `UNPCDecisionComponent`는 각 NPC가 실행할 Mission을 소유한다. 그래서 `Executing`으로 바꾸기 전에 실제 생존 Member 전원이 Mission을 받았는지 확인해야 한다.

```text
Assigned Command
→ 살아 있는 Leader와 Member 수신 가능 여부 사전 검사
→ Leader 위치에서 World Adapter 실행
→ 모든 Decision Component의 기존 Mission snapshot
→ 동일 Mission을 전원에게 적용
→ Execution Log와 Assigned → Executing 전이
→ 성공: 새 Mission 유지
→ 실패: snapshot으로 전원 복원
```

여기서 **원자적 배포**는 모든 NPC가 같은 전투 행동을 한다는 뜻이 아니다. 배포 시점에 같은 Mission을 받았다는 뜻이다. Mission overlay는 기존 전투 판단보다 낮은 우선순위이므로, 한 Member가 `Attack`이고 다른 Member가 `Patrol`이면 이동 시점은 달라질 수 있다.

죽은 Member는 더 이상 명령 수신자가 아니므로 제외한다. 반면 살아 있지만 Controller가 없는 Member는 조용히 제외하지 않는다. 현재 배치 NPC는 자동 Possess되므로 이 상태는 정상적인 전력 손실보다 구성 또는 수명주기 오류일 가능성이 높다. 해당 오류가 있으면 Command는 `Assigned`에 머물고 아무 NPC도 새 Mission을 받지 않는다.

롤백 전에 기존 Mission을 snapshot으로 저장하는 이유는 단순 `ClearMissionContext()`가 이전 임무까지 잃게 만들 수 있기 때문이다. 이전 Mission이 있던 NPC는 그 값을 복원하고, 원래 비어 있던 NPC만 빈 상태로 되돌린다.

Leader death, Command 완료·실패·취소, Scenario reset에서는 Mission overlay를 제거한다. 이번 단위에서 `ARetryNPCCharacter::OnDeath()`와 기존 `OnLeaderDied()`를 실제로 연결했기 때문에 별도 Blueprint death event가 필요하지 않다.

배포에 성공한 `NPCDecisionComponent`는 GroupManager가 weak reference로 기억한다. 따라서 terminal 정리 시 현재 `Members → Controller` 관계만 다시 찾는 것이 아니라 실제 Mission을 받았던 대상을 직접 정리한다. Weak reference는 대상이 먼저 파괴됐을 때 소유권을 강제로 연장하지 않고 안전하게 무효화된다.

사용자가 자동화 테스트 3개의 통과를 확인했다. 전원 적용 후 `Executing` 전이, stale Run rollback, unavailable 수신자의 사전 거부와 terminal Mission 정리가 모두 검증됐다.

현재 원자적 방식의 단점은 대규모 Group에서 한 명의 일시적 unavailable 상태가 전체 출발을 막는다는 것이다. 추후 증원·재Possess·차량 탑승이 생기면 Group Mission을 별도 `SquadBrain`이 소유하고 복귀 Member가 현재 Mission을 자동으로 재수신하도록 확장한다. 그 전에는 부분 배포로 `Executing`의 의미를 약화시키지 않는다.

사용자 실습 후보는 자동화 테스트에서 stale Run rollback과 unavailable recipient 거부 결과를 확인하는 것이다. 실제 NavMesh 이동은 다음 hardcoded Command 시작 경계를 연결한 뒤 PIE에서 확인한다.

관련 파일:

- `Source/Retry/AI/GroupManagerActor.h/.cpp`
- `Source/Retry/RetryNPCCharacter.cpp`
- `Source/Retry/Tests/GroupMissionDispatchTests.cpp`

새 용어:

- **Atomic operation**: 관련된 변경이 전부 성공하거나 전부 이전 상태로 복구되는 처리다.
- **Preflight**: 실제 상태를 바꾸기 전에 필요한 대상과 조건을 먼저 검사하는 단계다.
- **Snapshot/Rollback**: 변경 전 상태를 저장하고 실패 시 그 값으로 되돌리는 방식이다.
- **Fan-out**: 하나의 Group Mission을 여러 Member의 실행 상태로 배포하는 과정이다.

### Phase 5 진행 기록 — Unit 7 Scenario Opening Orders

상태: **Code Complete / Editor Integration Pending**

학습 목표는 정적인 DataAsset 템플릿과 한 번의 플레이에서만 유효한 runtime 객체를 분리하고, 여러 Actor의 BeginPlay 순서에 의존하지 않는 시작 경계를 이해하는 것이다.

`OpeningOrders`는 “두 진영이 왜 싸우는가”라는 설명 문구가 아니다. Scenario가 시작될 때 아직 존재하지 않는 HQ/Commander 시스템을 대신해 최초로 하달하는 실제 상위 Command 템플릿이다.

```text
DA_TS_ReconSecure_001
  Opening Order: HQ → Group A → ReconArea_A
        ↓ Run마다 새 CommandId
GroupManager A: Proposed → Validated → Assigned
        ↓ Leader 기준 World Adapter
ReconObs_A1/A2 중 최단 Nav 경로 선택
        ↓ 원자적 fan-out
Group A Decision Components
        ↓
Blackboard Mission key → Behavior Tree Move To
```

DataAsset에는 매번 달라지는 `CommandId`를 저장하지 않는다. 작성자는 Issuer, Group, Verb, Target처럼 변하지 않는 의도를 설정하고, `BuildOpeningOrders()`가 새 Run마다 identity를 생성한다. 같은 Scenario를 Restart하면 같은 명령 내용이지만 추적 가능한 새 Command가 된다.

Initializer가 자신의 `BeginPlay()` 안에서 즉시 명령을 보내면 NPC가 아직 Group에 등록되지 않았거나 AIController가 Possess하기 전일 수 있다. Unreal은 서로 다른 Actor의 BeginPlay 순서에 게임 규칙을 의존시키지 않는 편이 안전하다. 그래서 World의 다음 tick에 시작하며, callback에서도 현재 Run과 Definition이 여전히 일치하는지 다시 확인한다.

World timer delegate는 Initializer UObject에 결합한다. PIE 종료나 Level 전환으로 Initializer가 사라지면 유효하지 않은 객체에 callback을 실행하지 않는다. 이는 이전 LLM 비동기 요청 teardown 결함에서 배운 수명 규칙과 같은 방향이다.

현재 Group B는 Opening Order를 받지 않는다. Team/Perception과 기존 전투 AI는 계속 동작하므로 Group A의 Recon 도중 적을 발견하면 교전할 수 있다. Group A 리더는 상위 목표를 바꾸지 않고 `ReconObs_A1/A2` 중 실행 지점만 선택한다.

사용자 실습은 DataAsset Details에서 Opening Order를 입력하고, Menu부터 실제 Level 이동까지 첫 end-to-end PIE를 확인하는 것이다.

사용자가 DataAsset 입력 후 Scenario validation, Opening Order 실행 로그와 Group A의 실제 BT Mission 이동을 확인했다. 정적 템플릿이 새 runtime Command가 되어 Nav 후보 선택, Group dispatch, Blackboard/BT 이동까지 도달하는 흐름은 실제 레벨에서 연결됐다. Opening Order 자동화 테스트, 전투 후 Mission 복귀와 Restart/Return 수명은 다음 Phase 5 기능 체크포인트의 일괄 검증에 포함한다.

관련 파일:

- `Source/Retry/Scenario/ScenarioDefinition.h/.cpp`
- `Source/Retry/Scenario/ScenarioInitializer.h/.cpp`
- `Source/Retry/Tests/ScenarioOpeningOrderTests.cpp`

새 용어:

- **Template**: 여러 실행에서 복사해 쓰는 정적 원본 데이터다.
- **Runtime identity**: 특정 실행에서 생성된 객체를 추적하기 위한 고유 ID다.
- **Opening Order**: Scenario 시작 시 상위 지휘 체계가 최초로 하달한 작전 명령이다.
- **Next-tick initialization**: 모든 Actor의 현재 초기화 단계가 끝난 뒤 다음 World tick에서 후속 연결을 수행하는 방식이다.

### Phase 5 기능 배치 — 관측에서 HQ 보고와 완료까지

상태: **Code Complete / Batch Verification Pending**

이번 기능의 화면상 목표는 Group A의 Leader가 선택된 Observation Point에 도착하면 관측 결과가 HQ에 전달되고 Recon Command가 완료되는 것이다.

```text
Leader가 관측점 반경에 도착
→ 전투 중이 아니면 관측 hold 시간 누적
→ Operational Fact 생성
→ Report Created
→ 첫 스파이크에서는 즉시 Report Received
→ Team Operational Memory에 Fact 저장
→ 필수 정보 충족 확인
→ Command Completed
→ Group Mission overlay 정리
```

`Fact`는 “ReconArea_A가 관측됐다”처럼 하나의 작전 사실이다. `Report`는 여러 Fact를 HQ로 전달하는 봉투다. Team Memory는 Report가 `Received`가 되기 전에는 내용을 알 수 없다. 지금은 통신 지연이 없지만, 나중에 무전기·거리·재밍을 구현할 때 `Created`와 `Received` 사이에 통신 시스템을 넣을 수 있다.

`UTeamOperationalMemorySubsystem`을 World 수명으로 둔 이유는 한 전장 안에서 Team이 공유하는 정보이면서 다른 Level/Run으로 넘어가면 폐기돼야 하기 때문이다. 개인 감정과 경험은 기존 `UMemoryComponent`, 작전 보고로 확인된 정보는 Team Operational Memory가 담당한다.

관측 완료는 Group Leader를 기준으로 한다. World Adapter도 Leader 위치에서 공통 Observation Point를 선택했고, 지휘 계층에서 Leader가 Group Report를 책임진다. 일반 Member가 전투 때문에 늦는다고 보고가 영원히 막히지 않는다. 다만 Leader가 전투 상태라면 안정적인 관측으로 보지 않아 hold 시간이 초기화된다.

`FCommandExecutionMonitor`는 World를 직접 찾지 않는다. GroupManager가 “Leader 생존 여부, 도착 여부, 관측 가능 여부, 경과 시간”을 값으로 만들어 전달하고 Monitor는 Waiting, ObservationReady, Timeout 같은 결과만 반환한다. 이 분리 덕분에 이동과 전투를 실제로 재생하지 않고도 완료 규칙을 빠르게 테스트할 수 있다.

도착 판정은 3D 직선거리 하나로 계산하지 않는다. Nav 이동은 바닥 평면의 수평 거리와 Character Capsule의 높이·반경을 따로 고려해 멈추므로, 관측 판정도 수평 반경과 수직 허용치를 분리한다. 그렇지 않으면 화면상 도착한 NPC의 캡슐 중심과 바닥 Marker 사이 Z 차이가 수평 오차에 더해져 도착을 놓칠 수 있다. 반대로 수직 허용치를 별도로 제한하므로 다른 층에 있는 NPC를 같은 관측점 도착으로 오인하지 않는다.

Unreal의 높이 축은 `Z`다. Observation Point Actor는 레벨 디자이너가 의미와 방향을 배치하는 Marker이고, AI가 실제로 밟을 좌표는 Marker 주변의 NavMesh 표면으로 투영한다. 따라서 언덕이나 경사에서도 Mission의 Z가 이동 가능한 바닥 높이와 일치한다. 편집기에서도 Marker는 `End` 키 등으로 바닥에 스냅해 시각적 의미를 정확히 유지해야 하며, 런타임 투영은 잘못된 배치를 무제한으로 숨기는 용도가 아니라 작은 배치 오차와 Nav 표면 차이를 정규화하는 안전 경계다.

Opening Order에 Information Requirement가 비어 있어도 첫 수직 슬라이스는 `AreaObserved` implicit Fact를 만든다. 이후 DataAsset에 `AreaObserved / ReconArea_A / Required=true`를 명시하면 같은 실행 경계가 작성자의 요구사항을 그대로 사용한다. 아직 실제 sensor evaluator가 없는 `EnemyCountKnown` 같은 predicate는 도착했다는 이유만으로 사실을 꾸며내지 않고 거부한다.

관련 파일:

- `Source/Retry/AI/OperationalTypes.h/.cpp`
- `Source/Retry/AI/TeamOperationalMemorySubsystem.h/.cpp`
- `Source/Retry/AI/CommandExecutionMonitor.h/.cpp`
- `Source/Retry/AI/GroupManagerActor.h/.cpp`
- `Source/Retry/Scenario/ScenarioExecutionLogSubsystem.h/.cpp`
- `Source/Retry/Scenario/ScenarioInitializer.cpp`

새 용어:

- **Operational Fact**: 출처와 시점, 대상이 명확한 하나의 작전 사실이다.
- **Report Received Gate**: 보고가 수신되기 전에는 공유 정보로 사용하지 않는 경계다.
- **Idempotent**: 같은 Report가 다시 도착해도 결과가 한 번 처리한 것과 같게 유지되는 성질이다.
- **Completion Monitor**: 명령의 성공·실패·대기 조건을 관찰해 상태 전이를 결정하는 규칙이다.

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

### 이번 기능 배치에서 배우는 것 — 지역을 직접 확보하기

2026-08-06 사용자 검증에서 Secure resolver, World adapter, Area control, operational report, group dispatch, opening order 자동화 테스트가 모두 통과했다. 이제 자동화로 분리해 확인한 부품들을 기존 Scenario Definition에 연결하여 실제 이동·점유·보고 흐름을 확인하는 통합 단계다.

이번 기준선에서 `Secure Area`는 “미리 놓인 길을 따라가라”가 아니라 “이 ID의 지역을 확보하라”는 목표다. 따라서 코드가 먼저 `Objective Area Actor`를 찾고, 그 중심을 NavMesh 높이에 맞춘 실제 이동 위치로 바꾼다. Waypoint Actor는 사용하지 않는다.

`World Adapter`는 레벨의 Actor와 NavMesh를 읽는 경계다. `Mission Resolver`는 검증된 명령을 NPC가 실행할 구체적인 Mission으로 바꾼다. `Area Control Evaluator`는 World Actor를 직접 읽지 않고 이미 계산된 숫자와 bool만 받아 점령 여부를 판단한다. 이 분리 덕분에 점령 규칙은 작은 자동화 테스트로 빠르게 검증할 수 있고, 나중에 후보 위치를 만드는 방식만 베이크 데이터로 교체할 수 있다.

점령 완료는 단순 도착이 아니다.

- 살아 있는 leader가 Area 안에 있어야 한다.
- 최소 한 명의 아군 전투원이 Area 안에 있어야 한다.
- 살아 있는 적이 Area 안에 없어야 한다.
- 위 상태가 `Minimum Hold Seconds` 동안 연속으로 유지되어야 한다.
- 적이 다시 들어오거나 leader가 나가면 연속 시간이 0부터 다시 시작된다.
- 완료되면 `AreaSecured` Fact가 Report에 담기고 Team Memory가 수신해야 Command가 Completed가 된다.

실무에서 이런 구조는 목표의 의미와 이동 후보 생성 방식을 분리할 때 사용한다. 지금은 Area 중심이 하나의 기준 후보다. 추후에는 맵을 셀로 나누고 엄폐도·노출도·가시성·위험도·접근 방향을 베이크한 뒤, 같은 resolver 앞단에서 여러 후보를 공급할 수 있다. 신중한 leader는 낮은 위험도에 큰 가중치를 주고, 공격적인 leader는 속도나 압박 효과에 큰 가중치를 주게 만들 수 있다. 즉 이번 구현은 그 확장을 막지 않지만, 아직 존재하지 않는 베이크 시스템을 미리 흉내 내지는 않는다.

현재 알아둘 제한은 Opening Order들이 시나리오 시작과 동시에 독립적으로 실행된다는 점이다. Recon Report를 조건으로 Secure 명령을 나중에 발령하는 것은 command prerequisite/orchestration 기능에서 다룬다.

관련 코드 위치:

- `AI/SecureAreaWorldAdapter`: 레벨 목표와 NavMesh 해석
- `AI/MissionResolver`: Secure 명령을 Mission으로 변환
- `AI/AreaControlEvaluator`: 점유/경합/유지/실패 판정
- `AI/GroupManagerActor`: 실제 World 관찰, 팀 판정, 보고 및 완료 전이
- `AI/OperationalTypes`: `AreaSecured` Fact/Report 생성

새 용어:

- `Contested`: Area 안에 살아 있는 적이 있어 점유가 성립하지 않는 상태.
- `Stable Hold`: 점유 조건이 끊기지 않고 연속으로 유지된 시간.
- `Baked Tactical Data`: 런타임마다 비싼 공간 계산을 반복하지 않도록 에디터나 빌드 과정에서 미리 계산해 저장한 전술 데이터.

### Scripted Follow Up과 Commander 판단의 관계

자동화 검증 결과(2026-08-06): Follow Up schema, team/run Fact gate, source group 조건, all-of 조건, 그룹별 작성 순서 테스트가 모두 통과했다. 자동화 테스트 prefix는 이후 `Automation RunTests A+B+C` 형식으로 한 번에 실행한다.

Follow Up Order는 “사령관이 스스로 판단했다”는 뜻이 아니다. 디자이너가 특정 Fact 이후 실행할 명령을 미리 작성한 것이다. 자동화 테스트에서는 같은 입력으로 같은 경로를 재현하고, 게임 연출에서는 반드시 거쳐야 할 절차를 고정하는 데 사용한다.

예를 들어 전체 판단 과정을 0부터 10까지라고 할 때 0~5는 Follow Up Order로 고정하고, 6~10만 Commander Planner나 LLM이 선택하도록 만들 수 있다. 이때도 두 경로는 모두 같은 `FCommandIntent`, validation, Mission Resolver, Team Memory를 사용하므로 실행 시스템을 두 벌 만들지 않는다.

```text
디자이너 고정 구간
Opening Recon → AreaObserved → Follow Up Secure

동적 판단 구간(추후)
AreaSecured 이후 Team Memory/성격/교리/베이크 데이터를 보고
Commander가 Defend, Advance, Regroup 중 하나를 선택
```

Follow Up 조건은 수신 그룹의 Team Memory만 조회한다. 따라서 적군의 보고나 이전 Scenario Run의 Fact가 아군 명령을 실수로 시작시키지 않는다. `Source Group Id`를 비우면 같은 팀의 어느 그룹이 보고해도 되고, 값을 지정하면 그 그룹의 보고만 인정한다.

같은 그룹에 여러 Follow Up Order를 작성하면 배열 앞의 명령이 먼저 소비될 때까지 뒤 명령은 기다린다. 서로 다른 그룹은 각자의 첫 명령을 독립적으로 평가한다. 이것이 디자이너가 고정 절차를 읽고 예측할 수 있게 만드는 결정적 순서 규칙이다.

2026-08-06 PIE에서는 순차 Recon/Secure뿐 아니라 적을 만난 뒤 교전하고 지역을 확보하여 완료하는 실제 흐름까지 확인했다. 즉 Follow Up은 테스트용 데이터 전환에 그치지 않고 기존 전투와 mission execution 사이에서도 정상적으로 이어진다.

### `Use LLM`은 요청 생성 정책이다

Scenario Definition의 `Use LLM`은 개인용인지 그룹용인지 고르는 옵션이 아니다. 현재 Scenario Run 전체가 외부 LLM 요청을 사용할지를 정하는 실행 정책이다. 꺼져 있다면 개인 기억 임계값과 그룹 감정 임계값 모두 요청을 만들지 않아야 한다.

이번 결함은 옵션이 Run Context에 저장됐지만 실제 요청 경로가 읽지 않아 발생했다. 수정된 흐름은 두 겹으로 확인한다.

```text
개인/그룹 producer
→ 현재 Scenario 정책 확인
→ 허용될 때만 prompt/request 생성
→ LLM Request Queue에서 정책을 다시 확인
→ HTTP 전송 또는 fallback
```

앞의 확인은 불필요한 prompt 생성을 줄이고, Queue의 최종 확인은 앞으로 새로운 요청 호출자가 생겨도 옵션을 우회하지 못하게 한다. Scenario가 없는 기존 레벨은 호환성을 위해 이전처럼 LLM을 허용하지만, 활성 Scenario의 Run Context가 잘못됐다면 요청을 차단한다. 이를 `fail closed`라고 부른다.

관련 코드 위치:

- `Scenario/ScenarioTypes`: Scenario Run의 LLM 허용 정책
- `LLMRequestQueue`: 모든 요청의 최종 차단 지점
- `RetryNPCCharacter`: 개인 요청의 조기 차단
- `AI/GroupManagerActor`: 그룹 요청의 조기 차단
- `Tests/ScenarioRuntimeTests`: 옵션별 정책 자동화 테스트

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

## 12. 기능 배치로 반복할 학습 흐름

1. 사용자에게 보이는 end-to-end 기능과 완료 조건을 정한다.
2. 필요한 새 Unreal/C++ 개념과 설계 tradeoff를 이 문서에 기록한다.
3. 코드는 책임별로 나누고 자동화 테스트는 작은 유닛 단위로 작성한다.
4. 배치 구현 중에는 사용자 Live Coding·테스트를 요구하지 않는다.
5. 기능 코드와 테스트가 모두 준비되면 한 번의 코드 반영을 요청한다.
6. 자동화 테스트 전체 목록을 순차 실행하고 모두 통과하면 에디터 Asset 통합과 PIE를 수행한다.
7. 실제 호출 흐름, 사용자 검증 결과와 남은 질문을 문서에 갱신한다.

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
