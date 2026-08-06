# Unreal Editor Integration Actions

## 기능 배치 검증 방식 — 2026-08-05 이후

- Codex는 기능 배치의 C++와 유닛 단위 자동화 테스트를 모두 작성한 뒤 한 번에 사용자 검증을 요청한다.
- 사용자는 각 유닛마다 Live Coding하지 않고 기능 체크포인트에서 한 번만 코드를 반영한다.
- Codex는 체크포인트마다 실행할 자동화 테스트 이름을 순서대로 한 목록으로 제공한다.
- 자동화가 모두 통과한 뒤에만 DataAsset/Blueprint/Level 연결과 PIE 실제 동작 검증을 진행한다.
- 아래 과거 Unit별 절차는 당시 진행 기록으로 유지하며, 이후 작업에는 기능 배치 방식을 우선 적용한다.

## Phase 3 Unit 1 — Scenario Definition DataAsset

### Codex가 구현한 것

- `FScenarioLaunchOptions`: Seed, LLM 사용, 로그, 자동 시작 기본값.
- `UScenarioDefinition : UPrimaryDataAsset`: Scenario ID, 표시명, 설명, Soft Level 참조, 기본 실행 옵션.
- `IsDefinitionValid()`: 필수 ID, 표시명, Level 누락 검사.
- `GetPrimaryAssetId()`: `ScenarioDefinition:ScenarioId` 형식의 식별자 반환.

Blueprint에 property를 읽기 전용으로 노출한 이유는 Widget이 Scenario 목록과 설명을 표시할 수 있게 하되, 실행 중 임의 변경은 막기 위해서다. `IsDefinitionValid()`는 이후 Registry와 디버그 UI에서 같은 검증 규칙을 재사용하기 위해 노출했다.

### 사용자가 에디터에서 해야 할 것

Editor target 빌드와 에디터 재시작 후 수행한다.

1. `/Game/Scenarios` 폴더를 만든다.
2. Miscellaneous > Data Asset을 선택한다.
3. Data Asset Class로 `ScenarioDefinition`을 선택한다.
4. 이름을 `DA_TS_ReconSecure_001`로 지정한다.
5. 다음 property를 설정한다.
   - Scenario ID: `TS_ReconSecure_001`
   - Display Name: `Recon → Report → Secure 001`
   - Description: 첫 기술 스파이크를 설명하는 문장
   - Level: 복제한 전용 기능 검증 Level `Lvl_TS_ReconSecure_001`
   - Default Launch Options > Seed: `1001`
   - Use LLM: false
   - Enable Logging: true
   - Auto Start: true
6. 저장한다.

### 검증 절차

1. DataAsset 생성 메뉴에 `ScenarioDefinition`이 표시되는지 확인한다.
2. 위 property가 Details 패널에 표시되는지 확인한다.
3. Scenario ID와 Level을 비운 상태가 이후 Registry validation에서 거부되도록 현재 입력값을 확인한다.
4. 저장 후 asset 경로와 property 값을 기록한다.

### 현재 통합 상태

- [x] C++ Game Development 빌드 성공
- [x] Editor target 빌드 성공
- [x] `DA_TS_ReconSecure_001` 생성
- [x] 필수 property 설정
- [x] DataAsset 저장 및 전용 레벨 정상 동작 확인

사용자 검증으로 Phase 3 Unit 1은 **Integrated Complete**다.

---

## Phase 3 Unit 2 — Scenario Registry Settings

### Codex가 구현한 것

- `UScenarioRegistrySettings : UDeveloperSettings`
- Project Settings의 `Registered Scenarios` Soft Reference 목록
- 빈 참조, 로드 실패, 잘못된 Definition, 중복 Scenario ID 검증
- Registry validation 자동화 테스트 두 개
- `DeveloperSettings` 모듈 의존성

Registry는 Scenario를 실행하지 않는다. “어떤 Scenario를 실행 대상으로 인정할 것인가”만 저장하며, 실제 조회와 레벨 전환은 다음 Unit의 `UScenarioRuntimeSubsystem`이 담당한다.

### 사용자가 에디터에서 해야 할 것

1. Live Coding을 실행한다.
2. Project Settings를 연다.
3. `Game > Scenario Registry` 항목을 찾는다.
4. `Registered Scenarios` 배열에 항목 하나를 추가한다.
5. `/Game/Scenarios/Definitions/DA_TS_ReconSecure_001`을 지정한다.
6. Project Settings를 저장하고 `DefaultGame.ini` 변경을 확인한다.

이번 수정은 새 `UCLASS`와 Config `UPROPERTY`를 포함한다. Live Coding 성공 후에도 `Scenario Registry`가 나타나지 않으면 UHT 반영을 위해 에디터 재시작과 사용자 주도 전체 빌드가 필요할 수 있다. Codex는 빌드를 실행하지 않는다.

### 검증 절차

1. `Registered Scenarios`가 정확히 한 항목인지 확인한다.
2. 등록 Asset이 `DA_TS_ReconSecure_001`인지 확인한다.
3. Automation에서 `Retry.Scenario.Registry.RejectsInvalidEntries`와 `RejectsDuplicateIds`를 실행한다.
4. 두 테스트가 통과하면 Unit 2 C++ validation을 확인한 것이다.

### 현재 통합 상태

- [x] Registry C++ 및 자동화 테스트 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 빌드 및 에디터 재시작
- [x] Project Settings에 `DA_TS_ReconSecure_001` 등록
- [x] `DefaultGame.ini` Soft Reference 저장 확인
- [x] Registry 자동화 테스트 통과

사용자 검증으로 Phase 3 Unit 2는 **Integrated Complete**다.

---

## Phase 3 Unit 3 — Scenario Runtime Subsystem

### Codex가 구현한 것

- `FScenarioRunContext`: Run ID, Scenario ID, Level, Launch Options, 활성 상태
- `TryCreateScenarioRunContext()`: Definition 검증과 새 Run GUID 생성
- `UScenarioRuntimeSubsystem : UGameInstanceSubsystem`
- 등록 Scenario 목록 조회와 `StartScenario`
- 같은 옵션을 유지하면서 새 Run ID를 만드는 `RestartCurrentScenario`
- 메뉴 Level 존재 여부를 확인하는 `ReturnToScenarioMenu`
- Level 전환 직전 LLM queue reset
- Run Context 보존·새 GUID·잘못된 Definition 자동화 테스트

RuntimeSubsystem은 Unreal이 GameInstance마다 자동 생성한다. 메뉴 Widget이나 Level Actor가 직접 생성하지 않으며 Level 전환 후에도 같은 객체가 유지된다.

### 사용자가 확인할 것

이번 수정은 새 `UCLASS`, `USTRUCT`, Blueprint API를 포함한다. 사용자가 Live Coding 또는 사용자 주도 빌드·재시작으로 반영한다. Codex는 빌드를 실행하지 않는다.

Output Log에서 다음을 실행한다.

```text
Automation RunTests Retry.Scenario.Runtime
```

기대 테스트:

- `Retry.Scenario.Runtime.PreservesLaunchData`
- `Retry.Scenario.Runtime.RejectsInvalidDefinition`

첫 테스트는 Scenario ID, Seed/옵션, Soft Level이 Run Context에 보존되고 다시 생성할 때 Run ID가 달라지는지 확인한다. 두 번째 테스트는 Level이 없는 Definition이 활성 실행 상태를 만들지 못하는지 확인한다.

### 실제 OpenLevel 통합 결과

- `/Game/Scenarios/Maps/Lvl_ScenarioMenu`와 Scenario 선택 UI가 생성됐다.
- `StartScenario → OpenLevel → ScenarioInitializer`의 실제 Level 전환을 확인했다.
- 메뉴에서 지정한 Seed, 맵명, Level 초기화 성공이 로그에 기록됐다.
- Game Default Map은 메뉴로 변경했고 Editor Startup Map은 기존 값을 유지했다.

### 현재 통합 상태

- [x] RuntimeSubsystem C++ 및 자동화 테스트 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 Live Coding 또는 빌드·재시작
- [x] Runtime 자동화 테스트 통과
- [x] 후속 Widget 연결 후 실제 OpenLevel 검증

Unit 3은 Unit 5의 사용자 통합 검증을 포함해 **Integrated Complete** 상태다.

---

## Phase 3 Unit 4 — Scenario Initializer

### Codex가 구현한 것

- 레벨 배치 Actor `AScenarioInitializer`
- Details 패널의 `Validate Scenario Setup` 버튼
- Definition/현재 Level, Group ID, NPC Name, Group 참조, Team ID, 그룹별 Leader 검증
- Run Context가 있는 정상 진입에서 Seed와 LLM/group/NPC 런타임 상태 초기화
- Group Manager의 `TeamID`, `ResetGroupRuntimeState()`와 Memory Component의 `ResetMemories()`

### 사용자가 에디터에서 해야 할 것

이번 수정은 새 `UCLASS`, `UENUM`, `UPROPERTY`, `UFUNCTION`을 포함한다. 먼저 Live Coding을 시도하되 새 Actor나 property가 나타나지 않으면 에디터 재시작과 사용자 주도 빌드가 필요할 수 있다. Codex는 빌드를 실행하지 않는다.

1. `Lvl_TS_ReconSecure_001`을 연다.
2. Place Actors에서 `ScenarioInitializer`를 찾아 레벨에 정확히 하나 배치한다.
3. Initializer의 `Scenario Definition`에 `/Game/Scenarios/Definitions/DA_TS_ReconSecure_001`을 지정한다.
4. Group A와 Group B의 새 `Team ID`를 각각 소속 NPC들의 `Team ID`와 같은 값으로 지정한다.
5. 각 Group에 정확히 한 NPC만 `Is Group Leader`가 체크되어 있는지 확인한다.
6. Initializer의 Details 패널에서 `Validate Scenario Setup`을 누른다.
7. `Last Initialization Result = Succeeded`, `Last Validation Message = Scenario setup is valid.`와 성공 로그를 확인한다.
8. Level을 저장한다.

### 의도적인 실패 확인

1. Group 하나의 Team ID를 임시로 소속 NPC와 다르게 바꾼다.
2. `Validate Scenario Setup`을 눌러 `Invalid Actor Configuration`과 Team ID 불일치 메시지를 확인한다.
3. Team ID를 원래 값으로 복원하고 다시 성공하는지 확인한 뒤 저장한다.

Level을 직접 열어 PIE하면 아직 메뉴가 Run Context를 만들지 않았으므로 `[Scenario] No active run context` 경고가 정상이다. 이 경로에서는 초기화를 건너뛰며 기존 전투 테스트는 계속 동작해야 한다. 실제 Seed 적용과 정상 초기화 성공 로그는 Unit 5의 Start 버튼 연결 후 확인한다.

### 현재 통합 상태

- [x] Initializer 및 reset C++ 작성
- [x] 변경 범위 정적 검사
- [x] 사용자 코드 반영
- [x] Initializer 배치와 Definition/Team ID 연결
- [x] 성공 및 의도적 실패 validation 확인

Unit 4는 **Integrated Complete** 상태다.

### 레벨 테스트 후 Ragdoll 충돌 회귀 수정

사망 Capsule을 비활성화하고 시체 Mesh가 투사체를 막지 않게 만든 이전 처리에서, Mesh가 `WorldDynamic` 전체를 무시해 동적 바닥까지 통과할 수 있었다.

수정된 충돌 구분:

- 투사체 Sphere: `Projectile` object channel (`ECC_GameTraceChannel2`)
- 사망 Capsule: `NoCollision`
- Ragdoll Mesh: `QueryAndPhysics`, WorldStatic/WorldDynamic Block, Projectile Ignore

사용자가 Project Settings에서 `Projectile` Object Channel을 생성했고, 에디터가 `ECC_GameTraceChannel2`, Default Response `Block`으로 저장한 것을 확인했다.

사용자가 코드 반영 후 Ragdoll이 바닥에 정착하고 투사체가 시체를 통과해 뒤의 NPC를 타격하는 것을 모두 확인했다.

검증 절차:

1. `Project Settings > Engine > Collision`을 연다.
2. `Object Channels`에서 새 채널을 추가한다.
3. 이름을 정확히 `Projectile`, Default Response를 `Block`으로 지정한다.
4. 생성된 채널이 `GameTraceChannel2`에 할당됐는지 확인한다. 다른 번호라면 C++의 `ECC_GameTraceChannel2`와 맞추기 전에 사용자와 Codex가 함께 조정한다.
5. Project Settings를 닫고 필요하면 에디터를 재시작한 뒤 코드를 반영한다.
6. NPC를 바닥 위에서 사망시켜 Ragdoll이 바닥에 정착하는지 확인한다.
7. 바닥에 놓인 시체 뒤의 살아 있는 NPC를 사격한다.
8. 총알이 시체에 소모되지 않고 뒤의 NPC에게 피해를 주는지 확인한다.
9. 가능하면 정적 바닥과 이동 가능한 동적 바닥에서 각각 확인한다.

---

## Phase 3 Unit 5 — Dynamic Scenario Selection UI

### Codex가 구현한 것

- `UScenarioSelectWidget : UUserWidget` C++ 부모
- Registry 기반 동적 Scenario 목록과 첫 항목 기본 선택
- 등록 목록 밖 Definition 선택 거부
- 선택 시 Definition의 기본 Launch Options 복사
- Blueprint용 목록 갱신·선택 변경·시작 실패 event
- 선택 옵션을 사용한 `StartSelectedScenario()`

이번 수정은 새 `UCLASS`와 Blueprint API를 포함한다. 사용자가 Live Coding으로 반영하되 C++ 부모가 나타나지 않으면 사용자 주도 빌드·에디터 재시작이 필요하다. Codex는 빌드를 실행하지 않는다.

### 사용자 실습 A — 메뉴 Asset 뼈대

1. `/Game/Scenarios/UI` 폴더를 만든다.
2. `ScenarioSelectWidget`을 부모로 `WBP_ScenarioSelect`을 만든다.
3. 일반 `UserWidget` 부모로 `WBP_ScenarioEntry`를 만든다.
4. `PlayerController` 부모로 `BP_ScenarioMenuPlayerController`를 만든다.
5. `GameModeBase` 부모로 `BP_ScenarioMenuGameMode`를 만든다.
6. `/Game/Scenarios/Maps/Lvl_ScenarioMenu` 빈 Level을 만든다.

### 사용자 실습 B — WBP_ScenarioEntry

1. Button 하나와 Scenario 이름을 표시할 TextBlock 하나를 배치한다.
2. `Scenario` 변수를 `ScenarioDefinition Object Reference`로 만들고 `Instance Editable`, `Expose on Spawn`을 켠다.
3. `Owner Scenario Widget` 변수를 `ScenarioSelectWidget Object Reference`로 만들고 같은 옵션을 켠다.
4. `Pre Construct` 또는 별도 초기화 함수에서 `Scenario.Display Name`을 TextBlock에 표시한다.
5. Button의 `OnClicked`에서 `Owner Scenario Widget → Select Scenario(Scenario)`를 호출한다.

### 사용자 실습 C — WBP_ScenarioSelect

권장 레이아웃은 왼쪽 `ScrollBox`, 오른쪽 상세 패널이다. 상세 패널에는 Display Name, Description, Seed 입력, Use LLM·Enable Logging·Auto Start 체크박스, Start 버튼, 오류 TextBlock을 둔다.

1. `Refresh Scenario List` event를 구현한다.
2. ScrollBox의 기존 children을 지운다.
3. 전달받은 Scenarios를 `For Each Loop`로 순회한다.
4. 매 항목마다 `Create Widget(WBP_ScenarioEntry)`를 호출하고 Scenario와 `self`를 전달한다.
5. 생성한 Entry를 ScrollBox에 추가한다.
6. `Scenario Selection Changed` event에서 이름·설명과 Launch Options UI를 갱신한다.
7. Start Button에서 현재 UI 값으로 `FScenarioLaunchOptions`를 만든다.
8. `Set Selected Launch Options` 후 `Start Selected Scenario`를 호출한다.
9. `Scenario Start Failed` event에서 오류 TextBlock을 표시한다.

### 사용자 실습 D — 메뉴 소유권과 Level 연결

1. `BP_ScenarioMenuPlayerController`의 BeginPlay에서 `WBP_ScenarioSelect`을 생성한다. Owning Player는 self다.
2. 생성한 Widget을 Viewport에 추가한다.
3. `Show Mouse Cursor = true`, `Set Input Mode UI Only`로 설정한다.
4. `BP_ScenarioMenuGameMode`의 Player Controller Class를 위 Controller로 지정하고 Default Pawn Class는 비운다.
5. `Lvl_ScenarioMenu`의 World Settings에서 GameMode Override를 위 GameMode로 지정하고 저장한다.
6. 이 단계에서는 Project Settings의 Game Default Map을 바꾸지 않는다. 먼저 메뉴 Level을 직접 열어 PIE로 검증한다.

### 검증 순서

1. 메뉴 PIE에서 Registry의 `DA_TS_ReconSecure_001`이 목록과 상세 패널에 나타난다.
2. Seed를 알아보기 쉬운 값(예: `4242`)으로 바꾸고 Start한다.
3. `Lvl_TS_ReconSecure_001`로 전환된다.
4. 로그에 같은 Scenario ID, 새 Run ID, Seed `4242`가 출력된다.
5. Initializer 로그가 성공이고 `MissingRunContext` 경고가 없어야 한다.

### 현재 통합 상태

- [x] Widget C++ 부모 작성
- [x] 학습/에디터 절차 작성
- [x] 사용자 코드 반영
- [x] 메뉴 Level/Controller/GameMode/Widget Blueprint 생성
- [x] Start → OpenLevel → Initializer 성공 검증

Unit 5는 **Integrated Complete** 상태다.

---

## Phase 3 Unit 6 — Restart / Return Debug Panel

### Codex가 구현한 것

- `UScenarioDebugWidget` C++ Widget 부모
- Run Context 조회와 Blueprint 갱신 event
- 안전한 Restart/Return 호출과 실패 event
- 기존 `ARetryPlayerController`의 Scenario Debug Widget 생성·F12 Action 토글
- Return 성공 로그의 이전 Run ID 기록

이번 수정은 새 `UCLASS`, `UPROPERTY`, Blueprint API를 포함한다. 사용자가 Live Coding으로 반영하되 C++ 부모나 Class Defaults 항목이 나타나지 않으면 사용자 주도 빌드·에디터 재시작이 필요하다. Codex는 빌드를 실행하지 않는다.

### 사용자 실습 A — Enhanced Input

1. `/Game/Input/Actions`에 `IA_ScenarioDebug` Input Action을 만든다.
2. Value Type은 `Digital (bool)`로 둔다.
3. `/Game/Input/IMC_Default`를 열고 mapping 하나를 추가한다.
4. Action은 `IA_ScenarioDebug`, Key는 `F12`로 설정하고 저장한다. F9/F10/F11은 UE Editor 기본 명령과 충돌하므로 피한다.

### 사용자 실습 B — WBP_ScenarioDebug

1. `/Game/Scenarios/UI`에 `ScenarioDebugWidget`을 부모로 `WBP_ScenarioDebug`를 만든다.
2. 화면 우측 상단에 Border/VerticalBox 패널을 배치한다.
3. Scenario ID, Run ID, Seed, Active 상태용 TextBlock을 만든다.
4. `Restart`, `Return to Menu` Button과 오류 TextBlock을 만든다.
5. `Run Context Changed` event에서 `Break ScenarioRunContext`로 값을 분리한다.
6. Scenario ID는 Name→Text, Run ID는 Guid→String→Text, Seed는 Int→Text로 변환해 표시한다.
7. `Restart` OnClicked에서 `Restart Current Scenario`를 호출한다.
8. `Return to Menu` OnClicked에서 `Return to Scenario Menu`를 호출한다.
9. `Scenario Action Failed` event에서 오류 TextBlock을 표시한다.
10. Compile하고 저장한다.

### 사용자 실습 C — 기존 전투 PlayerController 연결

1. `/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController`를 연다.
2. Class Defaults의 `UI > Scenario > Scenario Debug Widget Class`에 `WBP_ScenarioDebug`를 지정한다.
3. `Input > Actions > IA Scenario Debug`에 `IA_ScenarioDebug`를 지정한다.
4. Compile하고 저장한다.

### 기본 검증

1. 메뉴에서 Seed를 `4242`, Use LLM은 false로 두고 Scenario를 시작한다.
2. 전투 Level에서 F12를 눌러 패널이 열리고 다시 누르면 닫히는지 확인한다.
3. Scenario ID, Seed `4242`, 유효한 Run ID, Active=true를 확인하고 첫 Run ID를 기록한다.
4. 다시 패널을 열어 `Restart`를 누른다.
5. 같은 Level이 다시 열리고 Initializer 성공 로그가 나오는지 확인한다.
6. F12 패널에서 Seed는 `4242`로 같고 Run ID는 이전 값과 다른지 확인한다.
7. NPC HP, 위치, memory와 group runtime 상태가 새 Level 상태로 돌아왔는지 확인한다.
8. `Return to Menu`를 눌러 `Lvl_ScenarioMenu`로 이동하는지 확인한다.
9. 로그에 `Returning to menu. Previous Run:`이 기록되는지 확인한다.
10. 메뉴에서 다시 Start해 또 다른 Run ID가 생성되는지 확인한다.

### 수명주기 회귀 검증

1. Use LLM=true로 Scenario를 시작하되 로컬 LLM 서버는 끈다.
2. LLM 요청이 active 또는 pending인 상태를 만든다.
3. F12에서 Restart 또는 Return을 누른다.
4. editor crash가 없고 queue 전환 정리 로그가 한 번 기록되는지 확인한다.
5. 이전 Run의 late callback, fallback, personality/order 결과가 새 Run에 적용되지 않는지 확인한다.

### 현재 통합 상태

- [x] Debug Widget C++ 부모와 PlayerController 토글 작성
- [x] Restart/Return 추적 로그 작성
- [x] 학습/에디터 절차 작성
- [x] 사용자 코드 반영
- [x] Input Action/Mapping과 Widget Blueprint 연결
- [x] Restart Level 전환 검증
- [x] Return 메뉴 복귀 검증
- [x] active LLM 요청 중 전환 회귀 검증

Unit 6는 **Integrated Complete** 상태다.

Phase 3은 **Feature Integrated Complete / Packaging Gate Pending** 상태다. 사용자 주도 Game target build와 등록 map cook/package 검증은 별도 최종 게이트로 남긴다.

---

## Phase 4 Unit 1 — Command Types and Status Transitions

### Codex가 구현한 것

- `ECommandVerb`: Recon, Secure, Defend, Block
- `ECommandTargetType`: Area, Route, Position, Unit, Information
- `ECommandStatus`: Proposed부터 terminal 상태까지
- `FCommandIntent`, `FMissionContext`와 constraint/info/completion value struct
- 모든 비종료 상태에서 취소 가능한 순수 상태 전이 함수
- 정상 전이, 취소, 역전이/terminal 거부 자동화 테스트 3개

이번 수정은 새 `UENUM`, `USTRUCT`를 포함한다. 사용자가 Live Coding으로 반영하되 UHT 반영이 되지 않으면 사용자 주도 빌드·에디터 재시작이 필요하다. Codex는 빌드를 실행하지 않는다.

Output Log에서 다음을 실행한다.

```text
Automation RunTests Retry.Command.Status
```

기대 테스트:

- `Retry.Command.Status.AcceptsForwardTransitions`
- `Retry.Command.Status.AllowsCancellationBeforeTerminal`
- `Retry.Command.Status.RejectsInvalidTransitions`

### 현재 통합 상태

- [x] Command/Mission value type 작성
- [x] 상태 전이 규칙과 자동화 테스트 작성
- [x] 기존 `ENPCOrder`, `LLMTypes` 무변경 확인
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] Command Status 자동화 테스트 3개 통과

Unit 1은 **Integrated Complete** 상태다.

---

## Phase 4 Unit 2 — Command Validation

### Codex가 구현한 것

- 구조화 오류 코드와 issue/result value struct
- Command identity, 초기 Status, Priority와 Target 검사
- 다섯 개 허용 Verb–Target 조합
- Constraint/Information Requirement/Completion nested data 검사
- 첫 오류에서 멈추지 않는 전체 오류 수집
- Validation 성공 전 Command 상태를 바꾸지 않는 순수 Validator

이번 수정은 새 `UENUM`, `USTRUCT`를 포함한다. 사용자가 Live Coding으로 반영하되 UHT 반영이 되지 않으면 사용자 주도 빌드·에디터 재시작이 필요하다. Codex는 빌드를 실행하지 않는다.

Output Log에서 다음을 실행한다.

```text
Automation RunTests Retry.Command.Validation
```

기대 테스트:

- `Retry.Command.Validation.AcceptsSupportedCombinations`
- `Retry.Command.Validation.RejectsUnsupportedCombinations`
- `Retry.Command.Validation.CollectsIdentityAndRangeErrors`
- `Retry.Command.Validation.RejectsInvalidNestedData`

### 현재 통합 상태

- [x] 구조화 validation result와 Validator 작성
- [x] 허용 조합 및 identity/range/nested 규칙 작성
- [x] 자동화 테스트 4개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] Command Validation 자동화 테스트 4개 통과

Unit 2는 **Integrated Complete** 상태다.

---

## Phase 4 Unit 3 — Scenario Execution Log

### Codex가 구현한 것

- GameInstance 수명의 Run별 메모리 실행 로그
- Run 시작·종료와 종료 이유 기록
- Run/Command/Event ID와 Run 내부 순번 연결
- Command validation 결과 및 상태 전이용 구조화 이벤트
- 이전 Run의 늦은 event write 거부
- Restart 시 완료 Run 보존, Return 시 활성 Run 종료
- ID 연결·stale write·완료 Run 보존 자동화 테스트 3개

이번 변경은 새 `UCLASS`, `UENUM`, `USTRUCT`를 포함한다. 사용자가 코드 반영 후 Unreal Editor에 타입이 나타나지 않으면 Editor를 닫고 사용자 주도 빌드·재시작이 필요하다. Codex는 빌드를 실행하지 않는다.

### 사용자 검증 절차

Output Log에서 다음을 실행한다.

```text
Automation RunTests Retry.Scenario.ExecutionLog
```

기대 테스트:

- `Retry.Scenario.ExecutionLog.LinksRunCommandAndEventIds`
- `Retry.Scenario.ExecutionLog.RejectsStaleRunWrites`
- `Retry.Scenario.ExecutionLog.PreservesCompletedRuns`

이 단위에는 Blueprint, Level, Project Settings 변경이 없다. 세 테스트가 모두 통과하면 결과를 알려준다.

첫 실행에서 테스트 fixture가 Subsystem을 transient package 아래에 생성하여 `ClassWithin GameInstance` ensure가 발생했다. fixture는 임시 `UGameInstance`를 올바른 Outer로 사용하도록 수정됐다. 코드 반영 후 세 테스트를 다시 실행하며, Success 표시뿐 아니라 해당 ensure가 더 이상 없는지도 확인한다.

### 현재 통합 상태

- [x] 실행 로그 Subsystem과 구조화 타입 작성
- [x] Scenario Start/Restart/Return 수명 연결
- [x] stale Run 및 상태 전이 기록 경계 작성
- [x] 자동화 테스트 3개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] Scenario Execution Log 자동화 테스트 3개 통과
- [x] `ClassWithin GameInstance` ensure 제거 확인

Unit 3는 **Integrated Complete** 상태다.

---

## Phase 4 Unit 4 — Group Command Authority

### Codex가 구현한 것

- GroupManager의 Current Command 권위 상태
- 구조화된 Command assignment outcome/result
- 활성 Scenario Run과 Execution Log가 있는 할당 경계
- validation 및 Assigned Group ID 일치 검사
- `Proposed → Validated → Assigned` 전이와 구조화 로그
- 활성 Command 덮어쓰기 방지
- Cancel/terminal Clear 수명 규칙
- 기존 `SetOrderForAll` 전투 명령 경로 보존
- Group authority 자동화 테스트 3개

이번 변경은 새 `UENUM`, `USTRUCT`, `UPROPERTY`, `UFUNCTION`을 포함한다. 사용자가 Live Coding으로 반영하되 새 reflection 타입이 나타나지 않으면 Editor를 닫고 사용자 주도 빌드·재시작이 필요할 수 있다. Codex는 빌드를 실행하지 않는다.

### 사용자 검증 절차

Output Log에서 다음을 실행한다.

```text
Automation RunTests Retry.Command.GroupAuthority
```

기대 테스트:

- `Retry.Command.GroupAuthority.AssignsAndLogs`
- `Retry.Command.GroupAuthority.RejectsInvalidOrMismatchedCommands`
- `Retry.Command.GroupAuthority.PreventsReplacementAndRequiresTerminalClear`

테스트는 임시 Preview World를 만들었다가 정리한다. 현재 열린 Level이나 Asset을 저장하지 않는다. 세 테스트의 Success와 함께 ensure, World cleanup 오류가 없는지도 확인한다.

### 현재 통합 상태

- [x] GroupManager Command 소유 및 assignment result 작성
- [x] validation·Group ID·활성 Command 경계 작성
- [x] 상태 전이와 Execution Log 연결
- [x] Cancel/terminal Clear 규칙 작성
- [x] 기존 `SetOrderForAll` 경로 무변경 확인
- [x] 자동화 테스트 3개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] Group Authority 자동화 테스트 3개 통과

Unit 4는 **Integrated Complete** 상태다.

Phase 4는 Command Status 3개, Validation 4개, Execution Log 3개, Group Authority 3개로 총 13개 자동화 테스트의 사용자 통과를 확인하여 **Feature Integrated Complete** 상태다.

---

## Phase 5 Unit 0 — ReconArea Preflight

### 읽기 전용 조사 결과

- `BB_NPC`의 기존 11개 key는 전투 의미로 유지한다.
- `BT_LowIntelNPC`의 모든 CombatState decorator는 현재 `FlowAbortMode=None`이다.
- 커스텀 `MoveToTarget`은 요청 직후 성공하고 `MoveToCover`는 이동 완료 callback이 없다.
- Recon Mission 이동은 별도 branch에서 native `Move To`를 사용하는 방향이 안전하다.
- 첫 구현 범위는 Objective Area와 Observation Point이며 Route marker는 후속으로 미룬다.
- 기존 Scenario Level에는 새 Mission marker가 아직 없다.

이 preflight에서는 Editor Asset을 열거나 변경하지 않았다. Marker 구조를 결정하고 C++ 타입을 만든 뒤 사용자 실습으로 Level에 배치한다.

상태: **Complete / Marker Architecture Decided**

---

## Phase 5 Unit 1 — Marker Foundation

### Codex가 구현한 것

- abstract `ScenarioMarkerActor` 공통 기반
- Objective Area 중심·반경과 에디터 Sphere 표시
- Observation Point의 Objective 연결과 에디터 Arrow 표시
- 공통 ID 중복, 반경, Objective 연결 구조화 validation
- 기존 `Validate Scenario Setup`에 marker 검증 연결
- marker 자동화 테스트 3개

이번 변경은 새 `UCLASS`, `UENUM`, `USTRUCT`, Component를 포함한다. Live Coding으로 반영하되 native Actor가 Place Actors에 나타나지 않으면 Editor를 닫고 사용자 주도 빌드·재시작이 필요할 수 있다. Codex는 빌드를 실행하지 않는다.

### 1. 자동화 검증

Output Log에서 실행한다.

```text
Automation RunTests Retry.Scenario.Markers
```

기대 테스트:

- `Retry.Scenario.Markers.AcceptsLinkedObjectiveAndObservation`
- `Retry.Scenario.Markers.RejectsDuplicateIds`
- `Retry.Scenario.Markers.RejectsInvalidAreaAndUnknownObjective`

### 2. Level 배치

자동화가 통과한 뒤 `Lvl_TS_ReconSecure_001`에서 진행한다.

1. `Objective Area Actor` 하나를 작전 목표 지역 중앙에 배치한다.
2. `Marker ID = ReconArea_A`, `Area Radius = 500`으로 시작한다.
3. `Observation Point Actor` 두 개를 NavMesh 안의 서로 다른 관측 후보 위치에 배치한다.
4. 첫 Point는 `Marker ID = ReconObs_A1`, 두 번째는 `ReconObs_A2`로 설정한다.
5. 두 Point 모두 `Objective ID = ReconArea_A`로 설정한다.
6. Arrow가 목표 지역을 향하도록 회전한다. 현재는 에디터 의미 표시이며 후속 관측 방향 평가에서 사용한다.
7. `ScenarioInitializer`의 `Validate Scenario Setup`을 실행해 성공을 확인한다.
8. 한 Point의 Objective ID를 임시로 `MissingArea`로 바꿔 실패 메시지를 확인한다.
9. `ReconArea_A`로 복원하고 다시 성공한 뒤 Level을 저장한다.

### 현재 통합 상태

- [x] Marker 공통/전문 Actor와 Component 작성
- [x] 구조화 marker validation 작성
- [x] Scenario Initializer 검증 연결
- [x] 자동화 테스트 3개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] Marker 자동화 테스트 3개 통과
- [x] Objective 1개와 Observation 2개 배치·연결
- [x] 의도적 실패와 최종 성공 validation 확인

Unit 1은 **Integrated Complete** 상태다.

---

## Phase 5 Unit 2 — Mission Overlay와 Blackboard 실행 투영

### Codex가 구현한 것

- `UNPCDecisionComponent`의 Mission Context 설정, 조회, 해제와 입력 검증
- 기존 Decision Service 주기의 Blackboard 위치/이동 허용 값 투영
- Idle/Patrol에서만 Mission 이동을 허용하는 전투 우선 규칙
- `OnUnPossess()`에서 이전 Mission Context 정리
- Mission Overlay 자동화 테스트 3개

새 `UFUNCTION`과 `UPROPERTY`가 있으므로 Live Coding 반영이 불완전하면 사용자가 에디터를 닫고 직접 빌드·재시작한다. Codex는 빌드를 실행하지 않는다.

### 1. 코드 반영 후 자동화 테스트

Output Log에서 실행한다.

```text
Automation RunTests Retry.Mission.Overlay
```

기대 테스트:

- `Retry.Mission.Overlay.AcceptsAndClearsValidContext`
- `Retry.Mission.Overlay.RejectsInvalidContext`
- `Retry.Mission.Overlay.AllowsOnlyIdleAndPatrolMovement`

### 2. Blackboard key 추가

테스트가 통과한 뒤 `/Game/AI/BB_NPC`를 열어 다음 두 key를 정확한 이름과 타입으로 추가한다.

1. `MissionTargetLocation` — Vector
2. `bMissionMovementAllowed` — Bool
3. Blackboard를 저장한다.

기존 `TargetActor`, `LastKnownEnemyLocation`, `CoverLocation` 등 전투 key는 수정하지 않는다. 새 key를 만들기 전에는 PIE를 실행하지 않는다. C++이 아직 존재하지 않는 key에 쓰려고 하면 missing key 경고가 생길 수 있다.

### 3. Behavior Tree Mission branch 추가

`/Game/AI/BT_LowIntelNPC`를 열고 root Selector의 자식 순서를 확인한다.

1. Alert branch 바로 다음, Patrol branch 바로 앞에 새 Sequence를 추가하고 `Mission Movement`로 알아보기 쉽게 표시한다.
2. Sequence에 Blackboard decorator를 추가한다.
3. Blackboard Key는 `bMissionMovementAllowed`, 조건은 `Is Set` 또는 `true`로 설정한다.
4. decorator의 `Observer Aborts`는 `Self`로 설정한다.
5. Sequence 자식으로 엔진 기본 `Move To` Task를 추가한다.
6. `Blackboard Key = MissionTargetLocation`으로 설정한다.
7. `Acceptable Radius = 100`으로 시작한다. 목적지 주변에서 멈추는 반경이며 실제 레벨 체감에 따라 후속 조정한다.
8. 기존 CombatState decorator와 기존 branch 순서는 그 외에는 변경하지 않는다.
9. Behavior Tree를 저장하고 compiler/error 표시가 없는지 확인한다.

현재는 Mission Resolver/Group dispatch가 없어 runtime에서 `SetMissionContext()`를 호출하지 않는다. 따라서 이 단위의 성공 기준은 테스트 3개 통과, 두 Blackboard key 존재, BT branch 배선과 저장 성공이다. 실제 관측점 이동은 다음 연결 단위에서 검증한다.

### 현재 통합 상태

- [x] Mission Context overlay와 validation 작성
- [x] Blackboard 투영 및 UnPossess 정리 작성
- [x] 전투 상태별 이동 허용 규칙 작성
- [x] 자동화 테스트 3개 작성
- [x] 사용자 코드 반영
- [x] Mission Overlay 테스트 3개 통과
- [x] Blackboard key 2개 추가
- [x] Mission Sequence/decorator/native Move To 배선

Unit 2는 **Integrated Complete** 상태다.

---

## Phase 5 Unit 3 — Observation Point Selector

### Codex가 구현한 것

- 배치 Actor와 미래 동적 후보가 공유할 `FObservationPointCandidate`
- Objective 연결과 도달 가능성을 hard constraint로 적용하는 순수 선택 규칙
- 최고 utility score 선택과 Point ID 기반 결정적 동점 처리
- 잘못된 Objective, 연결 후보 없음, 사용 가능한 후보 없음의 구조화 결과
- 자동화 테스트 3개

이 단위는 Blackboard, Behavior Tree, Level Asset을 수정하지 않는다. 새 reflection 타입도 없으며 Codex는 빌드를 실행하지 않았다.

### 사용자 검증

Live Coding으로 코드를 반영한 뒤 Output Log에서 실행한다.

```text
Automation RunTests Retry.Mission.ObservationSelector
```

기대 테스트:

- `Retry.Mission.ObservationSelector.ChoosesHighestUsableScore`
- `Retry.Mission.ObservationSelector.UsesStableIdTieBreak`
- `Retry.Mission.ObservationSelector.ReportsFailureReasons`

이번 테스트는 레벨의 `ReconObs_A1/A2`를 직접 읽지 않는다. 순수 규칙이므로 테스트 값 후보로 선택 동작을 검증한다. 실제 배치 Actor 수집과 NavMesh 도달 가능성 판정은 다음 Resolver 연결 단위에서 수행한다.

### 현재 통합 상태

- [x] 후보/결과 타입 작성
- [x] hard constraint와 utility 선택 규칙 작성
- [x] 결정적 동점 규칙 작성
- [x] 자동화 테스트 3개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] Observation Selector 테스트 3개 통과

Unit 3는 **Integrated Complete** 상태다.

---

## Phase 5 Unit 4 — Mission Resolver

### Codex가 구현한 것

- Assigned `Recon + Area` Command의 경계 검증
- Command 목표와 찾아온 Objective ID 일치 확인
- Observation Selector 결과를 `FMissionContext`로 변환
- Command ID, hard/soft constraint, Information Requirement 보존
- Selector의 상세 실패 outcome 전달
- Mission Resolver 자동화 테스트 3개

이 단위는 새 reflection 타입, Blackboard key, Behavior Tree node, Level Asset을 추가하지 않는다. Codex는 빌드를 실행하지 않았다.

### 사용자 검증

Live Coding으로 반영한 뒤 Output Log에서 실행한다.

```text
Automation RunTests Retry.Mission.Resolver
```

기대 테스트:

- `Retry.Mission.Resolver.BuildsReconMissionContext`
- `Retry.Mission.Resolver.RejectsWrongBoundaryInputs`
- `Retry.Mission.Resolver.PreservesSelectionFailure`

이번 테스트도 World와 Editor Asset을 열지 않는 순수 규칙 테스트다. 따라서 별도 에디터 작업은 없다.

### 현재 통합 상태

- [x] Resolver 결과와 실패 경계 작성
- [x] Assigned Recon Area 변환 작성
- [x] constraint/requirement 전달 작성
- [x] 자동화 테스트 3개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] Mission Resolver 테스트 3개 통과

Unit 4는 **Integrated Complete** 상태다.

---

## Phase 5 Unit 5 — Recon Mission World Adapter

### Codex가 구현한 것

- World의 Objective/Observation Marker 수집
- Objective 없음과 ID 중복의 구조화 실패
- UE Navigation의 valid non-partial path 판정
- 최단 Nav 경로 기반 baseline utility
- World marker 수집을 NavMesh 없이 검증할 path evaluator 경계
- World Adapter 자동화 테스트 3개

이 단위는 새 reflection 타입, Blackboard key, Behavior Tree node, Level Asset을 추가하지 않는다. Codex는 빌드를 실행하지 않았다.

### 사용자 검증

Live Coding으로 반영한 뒤 Output Log에서 실행한다.

```text
Automation RunTests Retry.Mission.WorldAdapter
```

기대 테스트:

- `Retry.Mission.WorldAdapter.SelectsShortestReachablePath`
- `Retry.Mission.WorldAdapter.ReportsObjectiveFailures`
- `Retry.Mission.WorldAdapter.PreservesNoReachableCandidate`

테스트는 Editor Preview World의 Marker와 주입된 path evaluator를 사용한다. 실제 `Lvl_TS_ReconSecure_001` NavMesh query는 다음 Group dispatch 단위에서 Mission Context를 배포한 뒤 PIE로 검증한다. 이번 단위에는 별도 에디터 Asset 작업이 없다.

### 미래 베이크 확장 기록

- 정적인 엄폐·가시성·고도·노출 데이터는 하나의 점수가 아닌 feature 채널로 Tactical Point/Grid Asset에 Editor bake 가능
- 런타임에는 작전 교리와 현재 Group Leader 성격을 feature 가중치로 적용하고 Nav path와 현재 위협을 동적으로 합성
- 현재 Selector/Resolver/Blackboard는 최종 Candidate 값을 소비하므로 구조 유지 가능
- Cell 크기, 방향 수, World Partition streaming 정책을 결정한 뒤 실제 Asset 타입 추가

지휘 우선순위는 `Hard Constraint > Command/Doctrine > Leader Personality > Point ID Tie-break`로 유지한다. Group Mission의 Observation Point는 Leader 성격으로 한 번 선택하고, 일반 Member 성격은 이후 개인 전투 판단에 사용한다.

### 현재 통합 상태

- [x] World marker 수집 작성
- [x] Objective 없음/중복 경계 작성
- [x] production Nav path 판정 작성
- [x] 최단 경로 baseline utility 작성
- [x] 자동화 테스트 3개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] World Adapter 테스트 3개 통과

Unit 5는 **Integrated Complete** 상태다.

---

## Phase 5 Unit 6 — Atomic Group Mission Dispatch

### Codex가 구현한 것

- Group Leader 위치와 Actor를 World Adapter의 Nav 시작점/context로 사용
- 죽은 Member는 제외하되 살아 있는 등록 Member 전원의 Controller와 `NPCDecisionComponent` 사전 검사
- 모든 수신자의 기존 Mission snapshot 저장 후 동일한 `FMissionContext` 원자적 배포
- Mission 거부 또는 Execution Log/status 전이 실패 시 모든 수신자의 이전 상태 복원
- 전원 배포 성공 후에만 Command를 `Assigned → Executing`으로 전이
- 성공한 수신자를 weak reference로 보관해 terminal 전이 때 정확한 Mission 대상 정리
- terminal Command, Scenario reset, Leader death에서 Group Mission 정리
- `ARetryNPCCharacter::OnDeath()`에서 실제 Leader death 경로 연결
- Group Mission Dispatch 자동화 테스트 3개

`DispatchCurrentReconMission()`이 새 `UFUNCTION`으로 추가됐다. Live Coding에서 함수가 Blueprint에 보이지 않으면 Editor를 닫고 사용자가 전체 빌드·재시작해야 한다. Codex는 빌드를 실행하지 않았다.

### 사용자 검증

코드를 반영한 뒤 Output Log에서 실행한다.

```text
Automation RunTests Retry.Mission.GroupDispatch
```

기대 테스트:

- `Retry.Mission.GroupDispatch.AppliesToAllBeforeExecuting`
- `Retry.Mission.GroupDispatch.RestoresWhenTransitionFails`
- `Retry.Mission.GroupDispatch.RejectsUnavailableRecipient`

첫 테스트는 두 수신자에게 Mission이 모두 들어간 뒤 Command가 `Executing`이 되는지 확인한다. 두 번째는 stale Run 때문에 마지막 상태 전이가 실패했을 때 기존 Mission/빈 상태가 각각 복구되는지 확인한다. 세 번째는 수신자 하나가 unavailable이면 정상 수신자도 전혀 변경하지 않는지 확인한다.

실제 `Lvl_TS_ReconSecure_001` NavMesh 이동은 다음 hardcoded Command 시작 경계를 연결한 뒤 PIE에서 검증한다. 이번 단위에는 Blueprint 또는 Level Asset 수정이 없다.

### 현재 통합 상태

- [x] Leader 기준 World resolution 작성
- [x] 생존 Member 전원 사전 검사 작성
- [x] Mission snapshot/rollback 작성
- [x] 성공 후 Executing 전이 작성
- [x] terminal/reset/Leader death 정리 작성
- [x] 자동화 테스트 3개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] Group Mission Dispatch 테스트 3개 통과

Unit 6는 **Integrated Complete** 상태다.

---

## Phase 5 Unit 7 — Scenario Opening Orders와 첫 End-to-End PIE

### Codex가 구현한 것

- `UScenarioDefinition::OpeningOrders` 명령 템플릿 배열
- 템플릿마다 새 runtime `CommandId`를 만들고 `Proposed` 상태로 시작하는 builder
- 기존 `FCommandValidator`를 재사용한 DataAsset 명령 validation
- 한 Group에 두 개의 동시 Opening Order가 들어가는 구성 거부
- Scenario 초기화 다음 tick에 Group 등록과 AI Possess를 기다린 뒤 명령 할당·원자적 Mission 배포
- Run/Definition이 바뀐 늦은 callback 거부와 UObject-bound timer delegate
- 실제로 선택된 Observation ID, 위치, 후보 수를 보여주는 로그
- Opening Order 자동화 테스트 3개

이 단위는 새 `UPROPERTY`를 추가한다. Live Coding에서 `Opening Orders`가 DataAsset Details에 나타나지 않으면 Editor를 닫고 사용자가 전체 빌드·재시작해야 한다. Codex는 빌드를 실행하지 않았다.

### 1. 자동화 테스트

Output Log에서 실행한다.

```text
Automation RunTests Retry.Scenario.OpeningOrders
```

기대 테스트:

- `Retry.Scenario.OpeningOrders.CreatesRuntimeIdentity`
- `Retry.Scenario.OpeningOrders.RejectsInvalidTemplate`
- `Retry.Scenario.OpeningOrders.RejectsDuplicateGroup`

### 2. ScenarioDefinition에 최초 HQ 명령 입력

`/Game/Scenarios/Definitions/DA_TS_ReconSecure_001`을 열고 `Opening Orders`에 element 하나를 추가한다.

- `Command Id`: 비워 둔다. Run 시작 때 자동 생성된다.
- `Parent Command Id`: 비워 둔다.
- `Issuer Id`: `HQ`
- `Assigned Group Id`: `A`
- `Verb`: `Recon`
- `Target Type`: `Area`
- `Target Id`: `ReconArea_A`
- `Target Location`: 기본값 유지
- `Priority`: `50`
- `Constraints`: 비움
- `Information Requirements`: 이번 이동 검증에서는 비움
- `Completion Criteria`: 기본값 유지
- `Status`: `Proposed`

DataAsset을 저장한다. `Default Launch Options > Auto Start`는 `true`를 유지한다.

### 3. 첫 End-to-End PIE 검증

1. `Lvl_TS_ReconSecure_001`에서 `Validate Scenario Setup`이 성공하는지 확인한다.
2. `Lvl_ScenarioMenu`를 열고 PIE를 시작한다.
3. `DA_TS_ReconSecure_001`을 선택해 Start한다.
4. Level 전환 후 다음 로그를 확인한다.

```text
[Scenario] Level initialized ... AutoStart:true
[Group:A] Recon Mission resolved. Observation:ReconObs_A1 또는 ReconObs_A2 ... Candidates:2
[Scenario] Opening Order executing. Group:A Command:<새 GUID> Recipients:2
```

5. Group A의 두 NPC가 선택된 같은 Observation Point 방향으로 이동하는지 확인한다.
6. 최단 **Nav 경로**가 더 짧은 Point가 선택되는지 확인한다. 직선거리만 비교하지 않는다.
7. 이동 중 적을 만나 전투 상태가 되면 Mission 이동이 잠시 중단되고, 전투가 끝나 `Idle/Patrol`로 돌아왔을 때 다시 이동하는지 확인한다.
8. Group B는 Opening Order를 받지 않고 기존 전투/Patrol 판단을 유지하는지 확인한다.
9. Restart하면 새 Run ID와 새 Command ID로 같은 흐름이 다시 시작되는지 확인한다.
10. Return to Menu가 정상이며 PIE 종료 중 timer/callback 오류가 없는지 확인한다.

### 현재 통합 상태

- [x] Opening Order template/builder 작성
- [x] validation과 Group 중복 경계 작성
- [x] 다음 tick Scenario 시작 연결 작성
- [x] 자동화 테스트 3개 작성
- [x] 변경 파일 정적 검사
- [x] 사용자 코드 반영
- [x] `DA_TS_ReconSecure_001` Opening Order 입력·저장
- [ ] Opening Order 테스트 3개 통과
- [x] Scenario validation과 Opening Order 실행 로그 확인
- [x] 실제 NavMesh 선택과 Group A Mission 이동 PIE 확인
- [ ] Restart/Return 수명 회귀 확인

Unit 7은 **Runtime Movement Verified / Feature Batch Verification Pending** 상태다. Opening Order 자동화 테스트와 Restart/Return 확인은 다음 Phase 5 기능 체크포인트의 일괄 검증 목록에 포함한다.

---

## Phase 5 기능 체크포인트 — Recon Observe → Report → Complete

상태: **Code Complete / User Batch Verification Pending**

이 체크포인트에서만 코드를 한 번 반영하고 아래 테스트를 순서대로 실행한다. 중간 Unit마다 다시 Live Coding하지 않는다. 새 `UENUM`, `USTRUCT`, `UCLASS`, `UPROPERTY`가 있으므로 Live Coding 반영이 불완전하면 Editor를 닫고 사용자가 전체 빌드·재시작한다. Codex는 빌드를 실행하지 않았다.

### 1. 자동화 테스트 전체 목록

아래 순서로 실행한다. 각 묶음이 모두 Success인지 확인한 뒤 다음 묶음으로 간다.

```text
Automation RunTests Retry.Scenario.OpeningOrders
Automation RunTests Retry.Operational.Report
Automation RunTests Retry.Operational.TeamMemory
Automation RunTests Retry.Command.ExecutionMonitor
Automation RunTests Retry.Mission.WorldAdapter
Automation RunTests Retry.Operational.ExecutionLog
Automation RunTests Retry.Mission.GroupDispatch
Automation RunTests Retry.Mission.Overlay
Automation RunTests Retry.Scenario.ExecutionLog
```

예상 테스트 28개:

1. `Retry.Scenario.OpeningOrders.CreatesRuntimeIdentity`
2. `Retry.Scenario.OpeningOrders.RejectsInvalidTemplate`
3. `Retry.Scenario.OpeningOrders.RejectsDuplicateGroup`
4. `Retry.Operational.Report.BuildsRequiredFacts`
5. `Retry.Operational.Report.ProvidesImplicitReconFact`
6. `Retry.Operational.Report.RejectsMismatchedMission`
7. `Retry.Operational.TeamMemory.GatesFactsOnReceive`
8. `Retry.Operational.TeamMemory.PartitionsAndDeduplicates`
9. `Retry.Operational.TeamMemory.ResetsRunState`
10. `Retry.Command.ExecutionMonitor.UsesHorizontalAndVerticalArrivalTolerances`
11. `Retry.Command.ExecutionMonitor.RequiresArrivalAndHold`
12. `Retry.Command.ExecutionMonitor.WaitsDuringCombat`
13. `Retry.Command.ExecutionMonitor.ReportsFailures`
14. `Retry.Mission.WorldAdapter.ProjectsMarkerToNavigationHeight`
15. `Retry.Mission.WorldAdapter.SelectsShortestReachablePath`
16. `Retry.Mission.WorldAdapter.ReportsObjectiveFailures`
17. `Retry.Mission.WorldAdapter.PreservesNoReachableCandidate`
18. `Retry.Operational.ExecutionLog.LinksFactReportAndCommandIds`
19. `Retry.Operational.ExecutionLog.RejectsStaleRun`
20. `Retry.Mission.GroupDispatch.AppliesToAllBeforeExecuting`
21. `Retry.Mission.GroupDispatch.RestoresWhenTransitionFails`
22. `Retry.Mission.GroupDispatch.RejectsUnavailableRecipient`
23. `Retry.Mission.Overlay.AcceptsAndClearsValidContext`
24. `Retry.Mission.Overlay.RejectsInvalidContext`
25. `Retry.Mission.Overlay.AllowsOnlyIdleAndPatrolMovement`
26. `Retry.Scenario.ExecutionLog.LinksRunCommandAndEventIds`
27. `Retry.Scenario.ExecutionLog.RejectsStaleRunWrites`
28. `Retry.Scenario.ExecutionLog.PreservesCompletedRuns`

### 2. 자동화 통과 후 DataAsset 통합

`DA_TS_ReconSecure_001 > Opening Orders[0]`에 다음을 추가한다.

- `Information Requirements`: element 하나
  - `Requirement Id`: `AreaObserved`
  - `Subject Id`: `ReconArea_A`
  - `Required`: true
- `Completion Criteria > Minimum Hold Seconds`: `2.0`
- `Completion Criteria > Timeout Seconds`: `120.0`

`Required Condition Ids`는 이번 Recon 기능에서 사용하지 않으므로 비워 둔다. 저장 후 `Lvl_TS_ReconSecure_001`의 `Validate Scenario Setup`이 성공하는지 확인한다.

### 3. PIE 실제 동작 검증

1. `Lvl_ScenarioMenu`에서 `DA_TS_ReconSecure_001`을 Start한다.
2. Group A가 선택된 Observation Point로 이동하는 기존 동작을 확인한다.
3. Leader가 반경 150 안에 들어온 뒤 전투 중이 아니라면 약 2초 후 다음 로그가 순서대로 나타나는지 확인한다.

```text
[TeamMemory:1] Recon Report received. Report:<GUID> Command:<GUID> Facts:1
[Group:A] Recon completed. Command:<같은 GUID> Report:<같은 Report GUID> Facts:1
```

4. 완료 후 Group A의 Mission 이동이 끝나고 기존 Patrol/전투 판단으로 복귀하는지 확인한다.
5. 관측점 근처에서 전투가 발생하면 전투 중에는 완료되지 않고, 전투가 끝난 뒤 hold를 다시 채워 완료되는지 확인한다.
6. Restart 후 새 Run ID, Command ID, Fact ID, Report ID로 다시 완료되는지 확인한다.
7. Return to Menu와 PIE 종료에서 timer, stale Run, WorldSubsystem 오류가 없는지 확인한다.
8. Group B의 기존 Patrol/전투와 기존 `Lvl_ThirdPerson` 전투가 회귀 없이 동작하는지 확인한다.

### 현재 체크리스트

- [x] Fact/Report 타입과 builder 작성
- [x] Team Operational Memory Received gate 작성
- [x] Recon completion/timeout monitor 작성
- [x] Group runtime observe/report/complete 연결
- [x] Execution Log Fact/Report ID 연결 작성
- [x] Scenario reset Team Memory 정리 작성
- [x] 신규 자동화 테스트 11개 작성
- [x] 회귀 테스트 12개 일괄 목록 작성
- [x] 변경 파일 정적 검사
- [ ] 사용자 코드 반영
- [x] 기존 자동화 테스트 23개 통과
- [ ] 도착 판정 회귀 테스트 1개 통과
- [ ] Nav 높이 투영을 포함한 World Adapter 테스트 4개 통과
- [ ] DataAsset requirement/hold/timeout 저장
- [ ] PIE Report Received와 Command Completed 확인
- [ ] Restart/Return 및 기존 전투 회귀 확인

이 기능 배치는 **Code Complete / User Batch Verification Pending** 상태다.

---

## Phase 0/1 — User Editor Verification Actions

Phase 0/1에서는 에셋을 생성하거나 수정하지 않는다. 아래는 사용자가 Unreal Editor에서 **열어 보고 확인할 항목만** 정리한 체크리스트다.

## Phase 2 — Implementation Plan

### Codex가 작성한 것

- `Generated/IMPLEMENTATION_PLAN.md`에 Phase 3~7의 아키텍처, 클래스 책임, 신규/수정 파일, 데이터 흐름, 테스트 및 롤백 전략을 작성했다.
- 레벨 고정 배치 재사용, 별도 `Lvl_ScenarioMenu`, Project Settings 기반 Scenario 등록, C++ 판단과 최소 Blackboard 실행 데이터, `uint8 TeamID`, World-scoped Team Memory 결정을 반영했다.

### 사용자 에디터 작업

사용자 에디터 작업 없음. Phase 2는 계획 확정 단계이며 Blueprint, DataAsset, Level, Project Settings를 수정하지 않는다.

### 검증 절차

1. `IMPLEMENTATION_PLAN.md`의 확정 결정과 신규/수정 파일 구분을 확인한다.
2. 후속 질문 게이트가 Phase별로 분리되었는지 확인한다.
3. 소스 코드와 `.uasset`이 Phase 2에서 변경되지 않았는지 확인한다.

### 현재 통합 상태

- [x] Phase 2 계획 문서 작성
- [x] 사용자 설계 결정 반영
- [x] 사용자 에디터 작업 없음
- [x] Implementation Not Started

분류:

- **확인된 사실**: 에셋 파일 존재, C++ 노출 property, config, 과거 로그로 확인된 범위
- **코드에 근거한 추론**: C++에 생성 경로가 없어 level 배치로 보이는 경우 등. Editor에서 확정 필요
- **현재 확인할 수 없는 항목**: `.uasset` 내부 저장값, graph, level instance reference, 현재 PIE 결과. 아래 절차의 직접 확인 대상

## 현재 통합 상태

- [x] **Code Analysis Complete**
- [x] **Build Verification Complete** — Development Editor
- [x] **Editor Verification Performed via Unreal MCP** — 2026-08-02
- [x] **Implementation Not Started**
- [x] BP/asset reference 확인
- [x] level placement 확인
- [x] current working tree PIE 확인

## 1. Maps & Modes / Game framework chain

### 확인 대상

- 타입/이름: Project Settings > Maps & Modes
- 예상 경로: `/Game/ThirdPerson/Lvl_ThirdPerson`, `/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode`
- 부모 클래스: `BP_ThirdPersonGameMode`의 부모가 `ARetryGameMode`인지 확인

### 확인할 프로퍼티 또는 참조

- Editor Startup Map, Game Default Map, Global Default GameMode
- GameInstance Class가 기본인지
- BP GameMode의 Default Pawn Class, Player Controller Class, HUD Class, Game State Class
- `Lvl_ThirdPerson` World Settings의 GameMode Override

### 확인할 레벨 및 배치 Actor

- `Lvl_ThirdPerson`을 연 상태에서 World Settings 확인

### 확인할 Blueprint 이벤트 또는 함수 연결

- `BP_ThirdPersonGameMode` 또는 Level Blueprint의 BeginPlay에서 widget, NPC, manager를 추가 생성하는지

### PIE 절차와 기대 결과

1. `Lvl_ThirdPerson`을 연다.
2. Output Log를 비운 뒤 PIE를 시작한다.
3. 로그의 `Game class is`가 의도한 GameMode인지 확인한다.

기대 결과: `BP_ThirdPersonGameMode_C`와 의도한 Pawn/Controller가 생성되고 시작 오류가 없다.

실패 시: World Settings override, Maps & Modes, BP compile errors, default class assignments를 확인한다.

코드만으로 확인할 수 없는 이유: default GameMode path는 config에서 확인되지만 BP 내부 class defaults와 per-level override는 `.uasset`에 저장된다.

## 2. NPC Character Blueprint

### 확인 대상

- 타입/이름: Character Blueprint `BP_RetryNPCCharacter`
- 경로: `/Game/AI/Blueprints/BP_RetryNPCCharacter`
- 부모 클래스: `ARetryNPCCharacter`

### 확인할 프로퍼티 또는 참조

- AI Controller Class = `BP_RetryNPCController`
- Auto Possess AI = Placed in World or Spawned(또는 의도한 동등값)
- TeamID, DefaultPersonality, bIsHighIntelligence
- DefaultWeapon/DefaultAmmo, StartingItems
- FloatingNameWidgetClass = `WBP_FloatingName`
- AIDebugWidgetClass = `WBP_AIDebug`
- DialogueWidgetClass = `WBP_Dialogue`
- Health/Combat/Inventory/Personality/Weapon/Memory/Dialog components 존재

### 확인할 레벨 및 배치 Actor

- `Lvl_ThirdPerson`에 배치된 네 NPC instance
- 각 instance의 NPCName, TeamID, MyGroup, bIsGroupLeader, PatrolPoints

### 확인할 Blueprint 이벤트 또는 함수 연결

- Construction Script/Event Graph에서 C++ BeginPlay/group registration을 중복 수행하거나 값을 덮어쓰는지

### PIE 절차와 기대 결과

1. 네 NPC 각각의 instance 값을 기록한다.
2. PIE 시작.
3. 네 번의 `[NPC] ... BeginPlay`와 controller BT start 로그를 확인한다.

기대 결과: 네 NPC가 의도한 controller에 possess되고 정확한 group/leader로 한 번씩 등록된다.

실패 시: AIControllerClass, Auto Possess AI, MyGroup soft/actor reference, group actor validity, BP compile status를 확인한다.

코드만으로 확인할 수 없는 이유: instance-only references와 BP class defaults는 C++에 없다.

## 3. NPC Controller / Behavior Tree

### 확인 대상

- 타입/이름: AIController Blueprint `BP_RetryNPCController`
- 경로: `/Game/AI/Blueprints/BP_RetryNPCController`
- 부모 클래스: `ARetryNPCController`
- 참조 에셋: `/Game/AI/BT_LowIntelNPC`

### 확인할 프로퍼티 또는 참조

- `BehaviorTreeAsset` = `BT_LowIntelNPC`
- Perception component/sight config의 BP overrides

### 확인할 레벨 및 배치 Actor

- 별도 controller actor 배치는 불필요해야 하며 Character possession으로 생성되는지 확인

### 확인할 Blueprint 이벤트 또는 함수 연결

- BP OnPossess가 parent call을 누락하지 않는지
- perception 이벤트를 BP가 별도로 처리/덮어쓰는지

### PIE 절차와 기대 결과

1. AI Debugger/Behavior Tree debugger에서 각 NPC를 선택한다.
2. `BT_LowIntelNPC` 실행과 `UBTService_Decision` 반복 tick을 확인한다.
3. target acquire/loss 때 blackboard 변화를 관찰한다.

기대 결과: possess 직후 BT 실행, CombatState 주기 갱신, target 사망 시 target clear.

실패 시: BehaviorTreeAsset, Auto Possess AI, controller class, service 배치, parent OnPossess를 확인한다.

코드만으로 확인할 수 없는 이유: 실제 BehaviorTreeAsset assignment와 graph는 Blueprint/Behavior Tree asset에 저장된다.

## 4. Blackboard와 CombatState enum

### 확인 대상

- Blackboard `/Game/AI/BB_NPC`
- Enum `/Game/AI/E_CombatState`
- native enum `ENPCCombatState` (`Source/Retry/NPCContext.h`)

### 확인할 프로퍼티 또는 참조

- `CombatState` key의 exact type
- native 순서와 asset enum 순서가 다음과 정확히 같은지:
  `Idle, Patrol, Alert, Search, Attack, TakeCover, Reload, Retreat, Hold, Suppress, Dead`
- 모든 C++ key 존재/type:
  `TargetActor(Object/Actor)`, `CombatState(Enum)`, `bAlerted`, `bCanSeeTarget`, `LastKnownEnemyLocation`, `CoverLocation`, `StimulusLocation`, `MoveDestination`, `PatrolIndex`, `DistanceToTarget`, `bIsLowHP`, `bIsOutOfAmmo`, `bIsReloading`, `Aggression`, `Fear`, `Trust`, `TrustBias`, `Loyalty`

### 확인할 레벨 및 배치 Actor

- 없음. AI debugger에서 runtime key 값 확인.

### 확인할 Blueprint 이벤트 또는 함수 연결

- BT decorators가 `CombatState` 값에 어떤 비교를 하는지 기록
- observer abort 설정과 branch 우선순위 기록

### PIE 절차와 기대 결과

1. AI debugger로 한 NPC blackboard를 연다.
2. 평시/적 발견/탄약 소진/엄폐/후퇴에서 enum 값과 branch를 비교한다.

기대 결과: C++ enum 이름과 BT branch 의미가 일치하며 missing key warning이 없다.

실패 시: key 이름 대소문자, type, BP enum ordinal, BT decorator value를 확인한다.

코드만으로 확인할 수 없는 이유: C++은 문자열 key와 ordinal만 쓰며 Blackboard schema/enum binding은 asset 내부다.

## 5. Behavior Tree graph와 Task 사용 현황

### 확인 대상

- Behavior Tree `/Game/AI/BT_LowIntelNPC`
- 부모/기반 Blackboard: `/Game/AI/BB_NPC`

### 확인할 프로퍼티 또는 참조

- root 또는 항상 실행되는 composite에 `BTService_Decision`이 붙어 있는지
- CombatState 11종 branch 중 실제 branch 수
- 다음 C++ Task 18종의 배치/미사용 여부를 각각 기록:
  AimAtTarget, CallOut, ChargeEnemy, CloseDistance, FireAtTarget, FireFromCover, LookAround, MoveToCover, MoveToLastKnown, MoveToPatrolPoint, MoveToTarget, Overwatch, Reload, Retreat, SearchArea, SelectCombatRoute, SuppressiveFire
- 각 movement task 뒤에 완료 확인 node가 있는지
- target loss/path failure decorator와 abort policy

### 확인할 레벨 및 배치 Actor

- `Lvl_ThirdPerson` NPC로 live debugging

### 확인할 Blueprint 이벤트 또는 함수 연결

- Blueprint Task/Service가 C++ 목록 외에 존재하는지

### PIE 절차와 기대 결과

1. 한 NPC의 active branch를 평시부터 전투 종료까지 추적한다.
2. target loss 후 Search/Patrol/Idle 복귀를 확인한다.
3. NavMesh 밖 목표 또는 차단 상황에서 branch가 정지하지 않는지 확인한다.

기대 결과: CombatState 변화가 올바른 branch abort/선택으로 이어지고 BT가 영구 InProgress에 갇히지 않는다.

실패 시: `AimAtTarget`/`MoveToCover` latent finish 부재, movement request result, decorator abort, missing BB key를 확인한다.

코드만으로 확인할 수 없는 이유: Task 클래스 코드는 읽을 수 있지만 실제 graph 배치와 composite semantics는 asset에 있다.

## 6. GroupManager와 group instance 연결

### 확인 대상

- Actor Blueprint `/Game/AI/Blueprints/BP_GroupManager`
- 부모 클래스: `AGroupManagerActor`
- 예상 level instances: Group A, Group B

### 확인할 프로퍼티 또는 참조

- 각 actor의 GroupID, GroupEmotionThreshold
- level NPC `MyGroup` references와 bIsGroupLeader
- 각 group에 leader 정확히 1명, member 총 2명인지

### 확인할 레벨 및 배치 Actor

- `Lvl_ThirdPerson` Outliner의 GroupManager 두 actor와 NPC 네 actor

### 확인할 Blueprint 이벤트 또는 함수 연결

- leader death가 `OnLeaderDied`를 호출하도록 BP에서 연결돼 있는지
- 별도 group creation/disband/event graph가 있는지

### PIE 절차와 기대 결과

1. PIE 시작 직후 Group A/B register 로그를 확인한다.
2. leader를 사망시킨다.
3. `OnLeaderDied` 로그와 `HoldFire, Weight=0` 전파 여부를 확인한다.

기대 결과: 중복 등록 없이 각 group 2명, leader death 처리 연결.

실패 시: MyGroup reference, leader flag, BP death event, C++ delegate 연결 부재를 확인한다.

코드만으로 확인할 수 없는 이유: C++ `ARetryNPCCharacter::OnDeath`는 `OnLeaderDied`를 호출하지 않으므로 BP 연결 여부가 유일한 미확인 경로다.

## 7. NavMesh / patrol / spawn

### 확인 대상

- `Lvl_ThirdPerson`
- Actor types: `NavMeshBoundsVolume`, `BP_PatrolPoint`, PlayerStart, 네 NPC, 두 GroupManager

### 확인할 프로퍼티 또는 참조

- NavMeshBoundsVolume extent와 모든 NPC/cover/search 영역 coverage
- NPC capsule/agent radius와 crowd navigation compatibility
- each PatrolPoints array의 valid index/order
- runtime spawned NPC class는 cheat manager에 설정됐는지

### 확인할 Blueprint 이벤트 또는 함수 연결

- Level Blueprint/initializer/spawner가 있는지. 현재 C++ scenario initializer는 없음.

### PIE 절차와 기대 결과

1. `P`로 navmesh visualization.
2. patrol NPC가 모든 point에 도달하는지 확인.
3. combat cover/search/retreat 이동 실패 로그와 정지 여부를 확인.

기대 결과: 이동 후보가 green nav area에 있고 path request가 정상 완료된다.

실패 시: NavMesh bounds, agent size, collision, zero vector keys, disconnected islands, patrol array null을 확인한다.

코드만으로 확인할 수 없는 이유: navigation data와 level geometry는 cooked/generated asset 상태다.

## 8. UI 및 fallback DataTable

### 확인 대상

- PlayerController BP `/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController`
- Widgets `/Game/UI/WBP_Inventory`, `WBP_Loot`, `WBP_FloatingName`, `WBP_AIDebug`, `WBP_Dialogue`
- DataTable `/Game/UI/DT_FallbackDialogue`

### 확인할 프로퍼티 또는 참조

- Inventory/Loot/Mobile widget classes와 Enhanced Input actions/mapping contexts
- NPC widget classes
- DataTable row struct = `FFallbackDialogueRow`, 각 `EPersonalityType` row 존재

### 확인할 레벨 및 배치 Actor

- PlayerController는 GameMode에서 지정; widget actor 배치 불필요

### 확인할 Blueprint 이벤트 또는 함수 연결

- Widget Blueprint가 native events/functions에 올바르게 binding되는지
- GameMode/Level Blueprint가 별도 HUD를 생성하는지

### PIE 절차와 기대 결과

1. inventory/loot UI가 기존대로 생성되고 toggle되는지 확인.
2. `ToggleAIDebug`와 high-intelligence dialogue를 확인.
3. LLM server를 끈 테스트는 사용자가 의도적으로 수행할 때만 진행하고 fallback dialogue를 확인.

기대 결과: missing class/table error 없이 기존 UI와 fallback이 표시된다.

실패 시: BP class assignment, DataTable row struct, hard path, bIsHighIntelligence를 확인한다.

코드만으로 확인할 수 없는 이유: widget class assignment, designer bindings, DataTable rows는 assets에 저장된다.

## 9. 현재 워킹트리 PIE 최종 절차

1. 위 1~8의 참조를 저장 변경 없이 확인한다.
2. Output Log를 비운다.
3. `Lvl_ThirdPerson` PIE를 시작한다.
4. GameMode, 네 BT start, Group A/B registration을 캡처한다.
5. 한 교전을 끝까지 관찰해 CombatState transition과 group/personal memory를 확인한다.
6. group threshold에 도달하면 LLM request/order flow를 확인한다. LLM 서버가 없으면 failure/fallback만 기록하고 성공으로 간주하지 않는다.
7. PIE 종료 후 crash/assert/access violation이 없는지 확인한다.
8. 결과와 로그 경로를 이 문서 checklist에 반영한다.

### 기대 결과

- 기존 전투/BT가 동작한다.
- Group A/B가 독립 등록된다.
- order response가 도착한 경우 `SetOrderForAll` 후 score/CombatState 변화가 관찰된다.
- target loss/사망 후 BT가 정상 상태로 복귀한다.

### 실패 시 우선 확인

- native/BP enum mismatch
- missing Blackboard key
- `BehaviorTreeAsset`/AIControllerClass/Auto Possess AI
- `MyGroup` null 또는 wrong group reference
- group-less allied witness null path
- latent BT task finish 부재
- LLM timeout 뒤 late callback

## 10. Phase 2 진입 전 사용자 확인 목록

- [x] Maps & Modes와 World Settings 기록
- [x] BP class parent/default reference 기록
- [x] `BB_NPC` key/type 전체 대조 — C++ 내부 판단 구조에 따른 의도된 최소 schema임을 사용자 확인
- [x] `E_CombatState` ordinal 대조 — 11개 표시 순서 및 BT 사용 ordinal 일치
- [x] `BT_LowIntelNPC` graph/task 사용표 작성
- [x] Group A/B와 네 NPC instance property 확인
- [x] NavMesh bounds와 초기 NPC 포함 여부 확인 — 실제 동적 path coverage 전체는 Phase 2/3 PIE에서 재검증
- [x] current working tree PIE baseline 실행 — 전투 흐름 통과, LLM 서버 미접속으로 order 적용은 미검증
- [x] packaging 검증 필요 여부 결정 — Phase 2 전에는 불필요, Scenario Loader가 들어가는 Phase 3 완료 게이트에서 수행
- [x] 확인 결과의 처리 방침 승인 — Blackboard는 현행 유지, BT abort 개선은 후속 작업

## 11. 2026-08-02 Unreal MCP 직접 검증 결과

### 검증 방식

- Unreal MCP의 Asset/Blueprint/Object/Scene/BehaviorTree/Editor/Logs toolset으로 현재 열린 에디터 세션을 읽었다.
- `.uasset`을 수정하거나 저장하지 않았다.
- `Lvl_ThirdPerson`에서 PIE를 직접 시작하고 종료했으며, 현재 세션 로그를 조회했다.

### 확인 완료 — Game Framework와 Blueprint defaults

- 현재 레벨: `/Game/ThirdPerson/Lvl_ThirdPerson`.
- World Settings `DefaultGameMode`: `BP_ThirdPersonGameMode_C`.
- `BP_ThirdPersonGameMode` parent: `/Script/Retry.RetryGameMode`.
- GameMode defaults:
  - Default Pawn: `BP_ThirdPersonCharacter_C`
  - Player Controller: `BP_ThirdPersonPlayerController_C`
  - HUD: native `AHUD`
  - GameState: native `AGameStateBase`
  - Seamless Travel: false
- `BP_ThirdPersonPlayerController` parent는 `ARetryPlayerController`이며 Inventory/Loot/Mobile widget과 Enhanced Input mapping이 실제 할당돼 있다.
- `BP_RetryNPCCharacter` parent는 `ARetryNPCCharacter`; `AIControllerClass=BP_RetryNPCController_C`, `AutoPossessAI=PlacedInWorldOrSpawned`, 세 widget class와 기본 weapon/ammo가 실제 할당돼 있다.
- `BP_RetryNPCController` parent는 `ARetryNPCController`; `BehaviorTreeAsset=BT_LowIntelNPC`가 실제 할당돼 있다.
- `BP_GroupManager` parent는 `AGroupManagerActor`다.
- 위 GameMode/NPC/NPCController/GroupManager Blueprint의 Construction Script와 EventGraph는 모두 비어 있다. 따라서 BP가 C++ BeginPlay를 중복 호출하지 않으며, 리더 사망을 `OnLeaderDied`로 연결하는 BP 경로도 없다.

### 확인 완료 — 레벨 인스턴스

| Actor | Team/Group | Leader | Patrol |
|---|---|---:|---|
| `기회주의자` | Team 1 / Group A | yes | 2 points |
| `지원가` (NPCName `아나`) | Team 1 / Group A | no | 2 points |
| `화난놈` | Team 2 / Group B | yes | none |
| `쫄보` | Team 2 / Group B | no | none |

- `BP_GroupManager_A`: GroupID `A`, threshold `0.6`.
- `BP_GroupManager_B`: GroupID `B`, threshold `0.6`.
- NavMeshBoundsVolume world bounds는 X/Y `-2000..2000`, Z `-500..500`; 네 NPC의 초기 X/Y 위치는 모두 bounds 안이다.
- NavMesh 생성 로그에는 serialized maxTiles 200과 calculated 75 불일치로 RecastNavMesh 재생성 경고가 있다. 초기 전투 이동은 실행됐지만 전체 작전 영역 path coverage가 검증됐다는 뜻은 아니다.

### 확인 완료 — Blackboard와 enum

실제 `BB_NPC` key는 정확히 다음 11개다.

```text
SelfActor, Aggression, Fear, Trust, TargetActor, PatrolIndex,
CoverLocation, LastKnownEnemyLocation, StimulusLocation, bAlerted, CombatState
```

- `TargetActor`/`SelfActor`: Object, base class `Actor`.
- `CoverLocation`/`LastKnownEnemyLocation`/`StimulusLocation`: Vector.
- `CombatState`: Enum `/Game/AI/E_CombatState`.
- enum editor 표시 순서는 C++과 동일하다: `Idle, Patrol, Alert, Search, Attack, TakeCover, Reload, Retreat, Hold, Suppress, Dead`.
- BT decorator의 실제 비교값도 사용 branch에서 C++ ordinal과 일치한다: Patrol 1, Alert 2, Search 3, Attack 4, TakeCover 5, Reload 6, Retreat 7, Suppress 9, Dead 10.

### 설계 확인 — C++ 내부 판단과 최소 Blackboard schema

다음 C++ 사용 key는 `BB_NPC`에 실제로 존재하지 않는다.

```text
bCanSeeTarget, MoveDestination, DistanceToTarget,
bIsLowHP, bIsOutOfAmmo, bIsReloading,
TrustBias, Loyalty
```

사용자 확인 결과, 기존 Blackboard 중심 판단에서 C++ 내부 판단으로 이전했고 Blackboard에는 상태와 현재 BT 실행에 필요한 값만 전달하도록 바꾼 구조다. 따라서 위 key가 asset에 없는 사실만으로 schema 결함이나 Phase 2 blocker로 취급하지 않는다.

Phase 2에서는 누락 key를 일괄 추가하지 않는다. 남아 있는 C++ write가 실제로 불필요한 legacy write인지 확인하고, 새 mission 기능이 소비하는 최소 key만 근거가 있을 때 추가한다.

### 확인 완료 — Behavior Tree graph

- Blackboard: `BB_NPC`.
- root: Selector; `BTService_Decision`은 root selector 범위에 있고 interval `0.1`, deviation `0.02`다.
- 실제 branch: Dead, Reload, Retreat, TakeCover, Attack, Suppress, Search, Alert, Patrol, Idle.
- `Hold` branch는 없다.
- 실제 task: Reload, Retreat, native MoveTo(CoverLocation), FireAtTarget, SuppressiveFire, MoveToLastKnown, Overwatch, SearchArea, TurnToStimulus, CallOut, MoveToPatrolPoint, LookAround, Wait.
- 모든 CombatState Blackboard decorator의 `FlowAbortMode`가 `None`이다. 상태가 바뀌어도 실행 중 branch를 observer abort하지 않으므로 latent/task lifecycle 위험이 분석 문서의 추측이 아니라 에셋에서 확인된 사실이다.
- 사용자 확인 결과 현재 구현을 변경하는 중인 알려진 과도기 상태다. 이번 Phase 2 진입을 막지 않고 후속 BT 정리 작업으로 추적한다.

### 현재 워킹트리 PIE 결과

PIE 시작 시각: 2026-08-02 19:19 KST 상당 (`Retry.log` UTC 표기 `10:19`).

- 네 NPC 모두 `BT_LowIntelNPC` 실행 확인.
- Group A/B 모두 leader 1 + member 1, 총 2명씩 한 번 등록.
- 실제 상태 전이: Idle→Reload, Reload→Patrol/Attack, Attack→TakeCover, 전투 종료 후 Reload/Idle 또는 Patrol.
- 사격, 피해, NPC 사망, 개인/그룹 memory 기록 확인.
- Group B threshold 초과 후 LLM group request가 생성됐다.
- localhost LLM 응답은 `bSuccess:0, RespValid:0`로 실패했고 기존 fallback이 실행됐다. 따라서 `SetOrderForAll` 이후 상태 변화는 이번 실행에서 검증하지 못했다.
- `LogBlueprint`, `LogBehaviorTree`, `LogAIModule` 오류는 없었고 PIE는 MCP로 정상 종료했다.

### Phase 2 진입 판정

**진입 가능(Ready), 아직 시작하지 않음**.

사용자 결정:

1. Blackboard는 C++ 내부 판단 후 상태 중심으로 전달하는 현행 구조를 유지한다. 미존재 key를 자동 추가하지 않는다.
2. `Observer Aborts=None`은 현재 전환 중인 알려진 상태이며 후속 BT 수정 작업으로 이관한다.

LLM 서버 성공 응답과 `SetOrderForAll` 적용은 Phase 2 계획 작성 자체의 필수 선행 조건은 아니지만, 기존 order adapter를 수정하기 전 회귀 테스트 항목으로 고정한다.

현재 상태는 **Phase 1 Editor Verification Complete / Implementation Not Started / Phase 2 Ready**다.

## 12. MCP 에디터 정리 결과

- enum 확인을 위해 열었던 `E_CombatState`는 최종 `GetOpenAssets=[]`로 닫힌 상태를 확인했다.
- 임시 Slate observer를 `Unobserve`했으며 최종 observer 목록은 `[]`다.
- PIE는 에이전트가 종료했다.
- 선택된 actor/asset은 모두 `[]`이며 최상위 창은 기존 `Retry - 언리얼 에디터` 하나뿐이다.
- 확인한 level/BB/BT/enum/BP asset의 dirty 상태는 모두 false였다.
- 이후 MCP 작업은 저장소 루트 `AGENTS.md`의 Unreal Editor MCP 작업 절차를 따른다.

## 13. LLM 요청 수명주기 수정 후 수동 검증

이 검증은 수정된 `UnrealEditor-Retry.dll`을 로드하도록 Unreal Editor를 완전히 종료하고 `RetryEditor Win64 Development` 빌드를 성공시킨 뒤 새로 실행해야 한다. Live Coding으로 대체하지 않는다.

### 재현 조건 검증

1. 로컬 LLM 서버를 실행하지 않는다.
2. `Lvl_TS_ReconSecure_001`에서 PIE를 시작하고 그룹 threshold가 LLM 요청을 만들 때까지 진행한다.
3. `[LLMQueue] ProcessRequest 시작 성공: 1` 직후 PIE를 중단한다.
4. editor가 유지되고 crash report가 생성되지 않는지 확인한다.
5. 로그에서 World cleanup 시 `[LLMQueue] 전환 정리 — 활성 요청 취소:1`이 한 번 기록되는지 확인한다.
6. PIE 종료 뒤 해당 요청의 fallback, group response 적용 또는 `ProcessNext()` 로그가 새로 기록되지 않는지 확인한다.

사용자 확인 결과, 요청 중 PIE 종료 시 활성 요청 취소 로그가 기록되고 editor가 유지됐다. 최초 crash 재현 경로는 통합 검증 완료다.

### 실행 중 연결 실패 검증

1. 로컬 LLM 서버를 끈 상태로 PIE를 유지한다.
2. 연결 실패 callback이 도착해도 editor가 유지되는지 확인한다.
3. 요청 하나당 callback, fallback, 다음 queue 진행이 각각 한 번만 발생하는지 확인한다.

### 지연 응답 검증

응답을 `TimeoutSeconds`보다 늦게 반환할 수 있는 테스트 서버가 있을 때 수행한다. timeout 뒤 늦은 응답이 게임 상태에 적용되지 않고, 다음 pending request의 timeout이나 완료를 방해하지 않아야 한다.
