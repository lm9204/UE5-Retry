# Phase 0 — Baseline Status

검증일: 2026-08-02 (Asia/Seoul)  
기준 브랜치/커밋: `main` / `beffbf9`  
중요: 작업 트리는 검증 시작 전부터 수정·추가·삭제 파일이 다수 존재했다. 이 문서는 커밋 상태가 아니라 **현재 워킹트리**를 기준으로 한다.

경로 주의: `02_CODEBASE_ANALYSIS.md`와 `05_WORK_ORDER.md` 본문은 일부 산출물을 `Docs/LLM_Command_System/Generated`로 적지만, `06_EDITOR_INTEGRATION.md`와 이번 사용자 지시는 `Docs/LLM_Command_System_Docs/Generated`를 명시한다. 이번 산출물은 후자를 권위 경로로 사용했다.

## 완료 상태

- **Code Analysis Complete**
- **Build Verification Complete** — `RetryEditor Win64 Development` 컴파일 기준
- **Editor Verification Complete** — Unreal MCP로 현재 워킹트리 PIE 및 에셋 연결 직접 확인
- **Implementation Not Started**

## 확인된 사실

### 프로젝트 및 빌드 구성

- `Retry.uproject`의 EngineAssociation GUID `{49272611-4066-C81C-6B46-9590BBDB8BD1}`는 레지스트리에서 `D:\UE_5.8`에 연결된다. 현재 에디터 로그의 실제 버전은 `5.8.1-56057345`다.
- `Retry.uproject`는 Runtime 모듈 `Retry` 하나와 `ModelContextProtocol`, `AllToolsets` 등 활성 plugin을 선언한다.
- `Source/Retry.Target.cs`는 Game target, `Source/RetryEditor.Target.cs`는 Editor target이다. 둘 다 `BuildSettingsVersion.V7`, `EngineIncludeOrderVersion.Unreal5_8`, 모듈 `Retry`를 사용한다.
- `Source/Retry/Retry.Build.cs`의 주요 의존성은 `AIModule`, `NavigationSystem`, `HTTP`, `Json`, `JsonUtilities`, `UMG`, `Slate`, `StateTreeModule`, `GameplayStateTreeModule`, `EnhancedInput`이다.
- Windows SDK `10.0.22621.0`, Visual Studio 2022 toolchain `14.44.35228`이 UnrealBuildTool에서 인식됐다.

### 실제 빌드 검증

실행한 명령:

```powershell
& 'D:\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  RetryEditor Win64 Development `
  'C:\Projects\UnrealProjects\Retry 5.8\Retry.uproject' `
  -WaitMutex -NoHotReloadFromIDE -NoUBA `
  '-Log=C:\Projects\UnrealProjects\Retry 5.8\Saved\Logs\CodexPhase1BuildNoUBA.log'
```

결과:

```text
Result: Succeeded
Total execution time: 3.08 seconds
```

- 성공 로그: `Saved/Logs/CodexPhase1BuildNoUBA.log`
- 최초 문서의 잘못된 `D:\UE_5.6` 경로로 실행한 시도는 Target의 `V7`/`Unreal5_8`을 지원하지 않아 RulesError가 났다. 프로젝트 GUID와 레지스트리를 대조해 `D:\UE_5.8`이 권위 엔진임을 확인한 뒤 위 명령으로 재검증했으며, 5.6 결과는 기준 상태에서 제외했다.
- 빌드는 UHT timestamp 확인과 `RetryEditor.target` metadata 갱신 후 성공했다. 강제 clean rebuild는 아니다.
- Game target, cook, package, Shipping build는 실행하지 않았다.

### 실행 기준 증거

- `Config/DefaultEngine.ini`의 editor/game 기본 맵은 `/Game/ThirdPerson/Lvl_ThirdPerson`, global default GameMode는 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode`다.
- 최신 `Saved/Logs/Retry.log`는 2026-08-02 UE `5.8.1-56057345` 에디터 기동과 이번 MCP PIE(`UEDPIE_0_Lvl_ThirdPerson`) 기록을 포함한다.
- 이전 로그 `Saved/Logs/Retry-backup-2026.08.01-12.35.25.log`에는 다음 실행 증거가 있다.
  - `Lvl_ThirdPerson` PIE world 생성
  - 실제 GameMode `BP_ThirdPersonGameMode_C`
  - 네 NPC가 `BT_LowIntelNPC` 실행
  - Group A/B 각각 리더 1명, member array 총 2명 등록
  - 전투 시작/아군 사망 그룹 메모리 생성
  - 그룹 감정 임계값 도달, LLM 그룹 요청, 정상 HTTP 응답, `SetOrderForAll` 전파
- 위 기록은 **2026-08-01 당시 실행 증거**이며 2026-08-02 현재 워킹트리의 새 PIE 통과를 의미하지 않는다.
- 더 오래된 `Saved/Logs/Retry-backup-2026.08.01-05.19.50.log`에는 `Cast of Object /Script/CoreUObject.Default__Object to Actor failed` fatal 기록이 있다. 이후 로그에서 PIE 및 LLM 흐름이 실행된 사실은 확인되지만, 원인과 수정 커밋은 이 분석만으로 확정할 수 없다.
- 에디터 시작 시 `LogAutomationTest: Error: Condition failed` 3건이 반복된다. 프로젝트 자동화 테스트 실패인지 엔진/플러그인 자체 점검 메시지인지 현재 로그 조각만으로 확정할 수 없다.

## 코드에 근거한 추론

- 현재 Editor target이 up-to-date로 성공했으므로 UHT와 C++ 산출물은 현재 입력 타임스탬프 기준 일치한다. 다만 강제 clean rebuild를 수행한 것은 아니다.
- 2026-08-01 로그의 동일 레벨·동일 네 NPC 실행은 현재 기본 테스트 장면이 `Lvl_ThirdPerson`임을 강하게 뒷받침한다.
- 현재 source에는 자동 시나리오 초기화기가 없으므로 로그에 나타난 Group/NPC는 레벨 배치와 Blueprint 인스턴스 참조에 의존할 가능성이 높다.

## 후속으로 남은 미확인 항목

- NavMesh bounds 밖을 포함한 전체 작전 영역의 동적 pathfinding 성공 범위
- target loss 이후 Search/Patrol 복귀와 모든 latent task 종료의 장시간 실행 결과
- localhost LLM 서버 정상 응답 시 `SetOrderForAll` 이후 score/CombatState 변화
- 현재 패키징 목록, cook 포함 map, Game/Shipping/package 성공 여부

## 기준 상태 판정

| 구분 | 상태 | 근거/제한 |
|---|---|---|
| Development Editor 컴파일 | Complete | UBT `Result: Succeeded`, up-to-date |
| Editor 기동 | Complete | 2026-08-02 UE 5.8.1 현재 세션 |
| PIE 실행 이력 | Confirmed | 2026-08-01 과거 로그 + 2026-08-02 MCP 실행 |
| 현재 워킹트리 PIE | Complete (baseline) | MCP 실행: BT/그룹/전투/메모리 확인, 정상 종료 |
| Game/Shipping build | Incomplete | 미실행 |
| Cook/Package | Incomplete | 미실행 |
| 구현 | Not Started | 문서만 생성 |

## Phase 2 전 남은 결정

에디터 확인 작업은 MCP로 완료했다. 사용자는 Blackboard가 C++ 내부 판단 후 상태 중심으로 전달하는 의도된 구조임을 확인했고, BT observer abort 정리는 현재 전환 작업 이후로 이관했다. Phase 2 진입 가능 상태이며 Game/Shipping/package 검증은 Phase 3 완료 게이트로 이관한다.

## Unreal MCP 후속 검증 갱신 — 2026-08-02

기존 미확인 항목 중 에디터로 확인 가능한 항목은 Unreal MCP 직접 검증으로 해소했다. 상세 원시 결과와 체크리스트는 `EDITOR_ACTIONS.md` 11절에 기록했다.

- GameMode/Controller/Pawn/HUD/GameState, NPC/AIController/BT 기본 참조 확인 완료.
- Group A/B 및 네 NPC의 Team/Group/Leader/Patrol 인스턴스 값 확인 완료.
- `BB_NPC` key/type와 `E_CombatState` 순서 확인 완료.
- `BT_LowIntelNPC` full node 목록과 decorator abort 설정 확인 완료.
- 현재 워킹트리 PIE에서 네 BT, 그룹 등록, 상태 전이, 교전, 메모리, LLM 요청 실패 폴백을 확인하고 정상 종료.
- 확인된 구조: Blackboard는 C++ 내부 판단 전환에 따라 최소 key만 유지한다. 모든 state decorator의 Observer Aborts가 `None`인 상태는 알려진 후속 BT 수정 항목이다.
- LLM 서버가 응답하지 않아 `SetOrderForAll` 성공 경로는 이번 baseline에서 미검증이다.
- Game/Shipping build와 package/cook은 여전히 미실행이며, packaging은 Phase 3 Scenario Loader 완료 게이트에서 수행하기로 결정했다.
