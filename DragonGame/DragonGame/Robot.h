#pragma once

#include "Common.h"

// ============================================================
//  Robot.h  -  육면체 계층 구조 로봇 (담당: 안세현)
//
//  플레이어와 적이 같은 구조를 공유한다.
//
//  [다른 모듈과의 경계]
//   - Camera 는 RobotRootMatrix / RobotEyePos / RobotHeadPos 만 읽는다.
//   - 충돌 판정은 ComputeRobotParts 가 돌려주는 "관절 월드 행렬" 과
//     RobotSpec 의 치수만 보고 히트박스를 만든다.
//   => 그리기 코드와 판정 코드가 같은 값을 쓰게 되어 "판정 어긋남" 을 막는다.
// ============================================================

// ---- 로봇 치수와 색 (그리기와 히트박스가 함께 쓴다) ----
namespace RobotSpec
{
    const glm::vec3 torsoCenter{ 0.0f, 1.4f, 0.0f };
    const glm::vec3 torsoSize{ 0.7f, 1.0f, 0.4f };
    const glm::vec3 torsoColor{ 0.85f, 0.85f, 0.90f };

    const glm::vec3 headCenter{ 0.0f, 2.15f, 0.0f };
    const glm::vec3 headSize{ 0.5f, 0.5f, 0.5f };
    const glm::vec3 headColor{ 0.95f, 0.80f, 0.65f };

    // 머리 중심 기준. 0.325 = 머리 반 두께 0.25 + 코 반 두께 0.075
    const glm::vec3 noseOffset{ 0.0f, -0.03f, 0.325f };
    const glm::vec3 noseSize{ 0.12f, 0.12f, 0.15f };
    const glm::vec3 noseColor{ 0.90f, 0.25f, 0.15f };

    const float armLength = 0.8f;
    const float armThick = 0.18f;
    const glm::vec3 leftShoulder{ 0.47f, 1.85f, 0.0f };   // 어깨 = 팔의 회전 중심
    const glm::vec3 rightShoulder{ -0.47f, 1.85f, 0.0f };

    const float legLength = 0.9f;
    const float legThick = 0.24f;
    const glm::vec3 leftHip{ 0.18f, 0.9f, 0.0f };         // 엉덩이 = 다리의 회전 중심
    const glm::vec3 rightHip{ -0.18f, 0.9f, 0.0f };

    // 눈 = 1인칭 카메라가 놓일 자리 (머리 중심에서 살짝 앞/위)
    const glm::vec3 eyeOffset{ 0.0f, 0.08f, 0.26f };

    // 이동/충돌용 몸통 반지름 (벽에 박히지 않게 하는 데 쓴다)
    const float bodyRadius = 0.45f;
}

// ---- 파츠별 색 ----
// 치수는 플레이어와 적이 같고, 색만 다르게 해서 한눈에 구분한다.
struct RobotPalette
{
    glm::vec3 torso, head, nose;
    glm::vec3 leftArm, rightArm;
    glm::vec3 leftLeg, rightLeg;
};

const RobotPalette& PlayerPalette();
const RobotPalette& EnemyPalette();

// ---- 로봇 한 대의 상태 ----
struct Robot
{
    glm::vec3 pos{ 0.0f };          // 발바닥이 바닥(y=0)에 닿는 기준점
    float     yawDeg{ 0.0f };       // y축 회전 (0 = +z 를 바라봄)

    float velY{ 0.0f };             // 점프/낙하 속도
    bool  onGround{ true };

    float walkPhase{ 0.0f };        // 팔다리 스윙 위상 (계속 커진다)
    float walkAmount{ 0.0f };       // 0 = 정지, 1 = 전속 보행 (스윙 크기)

    int hp{ 100 };
    int maxHp{ 100 };

    bool IsAlive() const { return hp > 0; }
};

// 로봇을 이루는 파츠들의 "관절 월드 행렬" (이동 + 회전까지만. 스케일은 들어있지 않다)
// 스케일이 없기 때문에 이 행렬에 로컬 좌표를 곱하면 손끝/발끝 같은 지점을 바로 구할 수 있다.
struct RobotParts
{
    glm::mat4 root{ 1.0f };
    glm::mat4 torso{ 1.0f };
    glm::mat4 head{ 1.0f };
    glm::mat4 leftArm{ 1.0f }, rightArm{ 1.0f };    // 원점 = 어깨
    glm::mat4 leftLeg{ 1.0f }, rightLeg{ 1.0f };    // 원점 = 엉덩이
};

void ResetRobot(Robot& r, const glm::vec3& pos, float yawDeg);

// moveInput : x 좌우 / y 전후 (화면 기준, 길이 0~1)
// turnInput : 좌(-1) / 우(+1)
void UpdateRobotMotion(Robot& r, const glm::vec2& moveInput, float turnInput,
    bool jumpPressed, float dt);

RobotParts ComputeRobotParts(const Robot& r);
void       DrawRobot(const RobotParts& parts, const RobotPalette& palette);

// targetPos 를 향해 최대 turnSpeedDeg(초당) 만큼 천천히 돌아선다.
// 적이 플레이어를 바라보게 하는 데 쓴다. (제안서대로 AI 행동 트리는 만들지 않는다)
void FaceTowards(Robot& r, const glm::vec3& targetPos, float turnSpeedDeg, float dt);

// 두 로봇의 몸통이 겹치면 서로 반씩 밀어낸다. (공격 히트박스와는 별개인 몸통 충돌)
void ResolveBodyOverlap(Robot& a, Robot& b);

// 체력을 깎는다. 0 밑으로는 내려가지 않는다.
void ApplyDamage(Robot& r, int amount);

// ---- 다른 모듈이 쓰는 조회 함수 ----
glm::mat4 RobotRootMatrix(const Robot& r);
glm::vec3 RobotForward(const Robot& r);         // 바라보는 방향 (단위벡터)
glm::vec3 RobotEyePos(const RobotParts& parts); // 1인칭 카메라 위치
glm::vec3 RobotHeadPos(const RobotParts& parts);

// 공격 히트박스의 중심이 될 지점 (3단계 충돌 처리에서 사용)
glm::vec3 RobotHandPos(const RobotParts& parts, bool left);
glm::vec3 RobotFootPos(const RobotParts& parts, bool left);
