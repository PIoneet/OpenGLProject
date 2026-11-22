# CLAUDE.md — DragonGame (OpenGL 팀 프로젝트 / 20팀)

> 이 파일이 **이 저장소(openGL)의 영구 컨텍스트**입니다. 작업 시작 시 먼저 읽고, 결정/구조가 바뀌면 여기를 갱신합니다.
>
> ⚠️ 상위 폴더의 `C:\Users\sunho\HongLab\CLAUDE.md` 는 **Unity 게임엔진 기말과제** 문서입니다.
> 그 문서의 요구사항(Input System, NavMesh, 적 300마리, Ellen 등)은 **이 프로젝트와 무관**하니 적용하지 마십시오.

---

## 1. 프로젝트 정체

- **제목(가제):** 카메라 연출형 필살기 격투 게임
- **장르:** 3D 격투 액션 / **과제:** 3차원 상호작용 프로그램 만들기
- **팀:** 20팀 — 선홍재, 안세현 (제안서 제출 2026-11-09)
- **핵심 차별점:** 필살기를 쓰는 순간 카메라가 **3인칭 → 1인칭(플레이어 눈) → 맞는 상대 얼굴 앞 → 3인칭 복귀**
  로 부드럽게 전환된다. 이 카메라 연출 + 사운드로 타격감을 만드는 것이 이 프로젝트의 중심이다.

### 규모 (제안서 기준 — 임의로 늘리지 말 것)
- 스테이지는 **바닥 + 벽으로만 이루어진 연습장 하나**. 조명으로 음영만 넣는다.
  크기는 22 x 22 x 8 (`Common.h`). 처음엔 12 x 12 였는데 3인칭 카메라를 뒤로 뺄 자리가 없어 넓혔다.
- 캐릭터는 **OBJ 로딩 없이 육면체 조합**으로 만든 인간형 로봇. (여유 있으면 텍스처 매핑)
- 적은 **AI 행동 트리 없음**. 필살기를 시험해 볼 수 있는 조건부 상황만 만든다.
- 여유 있으면 서버 1대1 대전으로 확장 (선택).

### 필살기 발동 조건표 (제안서)
| # | 조건 | 필살기 |
|---|------|--------|
| 1 | 상대가 땅에 쓰러져 있을 때 | 다리로 얼굴을 밟는 액션 |
| 2 | 상대를 잡고 있을 때 | 땅으로 엎어치는 액션 |
| 3 | 상대를 잡고 && 인근에 벽이 있을 때 | 상대 머리를 벽에 내리꽂는 액션 |
| 4 | 상대 공격을 회피했을 때 | 상대 팔을 잡고 다리로 꺾는 액션 |
| 5 | (추가 예정) | |

### 종료 조건
적의 체력이 0이 되면 종료.

---

## 2. 역할 분담 (제안서 3번)

| 담당 | 영역 | 주로 건드리는 파일 |
|------|------|--------------------|
| **선홍재** | 필살기 **카메라 연출**, 충돌/히트박스 | `Camera.h/.cpp` |
| **안세현** | 로봇 **모델링·이동·공격 모션**, 충돌/히트박스 | `Robot.h/.cpp` |
| **공동** | 히트박스 설계, 필살기 조건↔모션↔카메라 연결 | `main.cpp`, `Stage.*` |

**충돌(코드 머지) 방지 규칙:** 각자 자기 담당 파일만 수정한다. 공용 파일(`main.cpp`, `Common.h`,
`Renderer.*`)을 고쳐야 하면 먼저 상의한다. 모듈 간 통신은 아래 "모듈 경계"의 함수들로만 한다.

---

## 3. 빌드 환경

- **경로:** `C:\Users\sunho\HongLab\openGL` (저장소 루트) / 솔루션 `DragonGame\DragonGame.sln`
- **Visual Studio 2022 Community**, 툴셋 v143, **x64 전용** (Win32 설정은 라이브러리가 안 붙어 있음)
- **OpenGL 3.3 Core Profile**

### 의존성이 설치된 위치 (팀원 세팅 시 중요)
| 라이브러리 | 위치 | 비고 |
|---|---|---|
| GLEW, GLFW **헤더** | `C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\GL\` | 수동 복사 |
| GLEW, GLFW **lib** | `C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64\` | 수동 복사 |
| **GLM** | `C:\Users\sunho\vcpkg\installed\x64-windows\include\glm` | **vcpkg** |

> ⚠️ GLM 은 vcpkg 의 **user-wide MSBuild 통합**(`vcpkg integrate install`)으로 자동 주입된다.
> 그래서 `.vcxproj` 에 GLM 경로가 적혀 있지 않고, vcvars 환경의 `cl.exe` 로 직접 컴파일하면 GLM 을 못 찾는다.
> **다른 PC에서 빌드하려면** `vcpkg install glm:x64-windows` + `vcpkg integrate install` 가 필요하다.

### 빌드 / 실행
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" .\DragonGame\DragonGame\DragonGame.vcxproj -p:Configuration=Debug -p:Platform=x64
```
- 실행 시 **작업 디렉터리가 `VS.glsl` / `PS.glsl` 이 있는 폴더**여야 한다 (셰이더를 상대 경로로 읽음).
  VS 에서 F5 로 실행하면 자동으로 맞는다.
- `LNK4098 MSVCRT 충돌` 경고는 `glfw3.lib` 가 /MT 로 빌드된 것 때문. 동작에는 문제 없음.

---

## 4. 파일 구조와 모듈 경계

```
DragonGame/DragonGame/
  main.cpp      초기화 + 메인 루프만. 게임 로직은 넣지 않는다.
  Common.h      라이브러리 include 순서, 연습장 치수(roomW/D/H), SmoothFactor
  Mesh.h/.cpp   Vertex(위치+법선), 육면체/평면 정점 데이터, VAO/VBO
  Renderer.h/.cpp  셰이더 + uniform + DrawCube/DrawWall/DrawMesh
  Input.h/.cpp  키 폴링 -> InputState
  Robot.h/.cpp  로봇 상태/치수/모션/그리기        (안세현)
  Camera.h/.cpp 시점 모드와 보간                  (선홍재)
  Stage.h/.cpp  연습장 그리기, 공간 판정, 광원
  Hud.h/.cpp    화면 고정 UI (체력 바)
  Screenshot.h/.cpp  화면을 BMP 로 저장 (보고서용)
  VS.glsl / PS.glsl
```

### 모듈 경계 (이 함수들로만 소통한다)
- **Robot → Camera:** `RobotRootMatrix`, `RobotForward`, `RobotEyePos`, `RobotHeadPos`
  (카메라는 로봇 상태를 **읽기만** 한다)
- **Robot → 충돌:** `ComputeRobotParts(robot)` 가 돌려주는 **관절 월드 행렬** + `RobotSpec` 의 치수.
  `RobotHandPos` / `RobotFootPos` 로 손끝·발끝 월드 좌표를 얻는다.
  → **그리기와 판정이 같은 행렬을 쓰므로 "판정 어긋남" 이 생기지 않는다.**
- **Robot/Camera/Stage → 화면:** `Renderer.h` 의 `DrawCube` / `DrawWall` 만 호출. OpenGL 직접 호출 금지.

### `RobotParts` 가 스케일을 포함하지 않는 이유
관절 행렬에 스케일을 넣으면 그 행렬로 손끝 좌표를 구할 수 없다. 그래서 `RobotParts` 는
**이동+회전까지만** 담고, 큐브 크기는 `DrawRobot` 에서 마지막에 곱한다.

---

## 5. 코딩 규칙 / 주의사항

- **입력:** 이동처럼 계속 눌리는 입력은 `glfwGetKey` **폴링**. `glfwSetCharCallback` 은 OS 키 반복에
  의존해 끊기므로 쓰지 않는다. 점프/필살기처럼 한 번만 반응할 입력은 `Input.cpp` 의 **엣지 검출**을 쓴다.
- **시간:** 모든 움직임은 `dt` 를 곱한다. `dt` 는 0.05초로 상한을 둔다(창 끌기 대응).
- **보간:** 카메라 등 "따라가는" 값은 `SmoothFactor(lambda, dt)` 를 쓴다 (프레임레이트 독립).
- **변환 순서:** `translate → rotate → scale` 순으로 곱한다. 회전보다 스케일을 먼저 하면
  로컬 축이 아니라 월드 축으로 늘어나서 모양이 깨진다.
- **법선:** 팔다리처럼 **비균등 스케일**이 들어가면 `model` 을 법선에 그대로 쓸 수 없다.
  `Renderer::DrawMesh` 가 `transpose(inverse(mat3(model)))` 를 `normalMat` 으로 넘겨 준다.
- **조명:** 점광원 **하나** + **반구 환경광**. 천장 광원 하나만 쓰면 로봇의 앞/옆면처럼
  수직인 면에 빛이 거의 닿지 않아 캐릭터가 통째로 어둡게 뭉갠다. 그래서 환경광을
  "위를 보는 면 / 아래를 보는 면" 두 색으로 나눠 섞어(`N.y` 기준) 수직면도 살아나게 했다.
  광원도 방 정중앙이 아니라 살짝 앞(-z)에 두어 마주 선 두 로봇의 앞면에 빛이 걸리게 했다.
- **UI:** 체력 바처럼 화면에 고정되는 것은 `BeginUI` ~ `EndUI` 사이에서 그린다.
  직교 투영 + 조명 끔(`uUnlit`) + 깊이 테스트/컬링 끔. **반드시 3D 를 다 그린 뒤에** 호출한다.
- **컬링:** 앞면 = CCW. 벽은 **안쪽이 앞면**이 되도록 돌려 놓았다.
  → 카메라가 방 밖으로 나가면 벽이 사라져 보인다. 그래서 `ClampCameraInsideRoom` 으로 카메라를 방 안에 묶는다.
  (로봇은 `ClampInsideRoom` — y 는 건드리지 않는다. 점프 처리와 겹치면 안 되므로)
- **한글 주석:** 모든 설정에 `/utf-8` 을 켜 두었다. 파일은 **UTF-8** 로 저장한다.
- 애니메이션-게임로직 동기화는 **모션 진행도(normalizedTime 역할의 phase)** 를 상태머신이 들고,
  정해진 지점에서 판정/카메라 전환을 트리거하는 방식으로 한다.

---

## 6. 조작법 (현재 구현)

| 키 | 동작 |
|---|---|
| `W` `A` `S` `D` | 이동 (전진 / 좌 / 후진 / 우) |
| `←` `→` | 방향 회전 |
| `Space` | 점프 |
| `K` | 필살기 (구현 예정) |
| `F12` | 스크린샷 저장 (`shot_0001.bmp` …) |
| `G` / `H` | **[임시]** 플레이어 / 적 체력 -10 — 3단계에서 공격 판정이 들어오면 제거 |
| `I` | 초기 상태로 리셋 |
| `Q` / `ESC` | 종료 |

실행 인자 `--shot <파일>` 을 주면 30프레임 뒤 자동으로 저장하고 종료한다.
화면을 띄워 놓고 보지 않아도 렌더링 결과를 확인할 수 있어, 보고서 캡처를 뽑을 때도 쓴다.

---

## 7. 개발 일정과 진행 상황

### 1단계 — 기본 렌더링 / 입력 / 연습장 / 로봇 모델링
- [x] 창 + 셰이더 + 깊이 테스트 + 면 컬링
- [x] 연습장 (바닥·벽4·천장)
- [x] 육면체 계층 구조 로봇 (몸통/머리/코/팔2/다리2)
- [x] 입력 폴링 — 이동 + yaw 회전 + 점프(중력)
- [x] 보행 스윙 (멈출 때 부드럽게 감쇠)
- [x] 법선 + 점광원 1개 (ambient / diffuse / specular / 거리 감쇠)
- [x] 3인칭 추적 카메라 + 지수 보간 + 방 안으로 클램프
- [x] 모듈 분리 (Mesh / Renderer / Input / Robot / Camera / Stage)

### 2단계 — 적 로봇
- [x] 적 로봇 1체 추가 (`Robot` 구조 재사용), 마주 보고 배치
- [x] 적이 플레이어 쪽으로 천천히 돌아섬 (`FaceTowards`) — 행동 트리 없음
- [x] 몸통끼리 밀어내기 (`ResolveBodyOverlap`)
- [x] 플레이어/적 팔레트 분리 (`PlayerPalette` / `EnemyPalette`)
- [x] 체력 바 HUD (좌=플레이어, 우=적, 30% 이하 경고색)
- [x] 스크린샷 기능 (F12 / `--shot`)

### 3단계 — 공격 / 충돌
- [ ] 공격 모션 상태머신 (펀치 / 킥)
- [ ] 히트박스 (손끝·발끝 월드 좌표 기준) + 디버그 와이어 표시
- [ ] 피격 판정 → 체력 감소, 피격 반응 모션

### 4단계 — 필살기 (핵심)
- [ ] 필살기 발동 조건 판정 (쓰러짐 / 잡기 / 벽 근접 / 회피 성공)
- [ ] 필살기 모션 (조건표 1~4)
- [ ] **카메라 연출**: `CameraMode::FirstPerson` → `VictimFace` → `Follow` 복귀, 구간별 보간
- [ ] 타격 사운드

### 5단계 — 마무리
- [ ] 체력 0 종료 처리
- [ ] 테스트 / 오류 수정
- [ ] 보고서

---

## 8. 보고서 체크리스트

- 목표 / 가정 / 실행 결과 스크린샷 / 조작법
- **자료구조:** `Robot`, `RobotParts`(관절 월드 행렬), `InputState`, `Camera`
- **알고리즘:** 계층 변환 누적, 히트박스 판정, 카메라 모드 전환과 보간(`SmoothFactor`)
- **그래픽스 기술:** 모델링 변환(T·R·S)과 계층 구조, 뷰/투영 변환과 `lookAt`,
  조명 계산(법선 행렬의 필요성 포함), 깊이 테스트, 면 컬링
- **외부 에셋·라이브러리 출처 명시 (필수)**
  | 항목 | 출처 |
  |---|---|
  | GLFW | https://www.glfw.org/ |
  | GLEW | https://glew.sourceforge.net/ |
  | GLM | https://github.com/g-truc/glm (vcpkg) |
  | 참고 영상 (연출 레퍼런스) | https://youtu.be/kQk_jNDD6yM |
  | (추가 시 계속 기재) | |
