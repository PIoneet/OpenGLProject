#include "Stage.h"
#include "Renderer.h"

void DrawStage()
{
    const glm::vec3 X{ 1.0f, 0.0f, 0.0f };
    const glm::vec3 Y{ 0.0f, 1.0f, 0.0f };

    // 벽은 "안쪽이 앞면(CCW)" 이 되도록 돌려 놓는다.
    // 그래서 바깥에서 보면 컬링되어 사라지고, 방 안에서는 제대로 보인다.
    DrawWall(glm::vec3(0.0f, roomH / 2, -roomD / 2), Y, 0.0f,
        roomW, roomH, glm::vec3(0.75f, 0.28f, 0.28f));

    DrawWall(glm::vec3(0.0f, roomH / 2, roomD / 2), Y, 180.0f,
        roomW, roomH, glm::vec3(0.90f, 0.80f, 0.30f));

    DrawWall(glm::vec3(-roomW / 2, roomH / 2, 0.0f), Y, 90.0f,
        roomD, roomH, glm::vec3(0.30f, 0.60f, 0.35f));

    DrawWall(glm::vec3(roomW / 2, roomH / 2, 0.0f), Y, -90.0f,
        roomD, roomH, glm::vec3(0.25f, 0.30f, 0.70f));

    // 바닥 : x축 기준 -90도. 평면의 법선(+z)이 위(+y)를 향하게 된다.
    // (+90도로 돌리면 법선이 아래를 향해서, 위에서 내려다볼 때 뒷면이 되어 컬링된다)
    DrawWall(glm::vec3(0.0f, 0.0f, 0.0f), X, -90.0f,
        roomW, roomD, glm::vec3(0.45f, 0.58f, 0.42f));

    // 천장 : +90도 -> 법선이 아래(-y)를 향한다
    DrawWall(glm::vec3(0.0f, roomH, 0.0f), X, 90.0f,
        roomW, roomD, glm::vec3(0.80f, 0.80f, 0.84f));
}

void ClampInsideRoom(glm::vec3& pos, float radius)
{
    const float halfW = roomW * 0.5f - radius;
    const float halfD = roomD * 0.5f - radius;

    pos.x = glm::clamp(pos.x, -halfW, halfW);
    pos.z = glm::clamp(pos.z, -halfD, halfD);
}

void ClampCameraInsideRoom(glm::vec3& pos, float margin)
{
    pos.x = glm::clamp(pos.x, -roomW * 0.5f + margin, roomW * 0.5f - margin);
    pos.z = glm::clamp(pos.z, -roomD * 0.5f + margin, roomD * 0.5f - margin);
    pos.y = glm::clamp(pos.y, margin, roomH - margin);
}

bool IsNearWall(const glm::vec3& pos, float margin)
{
    const float halfW = roomW * 0.5f;
    const float halfD = roomD * 0.5f;

    return (halfW - std::abs(pos.x) <= margin)
        || (halfD - std::abs(pos.z) <= margin);
}

glm::vec3 StageLightPos()
{
    // 정가운데 바로 위에 두면 수직면에 빛이 거의 안 닿는다.
    // 살짝 앞(-z)으로 밀어서 마주 선 두 로봇의 앞면에도 빛이 걸리게 한다.
    return glm::vec3(0.0f, roomH - 1.5f, -2.0f);
}

glm::vec3 StageLightColor()
{
    return glm::vec3(1.0f, 0.97f, 0.92f);
}
