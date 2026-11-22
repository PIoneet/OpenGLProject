#pragma once

#include "Common.h"

// ============================================================
//  Hud.h  -  화면 고정 UI
//
//  폰트를 아직 쓰지 않으므로, 체력은 격투 게임처럼 좌우 체력 바로 보여 준다.
//  (왼쪽 = 플레이어, 오른쪽 = 적)
// ============================================================

struct Robot;

void DrawHud(const Robot& player, const Robot& enemy, int fbWidth, int fbHeight);
