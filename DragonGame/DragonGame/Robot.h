#pragma once

#include "Common.h"

// ============================================================
//  Robot.h  -  다관절 육면체 계층 구조 로봇 (담당: 안세현)
// ============================================================

namespace RobotSpec
{
    const glm::vec3 torsoCenter{ 0.0f, 1.4f, 0.0f };
    const glm::vec3 torsoSize{ 0.7f, 1.0f, 0.4f };
    const glm::vec3 torsoColor{ 0.85f, 0.85f, 0.90f };

    const glm::vec3 headCenter{ 0.0f, 2.15f, 0.0f };
    const glm::vec3 headSize{ 0.5f, 0.5f, 0.5f };
    const glm::vec3 headColor{ 0.95f, 0.80f, 0.65f };

    const glm::vec3 noseOffset{ 0.0f, -0.03f, 0.325f };
    const glm::vec3 noseSize{ 0.12f, 0.12f, 0.15f };
    const glm::vec3 noseColor{ 0.90f, 0.25f, 0.15f };

    // 기존 치수 유지 + 상완/하완/주먹 다관절 세부 치수 (합계 ≈ 0.8f)
    const float armLength = 0.8f;
    const float armThick = 0.18f;
    const float upperArmLen = 0.36f;
    const float lowerArmLen = 0.34f;
    const float fistSize = 0.20f;
    const glm::vec3 leftShoulder{ 0.47f, 1.85f, 0.0f };   // +X = 왼팔
    const glm::vec3 rightShoulder{ -0.47f, 1.85f, 0.0f }; // -X = 오른팔

    // 기존 치수 유지 + 허벅지/정강이/발 다관절 세부 치수 (0.4 + 0.4 + 0.1 = 0.9f)
    const float legLength = 0.9f;
    const float legThick = 0.24f;
    const float thighLen = 0.40f;
    const float calfLen = 0.40f;
    const float footHeight = 0.10f;
    const glm::vec3 footSize{ 0.24f, 0.10f, 0.36f };
    const glm::vec3 leftHip{ 0.18f, 0.9f, 0.0f };         // +X = 왼다리
    const glm::vec3 rightHip{ -0.18f, 0.9f, 0.0f };       // -X = 오른다리

    const glm::vec3 eyeOffset{ 0.0f, 0.08f, 0.26f };
    const float bodyRadius = 0.45f;
}

struct RobotPalette
{
    glm::vec3 torso, head, nose;
    glm::vec3 leftArm, rightArm;
    glm::vec3 leftLeg, rightLeg;
};

const RobotPalette& PlayerPalette();
const RobotPalette& EnemyPalette();

// 로봇 액션 상태 머신
enum class RobotActionState
{
    IdleMove,   // 기본 가드 / 보행 / 점프
    Dodge,      // 회피 스텝 (Left Shift)
    Attack,     // 기본 공격 콤보 (J: 펀치 3단, L: 킥 2단)
    Hit,        // 피격 (추후 충돌 연동)
    Down,       // 쓰러짐 (추후 필살기 조건 1)
    Grab,       // 잡기 (추후 필살기 조건 2, 3)
    Ultimate    // 필살기 (추후 카메라 연동)
};

struct Robot
{
    glm::vec3 pos{ 0.0f };          // 발바닥이 바닥(y=0)에 닿는 기준점
    float     yawDeg{ 0.0f };       // y축 회전 (0 = +z 를 바라봄)

    float velY{ 0.0f };
    bool  onGround{ true };

    float idlePhase{ 0.0f };        // 기본 가드 숨쉬기 위상
    float walkPhase{ 0.0f };        // 보행 스윙 위상
    float walkAmount{ 0.0f };       // 0 = 정지, 1 = 전속 보행

    int hp{ 100 };
    int maxHp{ 100 };

    // ---- 상태 머신 & 공격 콤보 상태 ----
    RobotActionState state{ RobotActionState::IdleMove };
    float stateTimer{ 0.0f };
    float stateDuration{ 0.0f };

    int   attackType{ 0 };          // 1: 펀치, 2: 킥
    int   comboStep{ 0 };           // 펀치 1~3, 킥 1~2
    bool  comboQueued{ false };     // 다음 콤보 선입력 예약
    float comboWindowTimer{ 0.0f }; // 콤보 유지 유예 시간
    bool  isAttackActive{ false };  // ★ 충돌 파트용: 실제 타격 판정 활성 구간
    bool  attackHitDealt{ false };  // ★ 충돌 파트용: 이번 타격 중복 판정 방지

    // ---- 다관절 각도 (ComputeRobotParts 에서 계층 행렬 계산에 사용) ----
    float bodyBobY{ 0.0f };
    float spinYawOffset{ 0.0f };    // 360도 회전 킥용
    float pelvisTwistY{ 0.0f };     // 하체(골반) 회전
    float torsoTwistY{ -20.0f };    // 허리(상체) 좌우 비틀기
    float torsoPitchX{ 0.0f };      // 허리 앞뒤 숙이기
    float torsoRollZ{ 0.0f };       // 허리 좌우 기울이기
    float headYaw{ 20.0f };
    float headPitch{ 0.0f };

    float lShoulderX{ -60.0f }, lShoulderZ{ 15.0f }, lElbowX{ -85.0f };
    float rShoulderX{ -45.0f }, rShoulderZ{ -15.0f }, rElbowX{ -100.0f };

    float lHipX{ -15.0f }, lHipZ{ 0.0f }, lKneeX{ 20.0f }, lAnkleX{ -5.0f };
    float rHipX{ 12.0f }, rHipZ{ 0.0f }, rKneeX{ 20.0f }, rAnkleX{ -8.0f };

    // 타격 피크 순간 크기 강조 (Squash & Stretch)
    float lFistScale{ 1.0f };
    float rFistScale{ 1.0f };
    float rFootScale{ 1.0f };

    bool IsAlive() const { return hp > 0; }
};

// 관절 월드 행렬 (기존 필드 전부 유지 + 팔꿈치/주먹/무릎/발 행렬 추가)
struct RobotParts
{
    glm::mat4 root{ 1.0f };
    glm::mat4 pelvis{ 1.0f };
    glm::mat4 torso{ 1.0f };
    glm::mat4 head{ 1.0f };

    glm::mat4 leftArm{ 1.0f }, rightArm{ 1.0f };         // 어깨 -> 상완
    glm::mat4 leftForearm{ 1.0f }, rightForearm{ 1.0f }; // 팔꿈치 -> 하완
    glm::mat4 leftHand{ 1.0f }, rightHand{ 1.0f };       // 주먹 중심

    glm::mat4 leftLeg{ 1.0f }, rightLeg{ 1.0f };         // 골반 -> 허벅지
    glm::mat4 leftCalf{ 1.0f }, rightCalf{ 1.0f };       // 무릎 -> 정강이
    glm::mat4 leftFoot{ 1.0f }, rightFoot{ 1.0f };       // 발목 -> 발

    float lFistScale{ 1.0f };
    float rFistScale{ 1.0f };
    float rFootScale{ 1.0f };
};

void ResetRobot(Robot& r, const glm::vec3& pos, float yawDeg);

void UpdateRobotMotion(Robot& r, const glm::vec2& moveInput, float turnInput,
    bool jumpPressed, float dt);

// ---- 새로 추가된 액션 트리거 함수 (main.cpp 에서 키 입력 시 호출) ----
void StartRobotPunch(Robot& r);
void StartRobotKick(Robot& r);
void StartRobotDodge(Robot& r);

RobotParts ComputeRobotParts(const Robot& r);
void       DrawRobot(const RobotParts& parts, const RobotPalette& palette);

void FaceTowards(Robot& r, const glm::vec3& targetPos, float turnSpeedDeg, float dt);
void ResolveBodyOverlap(Robot& a, Robot& b);
void ApplyDamage(Robot& r, int amount);

// ---- 다른 모듈(Camera, 충돌)이 쓰는 조회 함수 (시그니처 100% 유지) ----
glm::mat4 RobotRootMatrix(const Robot& r);
glm::vec3 RobotForward(const Robot& r);
glm::vec3 RobotEyePos(const RobotParts& parts);
glm::vec3 RobotHeadPos(const RobotParts& parts);
glm::vec3 RobotHandPos(const RobotParts& parts, bool left);
glm::vec3 RobotFootPos(const RobotParts& parts, bool left);