#include "Robot.h"
#include "Renderer.h"
#include "Stage.h"

namespace
{
    const float moveSpeed = 4.0f;       // 초당 이동 거리
    const float turnSpeedDeg = 160.0f;  // 초당 회전 각도
    const float gravity = -22.0f;
    const float jumpSpeed = 7.5f;

    const float swingDeg = 40.0f;       // 걸을 때 팔다리를 흔드는 최대 각도
    const float swingSpeed = 7.0f;      // 위상이 커지는 속도 (클수록 빨리 흔든다)
    const float airPoseDeg = 25.0f;     // 공중에 있을 때 다리를 모으는 각도
}

const RobotPalette& PlayerPalette()
{
    // 왼팔 빨강 / 오른팔 파랑 -> 어느 쪽이 왼손인지 바로 보인다
    static const RobotPalette p{
        glm::vec3(0.85f, 0.85f, 0.90f),   // torso
        glm::vec3(0.95f, 0.80f, 0.65f),   // head
        glm::vec3(0.90f, 0.25f, 0.15f),   // nose
        glm::vec3(0.90f, 0.20f, 0.20f),   // leftArm
        glm::vec3(0.20f, 0.30f, 0.90f),   // rightArm
        glm::vec3(0.95f, 0.60f, 0.10f),   // leftLeg
        glm::vec3(0.20f, 0.70f, 0.80f),   // rightLeg
    };
    return p;
}

const RobotPalette& EnemyPalette()
{
    // 적은 어두운 금속 + 보라/자주 계열. 플레이어의 밝은 색과 멀리서도 구분된다.
    static const RobotPalette p{
        glm::vec3(0.30f, 0.28f, 0.36f),   // torso
        glm::vec3(0.42f, 0.36f, 0.44f),   // head
        glm::vec3(0.95f, 0.85f, 0.20f),   // nose (적은 노란 코)
        glm::vec3(0.55f, 0.16f, 0.42f),   // leftArm
        glm::vec3(0.30f, 0.14f, 0.50f),   // rightArm
        glm::vec3(0.24f, 0.22f, 0.30f),   // leftLeg
        glm::vec3(0.18f, 0.17f, 0.24f),   // rightLeg
    };
    return p;
}

void ResetRobot(Robot& r, const glm::vec3& pos, float yawDeg)
{
    r = Robot{};
    r.pos = pos;
    r.yawDeg = yawDeg;
}

glm::mat4 RobotRootMatrix(const Robot& r)
{
    glm::mat4 m{ 1.0f };
    m = glm::translate(m, r.pos);
    m = glm::rotate(m, glm::radians(r.yawDeg), glm::vec3(0.0f, 1.0f, 0.0f));
    return m;
}

glm::vec3 RobotForward(const Robot& r)
{
    const float yaw = glm::radians(r.yawDeg);
    return glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw));   // yawDeg = 0 일 때 +z
}

void UpdateRobotMotion(Robot& r, const glm::vec2& moveInput, float turnInput,
    bool jumpPressed, float dt)
{
    // ---- 방향 회전 ----
    // 추적 카메라가 로봇 뒤에서 앞(+z 쪽)을 보고 있으므로, 화면 오른쪽은 로컬 -x 이다.
    // 그래서 "오른쪽으로 돌기" 는 yaw 를 줄이는 쪽이 된다.
    r.yawDeg -= turnInput * turnSpeedDeg * dt;

    // ---- 수평 이동 ----
    const glm::vec3 forward = RobotForward(r);
    const glm::vec3 right{ -std::cos(glm::radians(r.yawDeg)), 0.0f,
                            std::sin(glm::radians(r.yawDeg)) };

    glm::vec3 delta = (forward * moveInput.y + right * moveInput.x) * moveSpeed * dt;
    r.pos += delta;

    // 벽을 뚫고 나가지 않게 연습장 안으로 제한
    ClampInsideRoom(r.pos, RobotSpec::bodyRadius);

    // ---- 점프 / 낙하 ----
    if (jumpPressed && r.onGround)
    {
        r.velY = jumpSpeed;
        r.onGround = false;
    }

    r.velY += gravity * dt;
    r.pos.y += r.velY * dt;

    if (r.pos.y <= 0.0f)
    {
        r.pos.y = 0.0f;
        r.velY = 0.0f;
        r.onGround = true;
    }

    // ---- 보행 스윙 ----
    // 입력 세기를 바로 쓰지 않고 부드럽게 따라가게 해서, 멈출 때 팔다리가 툭 끊기지 않게 한다.
    const float target = glm::length(moveInput);
    r.walkAmount += (target - r.walkAmount) * SmoothFactor(10.0f, dt);

    // 걷는 동안에만 위상이 돌아간다 (멈추면 그 자세에서 진폭만 0 으로 줄어든다)
    r.walkPhase += dt * swingSpeed * r.walkAmount;
}

RobotParts ComputeRobotParts(const Robot& r)
{
    using namespace RobotSpec;

    RobotParts p;
    p.root = RobotRootMatrix(r);

    p.torso = glm::translate(p.root, torsoCenter);
    p.head = glm::translate(p.root, headCenter);

    // 걸을 때는 sin 으로 흔들고, 공중에서는 고정 자세를 쓴다.
    const float swing = r.onGround
        ? swingDeg * r.walkAmount * std::sin(r.walkPhase)
        : 0.0f;
    const float air = r.onGround ? 0.0f : airPoseDeg;

    const glm::vec3 xAxis{ 1.0f, 0.0f, 0.0f };

    // 관절 행렬 = 회전 중심으로 이동 -> 회전.
    // (스케일은 넣지 않는다. 그래야 이 행렬로 손끝/발끝 좌표를 바로 구할 수 있다)
    p.leftLeg = glm::rotate(glm::translate(p.root, leftHip),
        glm::radians(swing + air), xAxis);
    p.rightLeg = glm::rotate(glm::translate(p.root, rightHip),
        glm::radians(-swing + air), xAxis);

    // 팔은 다리와 반대로 흔든다
    p.leftArm = glm::rotate(glm::translate(p.root, leftShoulder),
        glm::radians(-swing - air), xAxis);
    p.rightArm = glm::rotate(glm::translate(p.root, rightShoulder),
        glm::radians(swing - air), xAxis);

    return p;
}

void DrawRobot(const RobotParts& parts, const RobotPalette& palette)
{
    using namespace RobotSpec;

    DrawCube(glm::scale(parts.torso, torsoSize), palette.torso);
    DrawCube(glm::scale(parts.head, headSize), palette.head);
    DrawCube(glm::scale(glm::translate(parts.head, noseOffset), noseSize), palette.nose);

    // 관절 행렬에서 길이의 절반만큼 아래로 내려 큐브를 걸어 준다.
    // -> 큐브의 "윗면 중심" 이 관절 위치에 오므로, 관절을 축으로 돌아가는 것처럼 보인다.
    auto drawLimb = [](const glm::mat4& joint, float length, float thick, const glm::vec3& color)
        {
            glm::mat4 m = glm::translate(joint, glm::vec3(0.0f, -length * 0.5f, 0.0f));
            m = glm::scale(m, glm::vec3(thick, length, thick));
            DrawCube(m, color);
        };

    drawLimb(parts.leftLeg, legLength, legThick, palette.leftLeg);
    drawLimb(parts.rightLeg, legLength, legThick, palette.rightLeg);
    drawLimb(parts.leftArm, armLength, armThick, palette.leftArm);
    drawLimb(parts.rightArm, armLength, armThick, palette.rightArm);
}

void FaceTowards(Robot& r, const glm::vec3& targetPos, float turnSpeedDeg, float dt)
{
    const glm::vec3 d = targetPos - r.pos;
    if (d.x * d.x + d.z * d.z < 1e-6f) return;

    // yawDeg = 0 일 때 +z 를 보므로(forward = sin, 0, cos), 원하는 각도는 atan2(x, z) 이다
    const float wantDeg = glm::degrees(std::atan2(d.x, d.z));

    // -180 ~ +180 으로 감싸서 항상 가까운 쪽으로 돌게 한다
    float diff = wantDeg - r.yawDeg;
    while (diff > 180.0f)  diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;

    const float step = turnSpeedDeg * dt;
    r.yawDeg += glm::clamp(diff, -step, step);
}

void ResolveBodyOverlap(Robot& a, Robot& b)
{
    glm::vec3 d = b.pos - a.pos;
    d.y = 0.0f;

    const float minDist = RobotSpec::bodyRadius * 2.0f;
    const float dist = std::sqrt(d.x * d.x + d.z * d.z);
    if (dist >= minDist) return;

    // 완전히 겹쳐 있으면 나눌 방향이 없으므로 +z 로 임의 분리한다
    const glm::vec3 dir = (dist > 1e-4f) ? (d / dist) : glm::vec3(0.0f, 0.0f, 1.0f);
    const float push = (minDist - dist) * 0.5f;

    a.pos -= dir * push;
    b.pos += dir * push;

    // 밀린 결과가 벽을 넘지 않도록 다시 가둔다
    ClampInsideRoom(a.pos, RobotSpec::bodyRadius);
    ClampInsideRoom(b.pos, RobotSpec::bodyRadius);
}

void ApplyDamage(Robot& r, int amount)
{
    r.hp = glm::max(r.hp - amount, 0);
}

glm::vec3 RobotEyePos(const RobotParts& parts)
{
    return glm::vec3(parts.head * glm::vec4(RobotSpec::eyeOffset, 1.0f));
}

glm::vec3 RobotHeadPos(const RobotParts& parts)
{
    return glm::vec3(parts.head[3]);
}

glm::vec3 RobotHandPos(const RobotParts& parts, bool left)
{
    const glm::mat4& arm = left ? parts.leftArm : parts.rightArm;
    return glm::vec3(arm * glm::vec4(0.0f, -RobotSpec::armLength, 0.0f, 1.0f));
}

glm::vec3 RobotFootPos(const RobotParts& parts, bool left)
{
    const glm::mat4& leg = left ? parts.leftLeg : parts.rightLeg;
    return glm::vec3(leg * glm::vec4(0.0f, -RobotSpec::legLength, 0.0f, 1.0f));
}
