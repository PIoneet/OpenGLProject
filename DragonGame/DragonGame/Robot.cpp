#include "Robot.h"
#include "Renderer.h"
#include "Stage.h"

namespace
{
    const float moveSpeed = 4.0f;
    const float turnSpeedDeg = 160.0f;
    const float gravity = -22.0f;
    const float jumpSpeed = 7.5f;
    const float swingSpeed = 9.0f;

    float Lerp(float a, float b, float t)
    {
        t = glm::clamp(t, 0.0f, 1.0f);
        return a + (b - a) * t;
    }

    float EaseIn(float t)
    {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t;
    }

    float EaseOut(float t)
    {
        t = glm::clamp(t, 0.0f, 1.0f);
        const float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    float EaseOutQuart(float t)
    {
        t = glm::clamp(t, 0.0f, 1.0f);
        const float u = 1.0f - t;
        return 1.0f - u * u * u * u;
    }

    float EaseOutBack(float t)
    {
        t = glm::clamp(t, 0.0f, 1.0f);
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        const float u = t - 1.0f;
        return 1.0f + c3 * u * u * u + c1 * u * u;
    }

    void StepForward(Robot& r, float speed, float dt)
    {
        r.pos += RobotForward(r) * speed * dt;
        ClampInsideRoom(r.pos, RobotSpec::bodyRadius);
    }

    void ResetCombatModifiers(Robot& r)
    {
        r.spinYawOffset = 0.0f;
        r.pelvisTwistY = 0.0f;
        r.torsoPitchX = 0.0f;
        r.torsoRollZ = 0.0f;
        r.headPitch = 0.0f;
        r.lHipZ = 0.0f;
        r.rHipZ = 0.0f;
        r.lFistScale = 1.0f;
        r.rFistScale = 1.0f;
        r.rFootScale = 1.0f;
    }

    void ChangeState(Robot& r, RobotActionState newState, float duration = 0.0f)
    {
        r.state = newState;
        r.stateTimer = 0.0f;
        r.stateDuration = duration;
    }

    void BeginAttackStep(Robot& r, int type, int step)
    {
        ResetCombatModifiers(r);
        r.attackType = type;
        r.comboStep = step;
        r.comboQueued = false;
        r.isAttackActive = false;
        r.attackHitDealt = false;

        if (type == 1)
        {
            if (step == 1)      ChangeState(r, RobotActionState::Attack, 0.30f); // 1타: 왼손 잽
            else if (step == 2) ChangeState(r, RobotActionState::Attack, 0.40f); // 2타: 오른손 스트레이트
            else                ChangeState(r, RobotActionState::Attack, 0.56f); // 3타: 오른손 어퍼컷
        }
        else if (type == 2)
        {
            if (step == 1)      ChangeState(r, RobotActionState::Attack, 0.52f); // 1타: 오른발 돌려차기
            else                ChangeState(r, RobotActionState::Attack, 0.62f); // 2타: 360도 회전 하이킥
        }
    }

    void UpdateIdleAndWalkPose(Robot& r, const glm::vec2& moveInput, float dt)
    {
        ResetCombatModifiers(r);

        // 공중 점프 자세
        if (!r.onGround)
        {
            r.bodyBobY = 0.0f;
            r.torsoTwistY = -15.0f;
            r.torsoPitchX = 10.0f;
            r.headYaw = 15.0f;
            r.lShoulderX = -110.0f; r.lShoulderZ = 25.0f;  r.lElbowX = -65.0f;
            r.rShoulderX = -100.0f; r.rShoulderZ = -25.0f; r.rElbowX = -75.0f;
            r.lHipX = -45.0f; r.lKneeX = 70.0f; r.lAnkleX = 15.0f;
            r.rHipX = -25.0f; r.rKneeX = 55.0f; r.rAnkleX = 15.0f;
            return;
        }

        // 기본 가드 숨쉬기 자세
        r.idlePhase += dt * 4.5f;
        const float bob = std::sin(r.idlePhase);

        const float idleBobY = bob * 0.02f;
        const float idleTorsoTwist = -20.0f; // 왼어깨(+X)가 앞, 오른어깨(-X)가 뒤
        const float idleLShX = -60.0f - bob * 3.0f;
        const float idleLElbX = -85.0f + bob * 3.0f;
        const float idleRShX = -45.0f + bob * 3.0f;
        const float idleRElbX = -100.0f - bob * 3.0f;

        // 보행 자세 (walkAmount 0~1 로 부드럽게 블렌딩)
        const float swing = std::sin(r.walkPhase);
        const float bounce = std::fabs(std::cos(r.walkPhase));
        const float w = glm::clamp(r.walkAmount, 0.0f, 1.0f);

        const float walkPitch = (moveInput.y < -0.1f) ? -5.0f : 8.0f;
        const float walkTorsoTwist = -10.0f + swing * 12.0f;

        r.bodyBobY = Lerp(idleBobY, bounce * 0.04f, w);
        r.torsoPitchX = Lerp(0.0f, walkPitch, w);
        r.torsoTwistY = Lerp(idleTorsoTwist, walkTorsoTwist, w);
        r.headYaw = -r.torsoTwistY;

        r.lShoulderX = Lerp(idleLShX, -55.0f + swing * 20.0f, w);
        r.lShoulderZ = 15.0f;
        r.lElbowX = Lerp(idleLElbX, -85.0f, w);

        r.rShoulderX = Lerp(idleRShX, -45.0f - swing * 20.0f, w);
        r.rShoulderZ = -15.0f;
        r.rElbowX = Lerp(idleRElbX, -95.0f, w);

        const float walkLHip = -swing * 35.0f;
        const float walkRHip = swing * 35.0f;
        const float walkLKnee = (swing < 0.0f) ? -swing * 35.0f : 10.0f;
        const float walkRKnee = (swing > 0.0f) ? swing * 35.0f : 10.0f;

        r.lHipX = Lerp(-15.0f, walkLHip, w);
        r.lKneeX = Lerp(20.0f, walkLKnee, w);
        r.lAnkleX = Lerp(-5.0f, 0.0f, w);

        r.rHipX = Lerp(12.0f, walkRHip, w);
        r.rKneeX = Lerp(20.0f, walkRKnee, w);
        r.rAnkleX = Lerp(-8.0f, 0.0f, w);
    }

    void UpdateDodgePose(Robot& r, float dt)
    {
        const float t = r.stateTimer / r.stateDuration;
        StepForward(r, 9.5f * (1.0f - t), dt);

        if (t < 0.5f)
        {
            const float s = EaseOut(t / 0.5f);
            r.bodyBobY = Lerp(0.0f, -0.18f, s);
            r.torsoPitchX = Lerp(0.0f, 28.0f, s);
            r.torsoTwistY = Lerp(-20.0f, 25.0f, s);
            r.headYaw = -r.torsoTwistY;
            r.rHipX = Lerp(12.0f, -38.0f, s);  r.rKneeX = Lerp(20.0f, 65.0f, s);
            r.lHipX = Lerp(-15.0f, 32.0f, s);  r.lKneeX = Lerp(20.0f, 45.0f, s);
            r.rShoulderX = Lerp(-45.0f, -20.0f, s);
            r.lShoulderX = Lerp(-60.0f, -80.0f, s);
        }
        else
        {
            const float s = EaseIn((t - 0.5f) / 0.5f);
            r.bodyBobY = Lerp(-0.18f, 0.0f, s);
            r.torsoPitchX = Lerp(28.0f, 0.0f, s);
            r.torsoTwistY = Lerp(25.0f, -20.0f, s);
            r.headYaw = -r.torsoTwistY;
            r.rHipX = Lerp(-38.0f, 12.0f, s);  r.rKneeX = Lerp(65.0f, 20.0f, s);
            r.lHipX = Lerp(32.0f, -15.0f, s);  r.lKneeX = Lerp(45.0f, 20.0f, s);
            r.rShoulderX = Lerp(-20.0f, -45.0f, s);
            r.lShoulderX = Lerp(-80.0f, -60.0f, s);
        }
    }

    void UpdateAttackPose(Robot& r, float dt)
    {
        const float t = r.stateTimer / r.stateDuration;

        if (r.attackType == 1)
        {
            if (r.comboStep == 1)
            {
                // [펀치 1타: 빠른 왼손 잽] (+X 왼어깨를 앞 +Z 로 밀어넣기 -> torsoTwistY 음수!)
                if (t < 0.20f)
                {
                    const float s = EaseIn(t / 0.20f);
                    r.isAttackActive = false;
                    r.torsoTwistY = Lerp(-20.0f, -5.0f, s);
                    r.headYaw = -r.torsoTwistY;
                    r.lShoulderX = Lerp(-60.0f, -45.0f, s);
                    r.lElbowX = Lerp(-85.0f, -105.0f, s);
                }
                else if (t < 0.55f)
                {
                    const float s = EaseOutQuart((t - 0.20f) / 0.35f);
                    r.isAttackActive = true;
                    StepForward(r, 2.5f * (1.0f - s), dt);
                    r.bodyBobY = Lerp(0.0f, -0.04f, s);
                    r.torsoTwistY = Lerp(-5.0f, -42.0f, s);
                    r.torsoPitchX = Lerp(0.0f, 8.0f, s);
                    r.headYaw = -r.torsoTwistY;
                    r.lShoulderX = Lerp(-45.0f, -92.0f, s);
                    r.lShoulderZ = Lerp(15.0f, 0.0f, s);
                    r.lElbowX = Lerp(-105.0f, 0.0f, s);
                    r.lFistScale = Lerp(1.0f, 1.35f, std::sin(s * 3.14159f));
                    r.rShoulderX = Lerp(-45.0f, -55.0f, s);
                    r.lHipX = Lerp(-15.0f, -28.0f, s); r.lKneeX = Lerp(20.0f, 35.0f, s);
                }
                else
                {
                    const float s = EaseIn((t - 0.55f) / 0.45f);
                    r.isAttackActive = false;
                    r.bodyBobY = Lerp(-0.04f, 0.0f, s);
                    r.torsoTwistY = Lerp(-42.0f, -20.0f, s);
                    r.torsoPitchX = Lerp(8.0f, 0.0f, s);
                    r.headYaw = -r.torsoTwistY;
                    r.lShoulderX = Lerp(-92.0f, -60.0f, s);
                    r.lShoulderZ = Lerp(0.0f, 15.0f, s);
                    r.lElbowX = Lerp(0.0f, -85.0f, s);
                    r.lFistScale = 1.0f;
                    r.lHipX = Lerp(-28.0f, -15.0f, s); r.lKneeX = Lerp(35.0f, 20.0f, s);
                }
            }
            else if (r.comboStep == 2)
            {
                // [펀치 2타: 허리 실은 오른손 스트레이트] (-X 오른어깨를 앞 +Z 로 -> torsoTwistY 양수!)
                if (t < 0.25f)
                {
                    const float s = EaseIn(t / 0.25f);
                    r.isAttackActive = false;
                    r.bodyBobY = Lerp(0.0f, -0.07f, s);
                    r.torsoTwistY = Lerp(-25.0f, -52.0f, s);
                    r.torsoRollZ = Lerp(0.0f, 8.0f, s);
                    r.headYaw = -r.torsoTwistY;
                    r.rShoulderX = Lerp(-45.0f, 20.0f, s);
                    r.rElbowX = Lerp(-100.0f, -120.0f, s);
                    r.lShoulderX = Lerp(-60.0f, -75.0f, s);
                }
                else if (t < 0.58f)
                {
                    const float s = EaseOutQuart((t - 0.25f) / 0.33f);
                    r.isAttackActive = true;
                    StepForward(r, 4.2f * (1.0f - s), dt);
                    r.bodyBobY = Lerp(-0.07f, -0.11f, s);
                    r.pelvisTwistY = Lerp(-10.0f, 20.0f, s);
                    r.torsoTwistY = Lerp(-52.0f, 42.0f, s);
                    r.torsoPitchX = Lerp(0.0f, 16.0f, s);
                    r.torsoRollZ = Lerp(8.0f, -10.0f, s);
                    r.headYaw = -(r.pelvisTwistY + r.torsoTwistY);
                    r.rShoulderX = Lerp(20.0f, -95.0f, s);
                    r.rShoulderZ = Lerp(-15.0f, 5.0f, s);
                    r.rElbowX = Lerp(-120.0f, 0.0f, s);
                    r.rFistScale = Lerp(1.0f, 1.48f, std::sin(s * 3.14159f));
                    r.lShoulderX = Lerp(-75.0f, 10.0f, s);
                    r.lElbowX = Lerp(-85.0f, -110.0f, s);
                    r.lHipX = Lerp(-15.0f, -35.0f, s); r.lKneeX = Lerp(20.0f, 45.0f, s);
                    r.rHipX = Lerp(12.0f, 35.0f, s);   r.rKneeX = Lerp(20.0f, 15.0f, s);
                }
                else
                {
                    const float s = EaseIn((t - 0.58f) / 0.42f);
                    r.isAttackActive = false;
                    r.bodyBobY = Lerp(-0.11f, 0.0f, s);
                    r.pelvisTwistY = Lerp(20.0f, 0.0f, s);
                    r.torsoTwistY = Lerp(42.0f, -20.0f, s);
                    r.torsoPitchX = Lerp(16.0f, 0.0f, s);
                    r.torsoRollZ = Lerp(-10.0f, 0.0f, s);
                    r.headYaw = -(r.pelvisTwistY + r.torsoTwistY);
                    r.rShoulderX = Lerp(-95.0f, -45.0f, s);
                    r.rShoulderZ = Lerp(5.0f, -15.0f, s);
                    r.rElbowX = Lerp(0.0f, -100.0f, s);
                    r.rFistScale = 1.0f;
                    r.lShoulderX = Lerp(10.0f, -60.0f, s);
                    r.lElbowX = Lerp(-110.0f, -85.0f, s);
                    r.lHipX = Lerp(-35.0f, -15.0f, s); r.lKneeX = Lerp(45.0f, 20.0f, s);
                    r.rHipX = Lerp(35.0f, 12.0f, s);   r.rKneeX = Lerp(15.0f, 20.0f, s);
                }
            }
            else
            {
                // [펀치 3타: 웅크렸다 솟구치는 오른손 어퍼컷]
                if (t < 0.30f)
                {
                    const float s = EaseIn(t / 0.30f);
                    r.isAttackActive = false;
                    r.bodyBobY = Lerp(0.0f, -0.22f, s);
                    r.torsoPitchX = Lerp(0.0f, 28.0f, s);
                    r.torsoTwistY = Lerp(-20.0f, -55.0f, s);
                    r.torsoRollZ = Lerp(0.0f, 18.0f, s); // 오른쪽(-X)으로 낮춤
                    r.headYaw = -r.torsoTwistY;
                    r.rShoulderX = Lerp(-45.0f, 35.0f, s);
                    r.rShoulderZ = Lerp(-15.0f, -24.0f, s);
                    r.rElbowX = Lerp(-100.0f, -75.0f, s);
                    r.lShoulderX = Lerp(-60.0f, -35.0f, s);
                    r.rHipX = Lerp(12.0f, -42.0f, s);  r.rKneeX = Lerp(20.0f, 78.0f, s);
                    r.lHipX = Lerp(-15.0f, -45.0f, s); r.lKneeX = Lerp(20.0f, 78.0f, s);
                }
                else if (t < 0.62f)
                {
                    const float s = EaseOutBack((t - 0.30f) / 0.32f);
                    const float sClamp = glm::clamp(s, 0.0f, 1.0f);
                    r.isAttackActive = true;
                    StepForward(r, 3.8f * (1.0f - sClamp), dt);
                    r.bodyBobY = Lerp(-0.22f, 0.14f, s);
                    r.pelvisTwistY = Lerp(-10.0f, 25.0f, sClamp);
                    r.torsoPitchX = Lerp(28.0f, -20.0f, s);
                    r.torsoTwistY = Lerp(-55.0f, 38.0f, sClamp);
                    r.torsoRollZ = Lerp(18.0f, -15.0f, sClamp);
                    r.headYaw = -(r.pelvisTwistY + r.torsoTwistY);
                    r.headPitch = Lerp(10.0f, -18.0f, sClamp);
                    r.rShoulderX = Lerp(35.0f, -150.0f, s);
                    r.rShoulderZ = Lerp(-24.0f, -5.0f, sClamp);
                    r.rElbowX = Lerp(-75.0f, -65.0f, sClamp);
                    r.rFistScale = Lerp(1.0f, 1.60f, std::sin(sClamp * 3.14159f));
                    r.lShoulderX = Lerp(-35.0f, 25.0f, sClamp);
                    r.rHipX = Lerp(-42.0f, 15.0f, sClamp);  r.rKneeX = Lerp(78.0f, 5.0f, sClamp);
                    r.lHipX = Lerp(-45.0f, -10.0f, sClamp); r.lKneeX = Lerp(78.0f, 5.0f, sClamp);
                }
                else
                {
                    const float s = EaseIn((t - 0.62f) / 0.38f);
                    r.isAttackActive = false;
                    r.bodyBobY = Lerp(0.14f, 0.0f, s);
                    r.pelvisTwistY = Lerp(25.0f, 0.0f, s);
                    r.torsoPitchX = Lerp(-20.0f, 0.0f, s);
                    r.torsoTwistY = Lerp(38.0f, -20.0f, s);
                    r.torsoRollZ = Lerp(-15.0f, 0.0f, s);
                    r.headYaw = -(r.pelvisTwistY + r.torsoTwistY);
                    r.headPitch = Lerp(-18.0f, 0.0f, s);
                    r.rShoulderX = Lerp(-150.0f, -45.0f, s);
                    r.rShoulderZ = Lerp(-5.0f, -15.0f, s);
                    r.rElbowX = Lerp(-65.0f, -100.0f, s);
                    r.rFistScale = 1.0f;
                    r.lShoulderX = Lerp(25.0f, -60.0f, s);
                    r.rHipX = Lerp(15.0f, 12.0f, s);   r.rKneeX = Lerp(5.0f, 20.0f, s);
                    r.lHipX = Lerp(-10.0f, -15.0f, s); r.lKneeX = Lerp(5.0f, 20.0f, s);
                }
            }
        }
        else if (r.attackType == 2)
        {
            if (r.comboStep == 1)
            {
                // [킥 1타: 오른발 돌려차기] (-X 오른다리를 옆(-X)으로 들기 -> rHipZ 음수, 상체는 왼쪽(+X) 기울기 -> torsoRollZ 음수!)
                if (t < 0.26f)
                {
                    const float s = EaseIn(t / 0.26f);
                    r.isAttackActive = false;
                    r.bodyBobY = Lerp(0.0f, -0.07f, s);
                    r.pelvisTwistY = Lerp(0.0f, -26.0f, s);
                    r.torsoTwistY = Lerp(-20.0f, -25.0f, s);
                    r.torsoRollZ = Lerp(0.0f, -28.0f, s);
                    r.headYaw = -(r.pelvisTwistY + r.torsoTwistY);
                    r.rHipZ = Lerp(0.0f, -68.0f, s);
                    r.rHipX = Lerp(12.0f, -38.0f, s);
                    r.rKneeX = Lerp(20.0f, 112.0f, s);
                    r.lHipX = Lerp(-15.0f, -10.0f, s); r.lKneeX = Lerp(20.0f, 35.0f, s);
                }
                else if (t < 0.58f)
                {
                    const float s = EaseOutQuart((t - 0.26f) / 0.32f);
                    r.isAttackActive = true;
                    StepForward(r, 2.8f * (1.0f - s), dt);
                    r.bodyBobY = Lerp(-0.07f, -0.02f, s);
                    r.pelvisTwistY = Lerp(-26.0f, 52.0f, s);
                    r.torsoTwistY = Lerp(-25.0f, 10.0f, s);
                    r.torsoPitchX = Lerp(0.0f, -12.0f, s);
                    r.torsoRollZ = Lerp(-28.0f, -36.0f, s);
                    r.headYaw = -(r.pelvisTwistY + r.torsoTwistY) * 0.85f;
                    r.rHipZ = Lerp(-68.0f, -92.0f, s);
                    r.rHipX = Lerp(-38.0f, -35.0f, s);
                    r.rKneeX = Lerp(112.0f, 0.0f, s);
                    r.rAnkleX = Lerp(-8.0f, 35.0f, s);
                    r.rFootScale = Lerp(1.0f, 1.55f, std::sin(s * 3.14159f));
                    r.rShoulderZ = Lerp(-15.0f, -55.0f, s); r.rShoulderX = Lerp(-45.0f, 15.0f, s);
                    r.lShoulderZ = Lerp(15.0f, 45.0f, s);   r.lShoulderX = Lerp(-60.0f, -20.0f, s);
                    r.lKneeX = Lerp(35.0f, 18.0f, s);
                }
                else
                {
                    const float s = EaseIn((t - 0.58f) / 0.42f);
                    r.isAttackActive = false;
                    r.bodyBobY = Lerp(-0.02f, 0.0f, s);
                    r.pelvisTwistY = Lerp(52.0f, 0.0f, s);
                    r.torsoTwistY = Lerp(10.0f, -20.0f, s);
                    r.torsoPitchX = Lerp(-12.0f, 0.0f, s);
                    r.torsoRollZ = Lerp(-36.0f, 0.0f, s);
                    r.headYaw = -(r.pelvisTwistY + r.torsoTwistY);
                    r.rHipZ = Lerp(-92.0f, 0.0f, s);
                    r.rHipX = Lerp(-35.0f, 12.0f, s);
                    r.rKneeX = Lerp(0.0f, 20.0f, s);
                    r.rAnkleX = Lerp(35.0f, -8.0f, s);
                    r.rFootScale = 1.0f;
                    r.rShoulderZ = Lerp(-55.0f, -15.0f, s); r.rShoulderX = Lerp(15.0f, -45.0f, s);
                    r.lShoulderZ = Lerp(45.0f, 15.0f, s);   r.lShoulderX = Lerp(-20.0f, -60.0f, s);
                    r.lHipX = Lerp(-10.0f, -15.0f, s); r.lKneeX = Lerp(18.0f, 20.0f, s);
                }
            }
            else
            {
                // [킥 2타: 360도 회전 하이킥]
                if (t < 0.22f)
                {
                    const float s = EaseIn(t / 0.22f);
                    r.isAttackActive = false;
                    r.bodyBobY = Lerp(0.0f, -0.11f, s);
                    r.spinYawOffset = Lerp(0.0f, -25.0f, s);
                    r.torsoTwistY = Lerp(-20.0f, -40.0f, s);
                    r.lKneeX = Lerp(20.0f, 45.0f, s);
                    r.rKneeX = Lerp(20.0f, 70.0f, s);
                }
                else if (t < 0.68f)
                {
                    const float s = EaseOutQuart((t - 0.22f) / 0.46f);
                    const float arc = std::sin(s * 3.14159f);
                    r.isAttackActive = true;
                    StepForward(r, 3.5f * (1.0f - s), dt);
                    r.bodyBobY = Lerp(-0.11f, 0.08f, arc);
                    r.spinYawOffset = Lerp(-25.0f, 360.0f, s);
                    r.torsoTwistY = Lerp(-40.0f, 20.0f, s);
                    r.torsoRollZ = Lerp(0.0f, -40.0f, arc);
                    r.torsoPitchX = Lerp(0.0f, -14.0f, arc);
                    r.headYaw = 0.0f;
                    r.rHipZ = Lerp(-20.0f, -98.0f, arc);
                    r.rHipX = Lerp(-20.0f, -42.0f, arc);
                    r.rKneeX = Lerp(70.0f, 0.0f, EaseOut(s));
                    r.rAnkleX = Lerp(0.0f, 35.0f, s);
                    r.rFootScale = Lerp(1.0f, 1.65f, arc);
                    r.rShoulderZ = Lerp(-15.0f, -70.0f, arc);
                    r.lShoulderZ = Lerp(15.0f, 70.0f, arc);
                    r.rElbowX = Lerp(-100.0f, -30.0f, arc);
                    r.lElbowX = Lerp(-85.0f, -30.0f, arc);
                }
                else
                {
                    const float s = EaseIn((t - 0.68f) / 0.32f);
                    r.isAttackActive = false;
                    r.spinYawOffset = 360.0f;
                    r.bodyBobY = Lerp(r.bodyBobY, 0.0f, s);
                    r.torsoTwistY = Lerp(20.0f, -20.0f, s);
                    r.torsoRollZ = Lerp(r.torsoRollZ, 0.0f, s);
                    r.torsoPitchX = Lerp(r.torsoPitchX, 0.0f, s);
                    r.headYaw = -r.torsoTwistY;
                    r.rHipZ = Lerp(r.rHipZ, 0.0f, s);
                    r.rHipX = Lerp(r.rHipX, 12.0f, s);
                    r.rKneeX = Lerp(r.rKneeX, 20.0f, s);
                    r.rAnkleX = Lerp(35.0f, -8.0f, s);
                    r.rFootScale = 1.0f;
                    r.rShoulderZ = Lerp(r.rShoulderZ, -15.0f, s);
                    r.lShoulderZ = Lerp(r.lShoulderZ, 15.0f, s);
                    r.rElbowX = Lerp(r.rElbowX, -100.0f, s);
                    r.lElbowX = Lerp(r.lElbowX, -85.0f, s);
                    r.lKneeX = Lerp(45.0f, 20.0f, s);
                }
            }
        }
    }
}

const RobotPalette& PlayerPalette()
{
    static const RobotPalette p{
        glm::vec3(0.85f, 0.85f, 0.90f),
        glm::vec3(0.95f, 0.80f, 0.65f),
        glm::vec3(0.90f, 0.25f, 0.15f),
        glm::vec3(0.90f, 0.20f, 0.20f),
        glm::vec3(0.20f, 0.30f, 0.90f),
        glm::vec3(0.95f, 0.60f, 0.10f),
        glm::vec3(0.20f, 0.70f, 0.80f),
    };
    return p;
}

const RobotPalette& EnemyPalette()
{
    static const RobotPalette p{
        glm::vec3(0.30f, 0.28f, 0.36f),
        glm::vec3(0.42f, 0.36f, 0.44f),
        glm::vec3(0.95f, 0.85f, 0.20f),
        glm::vec3(0.55f, 0.16f, 0.42f),
        glm::vec3(0.30f, 0.14f, 0.50f),
        glm::vec3(0.24f, 0.22f, 0.30f),
        glm::vec3(0.18f, 0.17f, 0.24f),
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
    return glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw));
}

void StartRobotPunch(Robot& r)
{
    if (!r.onGround) return;

    if (r.state == RobotActionState::IdleMove)
    {
        const int nextStep = (r.attackType == 1 && r.comboWindowTimer > 0.0f) ? (r.comboStep % 3) + 1 : 1;
        BeginAttackStep(r, 1, nextStep);
    }
    else if (r.state == RobotActionState::Attack && r.attackType == 1 && r.comboStep < 3)
    {
        if (r.stateTimer / r.stateDuration >= 0.40f)
            r.comboQueued = true;
    }
}

void StartRobotKick(Robot& r)
{
    if (!r.onGround) return;

    if (r.state == RobotActionState::IdleMove)
    {
        const int nextStep = (r.attackType == 2 && r.comboWindowTimer > 0.0f) ? (r.comboStep % 2) + 1 : 1;
        BeginAttackStep(r, 2, nextStep);
    }
    else if (r.state == RobotActionState::Attack && r.attackType == 2 && r.comboStep < 2)
    {
        if (r.stateTimer / r.stateDuration >= 0.45f)
            r.comboQueued = true;
    }
}

void StartRobotDodge(Robot& r)
{
    if (r.onGround && r.state == RobotActionState::IdleMove)
    {
        ResetCombatModifiers(r);
        ChangeState(r, RobotActionState::Dodge, 0.35f);
    }
}

void UpdateRobotMotion(Robot& r, const glm::vec2& moveInput, float turnInput,
    bool jumpPressed, float dt)
{
    r.stateTimer += dt;

    if (r.state != RobotActionState::Attack && r.comboWindowTimer > 0.0f)
    {
        r.comboWindowTimer -= dt;
        if (r.comboWindowTimer <= 0.0f)
            r.comboStep = 0;
    }

    // ---- 중력 / 착지 처리 (모든 상태에서 공통 유지) ----
    if (r.state == RobotActionState::IdleMove && jumpPressed && r.onGround)
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

    // ---- 상태별 이동 및 관절 애니메이션 분기 ----
    switch (r.state)
    {
    case RobotActionState::IdleMove:
    {
        r.yawDeg -= turnInput * turnSpeedDeg * dt;

        const glm::vec3 forward = RobotForward(r);
        const glm::vec3 right{ -std::cos(glm::radians(r.yawDeg)), 0.0f,
                                std::sin(glm::radians(r.yawDeg)) };

        r.pos += (forward * moveInput.y + right * moveInput.x) * moveSpeed * dt;
        ClampInsideRoom(r.pos, RobotSpec::bodyRadius);

        const float target = glm::length(moveInput);
        r.walkAmount += (target - r.walkAmount) * SmoothFactor(10.0f, dt);

        const float walkDirSign = (moveInput.y < -0.1f) ? -1.0f : 1.0f;
        r.walkPhase += dt * swingSpeed * r.walkAmount * walkDirSign;

        UpdateIdleAndWalkPose(r, moveInput, dt);
        break;
    }

    case RobotActionState::Dodge:
        UpdateDodgePose(r, dt);
        if (r.stateTimer >= r.stateDuration)
        {
            ResetCombatModifiers(r);
            ChangeState(r, RobotActionState::IdleMove);
        }
        break;

    case RobotActionState::Attack:
        // 공격 중에도 살짝 방향 전환은 허용하면 조작감이 좋아짐
        r.yawDeg -= turnInput * (turnSpeedDeg * 0.25f) * dt;
        UpdateAttackPose(r, dt);

        if (r.comboQueued && (r.stateTimer / r.stateDuration >= 0.82f))
        {
            BeginAttackStep(r, r.attackType, r.comboStep + 1);
        }
        else if (r.stateTimer >= r.stateDuration)
        {
            r.isAttackActive = false;
            ResetCombatModifiers(r);
            r.comboWindowTimer = 0.45f;
            ChangeState(r, RobotActionState::IdleMove);
        }
        break;

    default:
        break;
    }
}

RobotParts ComputeRobotParts(const Robot& r)
{
    using namespace RobotSpec;

    const glm::vec3 xAxis{ 1.0f, 0.0f, 0.0f };
    const glm::vec3 yAxis{ 0.0f, 1.0f, 0.0f };
    const glm::vec3 zAxis{ 0.0f, 0.0f, 1.0f };

    RobotParts p;
    p.root = RobotRootMatrix(r);
    p.lFistScale = r.lFistScale;
    p.rFistScale = r.rFistScale;
    p.rFootScale = r.rFootScale;

    // [0] 골반(하체 기준점: 바닥에서 다리 길이 0.9f 위)
    p.pelvis = glm::translate(p.root, glm::vec3(0.0f, legLength + r.bodyBobY, 0.0f));
    p.pelvis = glm::rotate(p.pelvis, glm::radians(r.spinYawOffset + r.pelvisTwistY), yAxis);

    // [1] 허리(상체 3축 회전) -> 몸통 중심은 허리에서 +0.5f 위 (서 있을 때 y = 1.4f)
    glm::mat4 waist = glm::rotate(p.pelvis, glm::radians(r.torsoTwistY), yAxis);
    waist = glm::rotate(waist, glm::radians(r.torsoPitchX), xAxis);
    waist = glm::rotate(waist, glm::radians(r.torsoRollZ), zAxis);
    p.torso = glm::translate(waist, glm::vec3(0.0f, torsoCenter.y - legLength, 0.0f));

    // [2] 목 -> 머리 중심 (서 있을 때 y = 2.15f, Camera의 RobotEyePos / RobotHeadPos 와 완벽 호환!)
    const float neckY = torsoCenter.y - legLength + torsoSize.y * 0.5f; // 1.0f
    glm::mat4 neck = glm::translate(waist, glm::vec3(0.0f, neckY, 0.0f));
    neck = glm::rotate(neck, glm::radians(r.headYaw), yAxis);
    neck = glm::rotate(neck, glm::radians(r.headPitch), xAxis);
    p.head = glm::translate(neck, glm::vec3(0.0f, headSize.y * 0.5f, 0.0f));

    // [3] 왼팔 (+X: 어깨 -> 상완 -> 팔꿈치 -> 하완 -> 주먹)
    const glm::vec3 lShLocal{ leftShoulder.x, leftShoulder.y - legLength, leftShoulder.z };
    p.leftArm = glm::translate(waist, lShLocal);
    p.leftArm = glm::rotate(p.leftArm, glm::radians(r.lShoulderX), xAxis);
    p.leftArm = glm::rotate(p.leftArm, glm::radians(r.lShoulderZ), zAxis);

    p.leftForearm = glm::translate(p.leftArm, glm::vec3(0.0f, -upperArmLen, 0.0f));
    p.leftForearm = glm::rotate(p.leftForearm, glm::radians(r.lElbowX), xAxis);

    p.leftHand = glm::translate(p.leftForearm, glm::vec3(0.0f, -(lowerArmLen + fistSize * 0.4f), 0.0f));

    // [4] 오른팔 (-X: 어깨 -> 상완 -> 팔꿈치 -> 하완 -> 주먹)
    const glm::vec3 rShLocal{ rightShoulder.x, rightShoulder.y - legLength, rightShoulder.z };
    p.rightArm = glm::translate(waist, rShLocal);
    p.rightArm = glm::rotate(p.rightArm, glm::radians(r.rShoulderX), xAxis);
    p.rightArm = glm::rotate(p.rightArm, glm::radians(r.rShoulderZ), zAxis);

    p.rightForearm = glm::translate(p.rightArm, glm::vec3(0.0f, -upperArmLen, 0.0f));
    p.rightForearm = glm::rotate(p.rightForearm, glm::radians(r.rElbowX), xAxis);

    p.rightHand = glm::translate(p.rightForearm, glm::vec3(0.0f, -(lowerArmLen + fistSize * 0.4f), 0.0f));

    // [5] 왼다리 (+X: 골반 -> 허벅지 -> 무릎 -> 정강이 -> 발)
    p.leftLeg = glm::translate(p.pelvis, glm::vec3(leftHip.x, 0.0f, leftHip.z));
    p.leftLeg = glm::rotate(p.leftLeg, glm::radians(r.lHipZ), zAxis);
    p.leftLeg = glm::rotate(p.leftLeg, glm::radians(r.lHipX), xAxis);

    p.leftCalf = glm::translate(p.leftLeg, glm::vec3(0.0f, -thighLen, 0.0f));
    p.leftCalf = glm::rotate(p.leftCalf, glm::radians(r.lKneeX), xAxis);

    p.leftFoot = glm::translate(p.leftCalf, glm::vec3(0.0f, -calfLen, 0.0f));
    p.leftFoot = glm::rotate(p.leftFoot, glm::radians(r.lAnkleX), xAxis);

    // [6] 오른다리 (-X: 골반 -> 허벅지 -> 무릎 -> 정강이 -> 발)
    p.rightLeg = glm::translate(p.pelvis, glm::vec3(rightHip.x, 0.0f, rightHip.z));
    p.rightLeg = glm::rotate(p.rightLeg, glm::radians(r.rHipZ), zAxis);
    p.rightLeg = glm::rotate(p.rightLeg, glm::radians(r.rHipX), xAxis);

    p.rightCalf = glm::translate(p.rightLeg, glm::vec3(0.0f, -thighLen, 0.0f));
    p.rightCalf = glm::rotate(p.rightCalf, glm::radians(r.rKneeX), xAxis);

    p.rightFoot = glm::translate(p.rightCalf, glm::vec3(0.0f, -calfLen, 0.0f));
    p.rightFoot = glm::rotate(p.rightFoot, glm::radians(r.rAnkleX), xAxis);

    return p;
}

void DrawRobot(const RobotParts& parts, const RobotPalette& palette)
{
    using namespace RobotSpec;

    DrawCube(glm::scale(parts.torso, torsoSize), palette.torso);
    DrawCube(glm::scale(parts.head, headSize), palette.head);
    DrawCube(glm::scale(glm::translate(parts.head, noseOffset), noseSize), palette.nose);

    auto drawSegment = [](const glm::mat4& joint, float length, float thick, const glm::vec3& color)
    {
        glm::mat4 m = glm::translate(joint, glm::vec3(0.0f, -length * 0.5f, 0.0f));
        m = glm::scale(m, glm::vec3(thick, length, thick));
        DrawCube(m, color);
    };

    // 관절 구분이 잘 보이도록 하완/정강이/주먹에 살짝 명암 변주 적용
    const glm::vec3 lForearmCol = palette.leftArm * 0.80f;
    const glm::vec3 rForearmCol = palette.rightArm * 0.80f;
    const glm::vec3 fistCol = palette.nose;
    const glm::vec3 lCalfCol = palette.leftLeg * 0.82f;
    const glm::vec3 rCalfCol = palette.rightLeg * 0.82f;

    // 왼팔 / 오른팔 (상완 -> 하완 -> 주먹)
    drawSegment(parts.leftArm, upperArmLen, armThick, palette.leftArm);
    drawSegment(parts.leftForearm, lowerArmLen, armThick * 0.88f, lForearmCol);
    DrawCube(glm::scale(parts.leftHand, glm::vec3(fistSize * parts.lFistScale)), fistCol);

    drawSegment(parts.rightArm, upperArmLen, armThick, palette.rightArm);
    drawSegment(parts.rightForearm, lowerArmLen, armThick * 0.88f, rForearmCol);
    DrawCube(glm::scale(parts.rightHand, glm::vec3(fistSize * parts.rFistScale)), fistCol);

    // 왼다리 / 오른다리 (허벅지 -> 정강이 -> 발)
    drawSegment(parts.leftLeg, thighLen, legThick, palette.leftLeg);
    drawSegment(parts.leftCalf, calfLen, legThick * 0.88f, lCalfCol);
    DrawCube(glm::scale(glm::translate(parts.leftFoot, glm::vec3(0.0f, -footHeight * 0.5f, 0.06f)), footSize), fistCol);

    drawSegment(parts.rightLeg, thighLen, legThick, palette.rightLeg);
    drawSegment(parts.rightCalf, calfLen, legThick * 0.88f, rCalfCol);
    DrawCube(glm::scale(glm::translate(parts.rightFoot, glm::vec3(0.0f, -footHeight * 0.5f, 0.06f)), footSize * parts.rFootScale), fistCol);
}

void FaceTowards(Robot& r, const glm::vec3& targetPos, float turnSpeedDeg, float dt)
{
    const glm::vec3 d = targetPos - r.pos;
    if (d.x * d.x + d.z * d.z < 1e-6f) return;

    const float wantDeg = glm::degrees(std::atan2(d.x, d.z));
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

    const glm::vec3 dir = (dist > 1e-4f) ? (d / dist) : glm::vec3(0.0f, 0.0f, 1.0f);
    const float push = (minDist - dist) * 0.5f;

    a.pos -= dir * push;
    b.pos += dir * push;

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

// ★ 다관절 적용: 팔꿈치가 꺾여 있어도 실제 주먹 정중앙 월드 좌표를 정확히 반환!
glm::vec3 RobotHandPos(const RobotParts& parts, bool left)
{
    return glm::vec3((left ? parts.leftHand : parts.rightHand)[3]);
}

// ★ 다관절 적용: 무릎이 꺾여 있어도 실제 발 정중앙 월드 좌표를 정확히 반환!
glm::vec3 RobotFootPos(const RobotParts& parts, bool left)
{
    const glm::mat4& foot = left ? parts.leftFoot : parts.rightFoot;
    return glm::vec3(foot * glm::vec4(0.0f, -RobotSpec::footHeight * 0.5f, 0.06f, 1.0f));
}