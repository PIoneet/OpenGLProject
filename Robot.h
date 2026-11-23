#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>

enum RobotState {
    STATE_IDLE,
    STATE_MOVE,
    STATE_DODGE,
    STATE_ATTACK,
    STATE_HIT,
    STATE_DOWN,
    STATE_GRAB,
    STATE_ULTIMATE
};

class Robot {
public:
    glm::vec3 position = glm::vec3(0.0f, 2.4f, 0.0f);
    float bodyYaw = 0.0f;
    glm::vec3 mainColor = glm::vec3(0.2f, 0.45f, 0.85f);

    float idleTimer = 0.0f;
    float bodyBobOffsetY = 0.0f;

    float walkTimer = 0.0f;
    float moveSpeed = 3.5f;
    float turnSpeed = 120.0f;

    RobotState currentState = STATE_IDLE;
    float stateTimer = 0.0f;
    float stateDuration = 0.0f;

    // [공격 및 콤보 시스템 변수]
    int attackType = 0;
    int comboStep = 0;
    bool comboQueued = false;
    float comboWindowTimer = 0.0f;
    bool isAttackActive = false;

    // [충돌 판정용 월드 좌표]
    glm::vec3 rFistWorldPos = glm::vec3(0.0f);
    glm::vec3 lFistWorldPos = glm::vec3(0.0f);
    glm::vec3 rFootWorldPos = glm::vec3(0.0f);

    // [하체/상체 분리 회전 및 3축 관절 각도]
    float spinYawOffset = 0.0f;
    float pelvisTwistY = 0.0f;
    float torsoTwistY = 0.0f;
    float torsoPitchX = 0.0f;
    float torsoRollZ = 0.0f;
    float headYaw = 0.0f, headPitch = 0.0f;

    float rShoulderX = 0.0f, rShoulderZ = 0.0f, rElbowX = 0.0f;
    float lShoulderX = 0.0f, lShoulderZ = 0.0f, lElbowX = 0.0f;

    float rHipX = 0.0f, rHipZ = 0.0f, rKneeX = 0.0f, rAnkleX = 0.0f;
    float lHipX = 0.0f, lHipZ = 0.0f, lKneeX = 0.0f, lAnkleX = 0.0f;

    float rFistScale = 1.0f;
    float lFistScale = 1.0f;
    float rFootScale = 1.0f;

    float lerp(float start, float end, float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return start + (end - start) * t;
    }

    float easeIn(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t;
    }

    float easeOut(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    float easeOutQuart(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        float u = 1.0f - t;
        return 1.0f - u * u * u * u;
    }

    float easeOutBack(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        float c1 = 1.70158f;
        float c3 = c1 + 1.0f;
        float u = t - 1.0f;
        return 1.0f + c3 * u * u * u + c1 * u * u;
    }

    void stepForward(float dt, float speed) {
        float rad = glm::radians(bodyYaw);
        glm::vec3 forward(sin(rad), 0.0f, cos(rad));
        position += forward * speed * dt;
        position.x = glm::clamp(position.x, -14.0f, 14.0f);
        position.z = glm::clamp(position.z, -14.0f, 14.0f);
    }

    void resetCombatModifiers() {
        spinYawOffset = 0.0f;
        pelvisTwistY = 0.0f;
        torsoPitchX = 0.0f;
        torsoRollZ = 0.0f;
        headPitch = 0.0f;
        rHipZ = 0.0f;
        lHipZ = 0.0f;
        rFistScale = 1.0f;
        lFistScale = 1.0f;
        rFootScale = 1.0f;
    }

    void changeState(RobotState newState, float duration = 0.0f) {
        currentState = newState;
        stateTimer = 0.0f;
        stateDuration = duration;
    }

    void updateIdle(float dt) {
        resetCombatModifiers();
        idleTimer += dt * 4.5f;

        float bob = sin(idleTimer);
        bodyBobOffsetY = bob * 0.04f;

        torsoTwistY = 20.0f;
        headYaw = -20.0f;

        rShoulderX = -45.0f + bob * 3.0f;
        rShoulderZ = 15.0f;
        rElbowX = -100.0f - bob * 3.0f;

        lShoulderX = -60.0f - bob * 3.0f;
        lShoulderZ = -15.0f;
        lElbowX = -85.0f + bob * 3.0f;

        rHipX = 12.0f;  rKneeX = 20.0f; rAnkleX = -8.0f;
        lHipX = -15.0f; lKneeX = 20.0f; lAnkleX = -5.0f;
    }

    void updateMove(float dt, int moveDir, int turnDir) {
        resetCombatModifiers();

        if (turnDir != 0) {
            bodyYaw += turnDir * turnSpeed * dt;
        }

        if (moveDir == 0) {
            if (turnDir != 0) {
                walkTimer += dt * 8.0f;
                rHipX = 12.0f + sin(walkTimer) * 10.0f;
                lHipX = -15.0f - sin(walkTimer) * 10.0f;
            }
            else {
                updateIdle(dt);
            }
            return;
        }

        stepForward(dt, (float)moveDir * moveSpeed);

        walkTimer += dt * 10.0f * moveDir;
        float swing = sin(walkTimer);
        float bounce = fabs(cos(walkTimer));

        bodyBobOffsetY = bounce * 0.08f;
        torsoPitchX = (moveDir > 0) ? 8.0f : -5.0f;
        torsoTwistY = 10.0f + swing * 12.0f;
        headYaw = -torsoTwistY;

        rHipX = swing * 35.0f;
        lHipX = -swing * 35.0f;

        rKneeX = (swing > 0.0f) ? swing * 35.0f : 10.0f;
        lKneeX = (swing < 0.0f) ? -swing * 35.0f : 10.0f;
        rAnkleX = 0.0f;
        lAnkleX = 0.0f;

        rShoulderX = -45.0f - swing * 20.0f;
        rShoulderZ = 15.0f;
        rElbowX = -95.0f;

        lShoulderX = -55.0f + swing * 20.0f;
        lShoulderZ = -15.0f;
        lElbowX = -85.0f;
    }

    void startDodge() {
        if (currentState == STATE_IDLE || currentState == STATE_MOVE) {
            resetCombatModifiers();
            changeState(STATE_DODGE, 0.35f);
        }
    }

    void updateDodge(float dt) {
        float t = stateTimer / stateDuration;
        stepForward(dt, 10.0f * (1.0f - t));

        if (t < 0.5f) {
            float subT = easeOut(t / 0.5f);
            bodyBobOffsetY = lerp(0.0f, -0.35f, subT);
            torsoPitchX = lerp(0.0f, 30.0f, subT);
            torsoTwistY = lerp(20.0f, -25.0f, subT);
            headYaw = -torsoTwistY;
            rHipX = lerp(12.0f, -40.0f, subT); rKneeX = lerp(20.0f, 65.0f, subT);
            lHipX = lerp(-15.0f, 35.0f, subT); lKneeX = lerp(20.0f, 45.0f, subT);
            rShoulderX = lerp(-45.0f, -20.0f, subT);
            lShoulderX = lerp(-60.0f, -80.0f, subT);
        }
        else {
            float subT = easeIn((t - 0.5f) / 0.5f);
            bodyBobOffsetY = lerp(-0.35f, 0.0f, subT);
            torsoPitchX = lerp(30.0f, 0.0f, subT);
            torsoTwistY = lerp(-25.0f, 20.0f, subT);
            headYaw = -torsoTwistY;
            rHipX = lerp(-40.0f, 12.0f, subT); rKneeX = lerp(65.0f, 20.0f, subT);
            lHipX = lerp(35.0f, -15.0f, subT); lKneeX = lerp(45.0f, 20.0f, subT);
            rShoulderX = lerp(-20.0f, -45.0f, subT);
            lShoulderX = lerp(-80.0f, -60.0f, subT);
        }
    }

    void beginAttackStep(int type, int step) {
        resetCombatModifiers();
        attackType = type;
        comboStep = step;
        comboQueued = false;
        isAttackActive = false;

        if (type == 1) {
            if (step == 1) changeState(STATE_ATTACK, 0.30f);
            else if (step == 2) changeState(STATE_ATTACK, 0.40f);
            else changeState(STATE_ATTACK, 0.56f);
        }
        else if (type == 2) {
            if (step == 1) changeState(STATE_ATTACK, 0.52f);
            else changeState(STATE_ATTACK, 0.62f);
        }
    }

    void startPunch() {
        if (currentState == STATE_IDLE || currentState == STATE_MOVE) {
            int nextStep = (attackType == 1 && comboWindowTimer > 0.0f) ? (comboStep % 3) + 1 : 1;
            beginAttackStep(1, nextStep);
        }
        else if (currentState == STATE_ATTACK && attackType == 1 && comboStep < 3) {
            if (stateTimer / stateDuration >= 0.40f) {
                comboQueued = true;
            }
        }
    }

    void startKick() {
        if (currentState == STATE_IDLE || currentState == STATE_MOVE) {
            int nextStep = (attackType == 2 && comboWindowTimer > 0.0f) ? (comboStep % 2) + 1 : 1;
            beginAttackStep(2, nextStep);
        }
        else if (currentState == STATE_ATTACK && attackType == 2 && comboStep < 2) {
            if (stateTimer / stateDuration >= 0.45f) {
                comboQueued = true;
            }
        }
    }

    void updateAttack(float dt) {
        float t = stateTimer / stateDuration;

        if (attackType == 1) {
            if (comboStep == 1) {
                if (t < 0.20f) {
                    float s = easeIn(t / 0.20f);
                    isAttackActive = false;
                    torsoTwistY = lerp(20.0f, 5.0f, s);
                    headYaw = -torsoTwistY;
                    lShoulderX = lerp(-60.0f, -45.0f, s);
                    lElbowX = lerp(-85.0f, -105.0f, s);
                }
                else if (t < 0.55f) {
                    float s = easeOutQuart((t - 0.20f) / 0.35f);
                    isAttackActive = true;
                    stepForward(dt, 3.0f * (1.0f - s));
                    bodyBobOffsetY = lerp(0.0f, -0.08f, s);
                    torsoTwistY = lerp(5.0f, 42.0f, s);
                    torsoPitchX = lerp(0.0f, 8.0f, s);
                    headYaw = -torsoTwistY;
                    lShoulderX = lerp(-45.0f, -92.0f, s);
                    lShoulderZ = lerp(-15.0f, 0.0f, s);
                    lElbowX = lerp(-105.0f, 0.0f, s);
                    lFistScale = lerp(1.0f, 1.35f, sin(s * 3.14159f));
                    rShoulderX = lerp(-45.0f, -55.0f, s);
                    lHipX = lerp(-15.0f, -28.0f, s); lKneeX = lerp(20.0f, 35.0f, s);
                }
                else {
                    float s = easeIn((t - 0.55f) / 0.45f);
                    isAttackActive = false;
                    bodyBobOffsetY = lerp(-0.08f, 0.0f, s);
                    torsoTwistY = lerp(42.0f, 20.0f, s);
                    torsoPitchX = lerp(8.0f, 0.0f, s);
                    headYaw = -torsoTwistY;
                    lShoulderX = lerp(-92.0f, -60.0f, s);
                    lShoulderZ = lerp(0.0f, -15.0f, s);
                    lElbowX = lerp(0.0f, -85.0f, s);
                    lFistScale = 1.0f;
                    lHipX = lerp(-28.0f, -15.0f, s); lKneeX = lerp(35.0f, 20.0f, s);
                }
            }
            else if (comboStep == 2) {
                if (t < 0.25f) {
                    float s = easeIn(t / 0.25f);
                    isAttackActive = false;
                    bodyBobOffsetY = lerp(0.0f, -0.15f, s);
                    torsoTwistY = lerp(25.0f, 52.0f, s);
                    torsoRollZ = lerp(0.0f, -8.0f, s);
                    headYaw = -torsoTwistY;
                    rShoulderX = lerp(-45.0f, 20.0f, s);
                    rElbowX = lerp(-100.0f, -120.0f, s);
                    lShoulderX = lerp(-60.0f, -75.0f, s);
                }
                else if (t < 0.58f) {
                    float s = easeOutQuart((t - 0.25f) / 0.33f);
                    isAttackActive = true;
                    stepForward(dt, 5.2f * (1.0f - s));
                    bodyBobOffsetY = lerp(-0.15f, -0.22f, s);
                    pelvisTwistY = lerp(10.0f, -20.0f, s);
                    torsoTwistY = lerp(52.0f, -42.0f, s);
                    torsoPitchX = lerp(0.0f, 16.0f, s);
                    torsoRollZ = lerp(-8.0f, 10.0f, s);
                    headYaw = -(pelvisTwistY + torsoTwistY);
                    rShoulderX = lerp(20.0f, -95.0f, s);
                    rShoulderZ = lerp(15.0f, -5.0f, s);
                    rElbowX = lerp(-120.0f, 0.0f, s);
                    rFistScale = lerp(1.0f, 1.48f, sin(s * 3.14159f));
                    lShoulderX = lerp(-75.0f, 10.0f, s);
                    lElbowX = lerp(-85.0f, -110.0f, s);
                    lHipX = lerp(-15.0f, -35.0f, s); lKneeX = lerp(20.0f, 45.0f, s);
                    rHipX = lerp(12.0f, 35.0f, s);   rKneeX = lerp(20.0f, 15.0f, s);
                }
                else {
                    float s = easeIn((t - 0.58f) / 0.42f);
                    isAttackActive = false;
                    bodyBobOffsetY = lerp(-0.22f, 0.0f, s);
                    pelvisTwistY = lerp(-20.0f, 0.0f, s);
                    torsoTwistY = lerp(-42.0f, 20.0f, s);
                    torsoPitchX = lerp(16.0f, 0.0f, s);
                    torsoRollZ = lerp(10.0f, 0.0f, s);
                    headYaw = -(pelvisTwistY + torsoTwistY);
                    rShoulderX = lerp(-95.0f, -45.0f, s);
                    rShoulderZ = lerp(-5.0f, 15.0f, s);
                    rElbowX = lerp(0.0f, -100.0f, s);
                    rFistScale = 1.0f;
                    lShoulderX = lerp(10.0f, -60.0f, s);
                    lElbowX = lerp(-110.0f, -85.0f, s);
                    lHipX = lerp(-35.0f, -15.0f, s); lKneeX = lerp(45.0f, 20.0f, s);
                    rHipX = lerp(35.0f, 12.0f, s);   rKneeX = lerp(15.0f, 20.0f, s);
                }
            }
            else {
                if (t < 0.30f) {
                    float s = easeIn(t / 0.30f);
                    isAttackActive = false;
                    bodyBobOffsetY = lerp(0.0f, -0.45f, s);
                    torsoPitchX = lerp(0.0f, 30.0f, s);
                    torsoTwistY = lerp(20.0f, 55.0f, s);
                    torsoRollZ = lerp(0.0f, -20.0f, s);
                    headYaw = -torsoTwistY;
                    rShoulderX = lerp(-45.0f, 35.0f, s);
                    rShoulderZ = lerp(15.0f, 25.0f, s);
                    rElbowX = lerp(-100.0f, -75.0f, s);
                    lShoulderX = lerp(-60.0f, -35.0f, s);
                    rHipX = lerp(12.0f, -42.0f, s);  rKneeX = lerp(20.0f, 80.0f, s);
                    lHipX = lerp(-15.0f, -45.0f, s); lKneeX = lerp(20.0f, 80.0f, s);
                }
                else if (t < 0.62f) {
                    float s = easeOutBack((t - 0.30f) / 0.32f);
                    float sClamp = glm::clamp(s, 0.0f, 1.0f);
                    isAttackActive = true;
                    stepForward(dt, 4.5f * (1.0f - sClamp));
                    bodyBobOffsetY = lerp(-0.45f, 0.24f, s);
                    pelvisTwistY = lerp(10.0f, -25.0f, sClamp);
                    torsoPitchX = lerp(30.0f, -22.0f, s);
                    torsoTwistY = lerp(55.0f, -38.0f, sClamp);
                    torsoRollZ = lerp(-20.0f, 16.0f, sClamp);
                    headYaw = -(pelvisTwistY + torsoTwistY);
                    headPitch = lerp(10.0f, -20.0f, sClamp);
                    rShoulderX = lerp(35.0f, -150.0f, s);
                    rShoulderZ = lerp(25.0f, 5.0f, sClamp);
                    rElbowX = lerp(-75.0f, -65.0f, sClamp);
                    rFistScale = lerp(1.0f, 1.60f, sin(sClamp * 3.14159f));
                    lShoulderX = lerp(-35.0f, 25.0f, sClamp);
                    rHipX = lerp(-42.0f, 15.0f, sClamp);  rKneeX = lerp(80.0f, 5.0f, sClamp);
                    lHipX = lerp(-45.0f, -10.0f, sClamp); lKneeX = lerp(80.0f, 5.0f, sClamp);
                }
                else {
                    float s = easeIn((t - 0.62f) / 0.38f);
                    isAttackActive = false;
                    bodyBobOffsetY = lerp(0.24f, 0.0f, s);
                    pelvisTwistY = lerp(-25.0f, 0.0f, s);
                    torsoPitchX = lerp(-22.0f, 0.0f, s);
                    torsoTwistY = lerp(-38.0f, 20.0f, s);
                    torsoRollZ = lerp(16.0f, 0.0f, s);
                    headYaw = -(pelvisTwistY + torsoTwistY);
                    headPitch = lerp(-20.0f, 0.0f, s);
                    rShoulderX = lerp(-150.0f, -45.0f, s);
                    rShoulderZ = lerp(5.0f, 15.0f, s);
                    rElbowX = lerp(-65.0f, -100.0f, s);
                    rFistScale = 1.0f;
                    lShoulderX = lerp(25.0f, -60.0f, s);
                    rHipX = lerp(15.0f, 12.0f, s);   rKneeX = lerp(5.0f, 20.0f, s);
                    lHipX = lerp(-10.0f, -15.0f, s); lKneeX = lerp(5.0f, 20.0f, s);
                }
            }
        }
        else if (attackType == 2) {
            if (comboStep == 1) {
                if (t < 0.26f) {
                    float s = easeIn(t / 0.26f);
                    isAttackActive = false;
                    bodyBobOffsetY = lerp(0.0f, -0.14f, s);
                    pelvisTwistY = lerp(0.0f, 28.0f, s);
                    torsoTwistY = lerp(20.0f, 25.0f, s);
                    torsoRollZ = lerp(0.0f, 28.0f, s);
                    headYaw = -(pelvisTwistY + torsoTwistY);
                    rHipZ = lerp(0.0f, 70.0f, s);
                    rHipX = lerp(12.0f, -40.0f, s);
                    rKneeX = lerp(20.0f, 115.0f, s);
                    lHipX = lerp(-15.0f, -10.0f, s); lKneeX = lerp(20.0f, 35.0f, s);
                }
                else if (t < 0.58f) {
                    float s = easeOutQuart((t - 0.26f) / 0.32f);
                    isAttackActive = true;
                    stepForward(dt, 3.2f * (1.0f - s));
                    bodyBobOffsetY = lerp(-0.14f, -0.05f, s);
                    pelvisTwistY = lerp(28.0f, -52.0f, s);
                    torsoTwistY = lerp(25.0f, -10.0f, s);
                    torsoPitchX = lerp(0.0f, -12.0f, s);
                    torsoRollZ = lerp(28.0f, 38.0f, s);
                    headYaw = -(pelvisTwistY + torsoTwistY) * 0.85f;
                    rHipZ = lerp(70.0f, 95.0f, s);
                    rHipX = lerp(-40.0f, -35.0f, s);
                    rKneeX = lerp(115.0f, 0.0f, s);
                    rAnkleX = lerp(-8.0f, 35.0f, s);
                    rFootScale = lerp(1.0f, 1.55f, sin(s * 3.14159f));
                    rShoulderZ = lerp(15.0f, 55.0f, s);  rShoulderX = lerp(-45.0f, 15.0f, s);
                    lShoulderZ = lerp(-15.0f, -45.0f, s); lShoulderX = lerp(-60.0f, -20.0f, s);
                    lKneeX = lerp(35.0f, 18.0f, s);
                }
                else {
                    float s = easeIn((t - 0.58f) / 0.42f);
                    isAttackActive = false;
                    bodyBobOffsetY = lerp(-0.05f, 0.0f, s);
                    pelvisTwistY = lerp(-52.0f, 0.0f, s);
                    torsoTwistY = lerp(-10.0f, 20.0f, s);
                    torsoPitchX = lerp(-12.0f, 0.0f, s);
                    torsoRollZ = lerp(38.0f, 0.0f, s);
                    headYaw = -(pelvisTwistY + torsoTwistY);
                    rHipZ = lerp(95.0f, 0.0f, s);
                    rHipX = lerp(-35.0f, 12.0f, s);
                    rKneeX = lerp(0.0f, 20.0f, s);
                    rAnkleX = lerp(35.0f, -8.0f, s);
                    rFootScale = 1.0f;
                    rShoulderZ = lerp(55.0f, 15.0f, s);  rShoulderX = lerp(15.0f, -45.0f, s);
                    lShoulderZ = lerp(-45.0f, -15.0f, s); lShoulderX = lerp(-20.0f, -60.0f, s);
                    lHipX = lerp(-10.0f, -15.0f, s); lKneeX = lerp(18.0f, 20.0f, s);
                }
            }
            else {
                if (t < 0.22f) {
                    float s = easeIn(t / 0.22f);
                    isAttackActive = false;
                    bodyBobOffsetY = lerp(0.0f, -0.22f, s);
                    spinYawOffset = lerp(0.0f, 25.0f, s);
                    torsoTwistY = lerp(20.0f, 40.0f, s);
                    lKneeX = lerp(20.0f, 45.0f, s);
                    rKneeX = lerp(20.0f, 70.0f, s);
                }
                else if (t < 0.68f) {
                    float s = easeOutQuart((t - 0.22f) / 0.46f);
                    isAttackActive = true;
                    stepForward(dt, 4.0f * (1.0f - s));
                    bodyBobOffsetY = lerp(-0.22f, 0.12f, sin(s * 3.14159f));
                    spinYawOffset = lerp(25.0f, -360.0f, s);
                    torsoTwistY = lerp(40.0f, -20.0f, s);
                    torsoRollZ = lerp(0.0f, 42.0f, sin(s * 3.14159f));
                    torsoPitchX = lerp(0.0f, -15.0f, sin(s * 3.14159f));
                    headYaw = 0.0f;
                    rHipZ = lerp(20.0f, 102.0f, sin(s * 3.14159f));
                    rHipX = lerp(-20.0f, -45.0f, sin(s * 3.14159f));
                    rKneeX = lerp(70.0f, 0.0f, easeOut(s));
                    rAnkleX = lerp(0.0f, 35.0f, s);
                    rFootScale = lerp(1.0f, 1.65f, sin(s * 3.14159f));
                    rShoulderZ = lerp(15.0f, 70.0f, sin(s * 3.14159f));
                    lShoulderZ = lerp(-15.0f, -70.0f, sin(s * 3.14159f));
                    rElbowX = lerp(-100.0f, -30.0f, sin(s * 3.14159f));
                    lElbowX = lerp(-85.0f, -30.0f, sin(s * 3.14159f));
                }
                else {
                    float s = easeIn((t - 0.68f) / 0.32f);
                    isAttackActive = false;
                    spinYawOffset = -360.0f;
                    bodyBobOffsetY = lerp(0.0f, 0.0f, s);
                    torsoTwistY = lerp(-20.0f, 20.0f, s);
                    torsoRollZ = lerp(torsoRollZ, 0.0f, s);
                    torsoPitchX = lerp(torsoPitchX, 0.0f, s);
                    headYaw = -torsoTwistY;
                    rHipZ = lerp(rHipZ, 0.0f, s);
                    rHipX = lerp(rHipX, 12.0f, s);
                    rKneeX = lerp(rKneeX, 20.0f, s);
                    rAnkleX = lerp(35.0f, -8.0f, s);
                    rFootScale = 1.0f;
                    rShoulderZ = lerp(rShoulderZ, 15.0f, s);
                    lShoulderZ = lerp(lShoulderZ, -15.0f, s);
                    rElbowX = lerp(rElbowX, -100.0f, s);
                    lElbowX = lerp(lElbowX, -85.0f, s);
                    lKneeX = lerp(45.0f, 20.0f, s);
                }
            }
        }
    }

    void update(float dt, int moveDir, int turnDir) {
        stateTimer += dt;

        if (currentState != STATE_ATTACK && comboWindowTimer > 0.0f) {
            comboWindowTimer -= dt;
            if (comboWindowTimer <= 0.0f) {
                comboStep = 0;
            }
        }

        switch (currentState) {
        case STATE_IDLE:
        case STATE_MOVE:
            if (moveDir != 0 || turnDir != 0) {
                currentState = STATE_MOVE;
            }
            else {
                currentState = STATE_IDLE;
            }
            updateMove(dt, moveDir, turnDir);
            break;

        case STATE_DODGE:
            updateDodge(dt);
            if (stateTimer >= stateDuration) {
                resetCombatModifiers();
                changeState(STATE_IDLE);
            }
            break;

        case STATE_ATTACK:
            updateAttack(dt);
            if (comboQueued && (stateTimer / stateDuration >= 0.82f)) {
                beginAttackStep(attackType, comboStep + 1);
            }
            else if (stateTimer >= stateDuration) {
                isAttackActive = false;
                resetCombatModifiers();
                comboWindowTimer = 0.45f;
                changeState(STATE_IDLE);
            }
            break;

        default:
            break;
        }
    }

    void drawBox(glm::mat4 jointMat, glm::vec3 pivotOffset, glm::vec3 size, glm::vec3 color, unsigned int shaderID, unsigned int vao) {
        glm::mat4 renderMat = glm::translate(jointMat, pivotOffset);
        renderMat = glm::scale(renderMat, size);

        int modelLoc = glGetUniformLocation(shaderID, "uModel");
        int colorLoc = glGetUniformLocation(shaderID, "uColor");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderMat));
        glBindVertexArray(vao);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glUniform3fv(colorLoc, 1, glm::value_ptr(color));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDisable(GL_POLYGON_OFFSET_FILL);

        glUniform3f(colorLoc, 0.1f, 0.1f, 0.1f);
        glLineWidth(1.5f);
        glDrawArrays(GL_LINES, 36, 24);
    }

    void render(unsigned int shaderID, unsigned int vao) {
        glm::vec3 white(0.92f, 0.92f, 0.92f);
        glm::vec3 red(0.85f, 0.22f, 0.22f);
        glm::vec3 darkGray(0.28f, 0.28f, 0.32f);

        // [0] 골반 기준점 (Root: y = position.y - 0.6f = 1.8f)
        glm::vec3 pelvisPos = position + glm::vec3(0.0f, bodyBobOffsetY - 0.6f, 0.0f);
        glm::mat4 pelvisMat = glm::translate(glm::mat4(1.0f), pelvisPos);
        pelvisMat = glm::rotate(pelvisMat, glm::radians(bodyYaw + spinYawOffset + pelvisTwistY), glm::vec3(0, 1, 0));

        // [1] 상체 몸통 (골반 위에서 허리 3축 회전: Twist Y, Pitch X, Roll Z)
        glm::mat4 torsoMat = glm::rotate(pelvisMat, glm::radians(torsoTwistY), glm::vec3(0, 1, 0));
        torsoMat = glm::rotate(torsoMat, glm::radians(torsoPitchX), glm::vec3(1, 0, 0));
        torsoMat = glm::rotate(torsoMat, glm::radians(torsoRollZ), glm::vec3(0, 0, 1));
        drawBox(torsoMat, glm::vec3(0.0f, 0.6f, 0.0f), glm::vec3(1.0f, 1.2f, 0.5f), mainColor, shaderID, vao);

        // [2] 머리 (몸통 위 끝 y = +1.2f)
        glm::mat4 neckJoint = glm::translate(torsoMat, glm::vec3(0.0f, 1.2f, 0.0f));
        neckJoint = glm::rotate(neckJoint, glm::radians(headYaw), glm::vec3(0, 1, 0));
        neckJoint = glm::rotate(neckJoint, glm::radians(headPitch), glm::vec3(1, 0, 0));
        drawBox(neckJoint, glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f), white, shaderID, vao);
        drawBox(neckJoint, glm::vec3(0.0f, 0.3f, 0.26f), glm::vec3(0.4f, 0.12f, 0.05f), red, shaderID, vao);

        // [3] 오른팔 (어깨 y = +1.1f)
        glm::mat4 rShoulderJoint = glm::translate(torsoMat, glm::vec3(0.65f, 1.1f, 0.0f));
        rShoulderJoint = glm::rotate(rShoulderJoint, glm::radians(rShoulderX), glm::vec3(1, 0, 0));
        rShoulderJoint = glm::rotate(rShoulderJoint, glm::radians(rShoulderZ), glm::vec3(0, 0, 1));
        drawBox(rShoulderJoint, glm::vec3(0.0f, -0.35f, 0.0f), glm::vec3(0.3f, 0.7f, 0.3f), white, shaderID, vao);

        glm::mat4 rElbowJoint = glm::translate(rShoulderJoint, glm::vec3(0.0f, -0.7f, 0.0f));
        rElbowJoint = glm::rotate(rElbowJoint, glm::radians(rElbowX), glm::vec3(1, 0, 0));
        drawBox(rElbowJoint, glm::vec3(0.0f, -0.3f, 0.0f), glm::vec3(0.25f, 0.6f, 0.25f), mainColor, shaderID, vao);

        glm::mat4 rWristJoint = glm::translate(rElbowJoint, glm::vec3(0.0f, -0.6f, 0.0f));
        drawBox(rWristJoint, glm::vec3(0.0f, -0.15f, 0.0f), glm::vec3(0.32f * rFistScale), red, shaderID, vao);
        rFistWorldPos = glm::vec3(glm::translate(rWristJoint, glm::vec3(0.0f, -0.15f, 0.0f))[3]);

        // [4] 왼팔 (어깨 y = +1.1f)
        glm::mat4 lShoulderJoint = glm::translate(torsoMat, glm::vec3(-0.65f, 1.1f, 0.0f));
        lShoulderJoint = glm::rotate(lShoulderJoint, glm::radians(lShoulderX), glm::vec3(1, 0, 0));
        lShoulderJoint = glm::rotate(lShoulderJoint, glm::radians(lShoulderZ), glm::vec3(0, 0, 1));
        drawBox(lShoulderJoint, glm::vec3(0.0f, -0.35f, 0.0f), glm::vec3(0.3f, 0.7f, 0.3f), white, shaderID, vao);

        glm::mat4 lElbowJoint = glm::translate(lShoulderJoint, glm::vec3(0.0f, -0.7f, 0.0f));
        lElbowJoint = glm::rotate(lElbowJoint, glm::radians(lElbowX), glm::vec3(1, 0, 0));
        drawBox(lElbowJoint, glm::vec3(0.0f, -0.3f, 0.0f), glm::vec3(0.25f, 0.6f, 0.25f), mainColor, shaderID, vao);

        glm::mat4 lWristJoint = glm::translate(lElbowJoint, glm::vec3(0.0f, -0.6f, 0.0f));
        drawBox(lWristJoint, glm::vec3(0.0f, -0.15f, 0.0f), glm::vec3(0.32f * lFistScale), red, shaderID, vao);
        lFistWorldPos = glm::vec3(glm::translate(lWristJoint, glm::vec3(0.0f, -0.15f, 0.0f))[3]);

        // [5] 오른다리 (골반 pelvisMat에서 출발! Z축 옆들기 + X축 앞뒤 회전)
        glm::mat4 rHipJoint = glm::translate(pelvisMat, glm::vec3(0.3f, 0.0f, 0.0f));
        rHipJoint = glm::rotate(rHipJoint, glm::radians(rHipZ), glm::vec3(0, 0, 1));
        rHipJoint = glm::rotate(rHipJoint, glm::radians(rHipX), glm::vec3(1, 0, 0));
        drawBox(rHipJoint, glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.35f, 0.8f, 0.35f), darkGray, shaderID, vao);

        glm::mat4 rKneeJoint = glm::translate(rHipJoint, glm::vec3(0.0f, -0.8f, 0.0f));
        rKneeJoint = glm::rotate(rKneeJoint, glm::radians(rKneeX), glm::vec3(1, 0, 0));
        drawBox(rKneeJoint, glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.3f, 0.8f, 0.3f), white, shaderID, vao);

        glm::mat4 rAnkleJoint = glm::translate(rKneeJoint, glm::vec3(0.0f, -0.8f, 0.0f));
        rAnkleJoint = glm::rotate(rAnkleJoint, glm::radians(rAnkleX), glm::vec3(1, 0, 0));
        drawBox(rAnkleJoint, glm::vec3(0.0f, -0.1f, 0.1f), glm::vec3(0.35f, 0.2f, 0.5f) * rFootScale, red, shaderID, vao);
        rFootWorldPos = glm::vec3(glm::translate(rAnkleJoint, glm::vec3(0.0f, -0.1f, 0.1f))[3]);

        // [6] 왼다리 (골반 pelvisMat에서 출발!)
        glm::mat4 lHipJoint = glm::translate(pelvisMat, glm::vec3(-0.3f, 0.0f, 0.0f));
        lHipJoint = glm::rotate(lHipJoint, glm::radians(lHipZ), glm::vec3(0, 0, 1));
        lHipJoint = glm::rotate(lHipJoint, glm::radians(lHipX), glm::vec3(1, 0, 0));
        drawBox(lHipJoint, glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.35f, 0.8f, 0.35f), darkGray, shaderID, vao);

        glm::mat4 lKneeJoint = glm::translate(lHipJoint, glm::vec3(0.0f, -0.8f, 0.0f));
        lKneeJoint = glm::rotate(lKneeJoint, glm::radians(lKneeX), glm::vec3(1, 0, 0));
        drawBox(lKneeJoint, glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.3f, 0.8f, 0.3f), white, shaderID, vao);

        glm::mat4 lAnkleJoint = glm::translate(lKneeJoint, glm::vec3(0.0f, -0.8f, 0.0f));
        lAnkleJoint = glm::rotate(lAnkleJoint, glm::radians(lAnkleX), glm::vec3(1, 0, 0));
        drawBox(lAnkleJoint, glm::vec3(0.0f, -0.1f, 0.1f), glm::vec3(0.35f, 0.2f, 0.5f), red, shaderID, vao);
    }
};