#include "Camera.h"
#include "Robot.h"
#include "Renderer.h"
#include "Stage.h"

namespace
{
    // 3인칭 추적 시점의 "목표" 위치와 주시점.
    // 로봇의 root 행렬에 로컬 오프셋을 곱하기 때문에, 로봇이 회전하면 카메라도 따라 돈다.
    void FollowDesired(const Camera& cam, const Robot& player,
        glm::vec3& outPos, glm::vec3& outTarget)
    {
        const glm::mat4 root = RobotRootMatrix(player);

        outPos = glm::vec3(root * glm::vec4(cam.followSide, cam.followHeight, -cam.followDist, 1.0f));

        // 로봇보다 한참 앞(상대가 서 있는 쪽)을 본다
        // -> 플레이어가 화면 아래쪽, 상대가 화면 가운데에 놓인다
        const glm::vec3 lookLocal = RobotSpec::headCenter + glm::vec3(0.0f, -0.9f, 3.2f);
        outTarget = glm::vec3(root * glm::vec4(lookLocal, 1.0f));

        // 로봇이 벽에 붙거나 점프하면 카메라가 벽/천장 밖으로 밀려난다.
        // 그대로 두면 방이 뚫려 보이므로 방 안으로 당겨 둔다.
        ClampCameraInsideRoom(outPos, 0.3f);
    }
}

void ResetCamera(Camera& cam)
{
    cam.mode = CameraMode::Follow;
    cam.up = glm::vec3(0.0f, 1.0f, 0.0f);
    cam.fovDeg = 45.0f;
    cam.snapNext = true;    // 다음 갱신에서 목표 위치로 즉시 이동
}

void UpdateCamera(Camera& cam, const Robot& player, const RobotParts& playerParts, float dt)
{
    glm::vec3 desiredPos{ cam.pos };
    glm::vec3 desiredTarget{ cam.target };

    switch (cam.mode)
    {
    case CameraMode::Follow:
        FollowDesired(cam, player, desiredPos, desiredTarget);
        break;

    case CameraMode::FirstPerson:
        // TODO(4단계) : 필살기 발동 순간. 눈 위치에서 상대를 바라본다.
        //   desiredPos    = RobotEyePos(playerParts);
        //   desiredTarget = 상대 머리 위치;
        FollowDesired(cam, player, desiredPos, desiredTarget);
        break;

    case CameraMode::VictimFace:
        // TODO(4단계) : 타격 순간. 맞는 상대의 얼굴 앞으로 이동한다.
        FollowDesired(cam, player, desiredPos, desiredTarget);
        break;
    }

    if (cam.snapNext)
    {
        cam.pos = desiredPos;
        cam.target = desiredTarget;
        cam.snapNext = false;
    }
    else
    {
        // 프레임레이트에 상관없이 같은 느낌으로 따라붙는 지수 보간.
        // 로봇이 급하게 꺾어도 카메라가 튀지 않는다.
        cam.pos += (desiredPos - cam.pos) * SmoothFactor(cam.posLambda, dt);
        cam.target += (desiredTarget - cam.target) * SmoothFactor(cam.targetLambda, dt);
    }

    (void)playerParts;  // 1인칭/얼굴 앞 연출에서 사용 예정
}

void ApplyCamera(const Camera& cam, int fbWidth, int fbHeight)
{
    const glm::mat4 view = glm::lookAt(cam.pos, cam.target, cam.up);

    const float aspect = (fbHeight > 0)
        ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight)
        : 1.0f;
    const glm::mat4 proj = glm::perspective(glm::radians(cam.fovDeg), aspect, 0.1f, 100.0f);

    SetCameraUniforms(view, proj, cam.pos);
}
