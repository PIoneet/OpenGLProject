#pragma once

#include "Common.h"

// ============================================================
//  Input.h  -  키 입력 폴링
//
//  이동처럼 "누르고 있는 동안 계속" 동작해야 하는 입력은 매 프레임 상태를 읽는다.
//  (glfwSetCharCallback 같은 이벤트 방식은 OS 의 키 반복에 의존해서
//   첫 입력 후 잠깐 끊기고, 두 키를 동시에 누르는 대각선 이동도 안 된다)
//
//  점프/필살기/리셋처럼 한 번만 반응해야 하는 입력은
//  "이번 프레임에 새로 눌렸는가"(엣지)로 따로 구분한다.
// ============================================================

struct InputState
{
    glm::vec2 move{ 0.0f };     // x : 좌(-1) / 우(+1),  y : 뒤(-1) / 앞(+1)
    float     turn{ 0.0f };     // 좌(-1) / 우(+1) 회전

    bool jumpPressed{ false };      // Space
    bool specialPressed{ false };   // K   (필살기 - 아직 비어 있음)
    bool resetPressed{ false };     // I
    bool quitPressed{ false };      // Q / ESC

    bool screenshotPressed{ false };         // F12 : 화면을 BMP 로 저장

    // 임시(3단계에서 공격 판정이 들어오면 제거) : 체력 바 동작 확인용
    bool debugDamageEnemyPressed{ false };   // H
    bool debugDamagePlayerPressed{ false };  // G
};

void PollInput(GLFWwindow* window, InputState& out);
