#include "Input.h"

namespace
{
    // 직전 프레임에 눌려 있었는지 기억해 두고, "안 눌림 -> 눌림" 인 순간만 true 로 만든다.
    bool gPrevJump{ false };
    bool gPrevSpecial{ false };
    bool gPrevReset{ false };
    bool gPrevDmgEnemy{ false };
    bool gPrevDmgPlayer{ false };
    bool gPrevShot{ false };

    bool Down(GLFWwindow* w, int key)
    {
        return glfwGetKey(w, key) == GLFW_PRESS;
    }

    // 눌린 순간 한 번만 true (엣지 검출)
    bool JustPressed(bool now, bool& prev)
    {
        const bool edge = now && !prev;
        prev = now;
        return edge;
    }
}

void PollInput(GLFWwindow* window, InputState& out)
{
    out = InputState{};

    // ---- 이동 (로봇이 바라보는 방향 기준) ----
    if (Down(window, GLFW_KEY_W)) out.move.y += 1.0f;   // 앞
    if (Down(window, GLFW_KEY_S)) out.move.y -= 1.0f;   // 뒤
    if (Down(window, GLFW_KEY_A)) out.move.x -= 1.0f;   // 왼쪽
    if (Down(window, GLFW_KEY_D)) out.move.x += 1.0f;   // 오른쪽

    // 대각선이 직선보다 빠르지 않도록 길이를 1 로 맞춘다
    if (glm::dot(out.move, out.move) > 1.0f)
        out.move = glm::normalize(out.move);

    // ---- 방향 회전 ----
    if (Down(window, GLFW_KEY_LEFT))  out.turn -= 1.0f;
    if (Down(window, GLFW_KEY_RIGHT)) out.turn += 1.0f;

    // ---- 한 번만 반응하는 입력 ----
    out.jumpPressed = JustPressed(Down(window, GLFW_KEY_SPACE), gPrevJump);
    out.specialPressed = JustPressed(Down(window, GLFW_KEY_K), gPrevSpecial);
    out.resetPressed = JustPressed(Down(window, GLFW_KEY_I), gPrevReset);

    out.quitPressed = Down(window, GLFW_KEY_Q) || Down(window, GLFW_KEY_ESCAPE);

    out.screenshotPressed = JustPressed(Down(window, GLFW_KEY_F12), gPrevShot);

    // 임시 : 공격 판정이 없는 동안 체력 바를 확인하기 위한 키
    out.debugDamageEnemyPressed = JustPressed(Down(window, GLFW_KEY_H), gPrevDmgEnemy);
    out.debugDamagePlayerPressed = JustPressed(Down(window, GLFW_KEY_G), gPrevDmgPlayer);
}
