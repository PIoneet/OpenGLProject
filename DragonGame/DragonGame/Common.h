#pragma once

// ============================================================
//  Common.h  -  모든 모듈이 함께 쓰는 헤더와 상수
//  (라이브러리 include 순서를 여기 한 곳에서만 관리한다)
// ============================================================

#include <GL/glew.h>        // glew 가 항상 gl 보다 먼저 와야 한다
#include <GL/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>

// 연습장(스테이지) 크기 - 스테이지 그리기와 로봇 이동 제한이 함께 쓴다
const float roomW = 22.0f;      // 가로 (x)
const float roomD = 22.0f;      // 깊이 (z)
const float roomH = 8.0f;       // 높이 (y)
// 12x12 로는 3인칭 카메라(로봇 뒤 약 5)를 둘 자리가 없어 벽에 끼었다.

// 프레임레이트에 상관없이 같은 속도로 "목표값에 다가가는" 보간.
//  - lambda 가 클수록 빠르게 따라붙는다.
//  - 필살기 카메라 연출의 부드러운 전환에도 이 함수를 쓴다.
inline float SmoothFactor(float lambda, float dt)
{
    return 1.0f - std::exp(-lambda * dt);
}
