#include "Hud.h"
#include "Robot.h"
#include "Renderer.h"

namespace
{
    // 화면 크기가 달라져도 비율이 유지되도록, 폭의 일정 비율로 치수를 잡는다.
    const float barWidthRatio = 0.38f;   // 화면 폭 대비 바 하나의 길이
    const float barHeightRatio = 0.028f; // 화면 높이 대비 바 두께
    const float marginRatio = 0.03f;     // 화면 가장자리 여백

    const glm::vec3 frameColor{ 0.06f, 0.06f, 0.08f };   // 바깥 테두리
    const glm::vec3 emptyColor{ 0.22f, 0.20f, 0.24f };   // 깎여 나간 부분
    const glm::vec3 playerColor{ 0.25f, 0.80f, 0.35f };  // 플레이어 체력
    const glm::vec3 enemyColor{ 0.85f, 0.25f, 0.30f };   // 적 체력
    const glm::vec3 dangerColor{ 0.95f, 0.70f, 0.15f };  // 30% 이하 경고색

    float HealthRatio(const Robot& r)
    {
        if (r.maxHp <= 0) return 0.0f;
        return glm::clamp(static_cast<float>(r.hp) / static_cast<float>(r.maxHp), 0.0f, 1.0f);
    }

    // anchorRight 가 true 면 바가 오른쪽 끝에 붙어서 오른쪽으로 줄어든다.
    // (격투 게임에서 1P 는 왼쪽, 2P 는 오른쪽으로 깎이는 그 모양)
    void DrawBar(float x, float y, float w, float h,
        float ratio, const glm::vec3& fill, bool anchorRight)
    {
        const float border = glm::max(h * 0.18f, 2.0f);

        DrawUIRect(x - border, y - border, w + border * 2.0f, h + border * 2.0f, frameColor);
        DrawUIRect(x, y, w, h, emptyColor);

        const float fillW = w * ratio;
        if (fillW <= 0.0f) return;

        const float fillX = anchorRight ? (x + w - fillW) : x;
        const glm::vec3 color = (ratio <= 0.3f) ? dangerColor : fill;

        DrawUIRect(fillX, y, fillW, h, color);
    }
}

void DrawHud(const Robot& player, const Robot& enemy, int fbWidth, int fbHeight)
{
    const float w = static_cast<float>(fbWidth);
    const float h = static_cast<float>(fbHeight);

    const float barW = w * barWidthRatio;
    const float barH = h * barHeightRatio;
    const float margin = w * marginRatio;
    const float top = h * 0.045f;

    BeginUI(fbWidth, fbHeight);

    // 왼쪽 = 플레이어 (왼쪽으로 깎인다)
    DrawBar(margin, top, barW, barH, HealthRatio(player), playerColor, false);

    // 오른쪽 = 적 (오른쪽으로 깎인다)
    DrawBar(w - margin - barW, top, barW, barH, HealthRatio(enemy), enemyColor, true);

    EndUI();
}
