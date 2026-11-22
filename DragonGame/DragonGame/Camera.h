#pragma once

#include "Common.h"

// ============================================================
//  Camera.h  -  시점 관리 (담당: 선홍재)
//
//  제안서의 차별점인 "필살기 카메라 연출" 이 들어올 자리.
//  3인칭 -> 1인칭(눈) -> 상대 얼굴 앞 -> 3인칭 복귀 를 모드로 나눠 두고,
//  모드 사이 전환은 위치/주시점을 보간해서 부드럽게 만든다.
//
//  [다른 모듈과의 경계]
//   Robot 의 상태를 직접 고치지 않고, 읽기만 한다. (Robot.h 의 조회 함수들)
// ============================================================

struct Robot;
struct RobotParts;

enum class CameraMode
{
    Follow,         // 3인칭 추적 (기본)
    FirstPerson,    // 1인칭 - 필살기 발동 순간   (4단계에서 구현)
    VictimFace,     // 상대 얼굴 앞 - 타격 순간   (4단계에서 구현)
};

struct Camera
{
    glm::vec3 pos{ 0.0f, 7.0f, 17.0f };
    glm::vec3 target{ 0.0f, 1.5f, 0.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };

    CameraMode mode{ CameraMode::Follow };
    float      fovDeg{ 45.0f };

    // 3인칭 추적 파라미터 (로봇 발바닥 기준)
    // 정확히 뒤에서 보면 플레이어 몸이 상대를 가린다.
    // 그래서 어깨 너머(TPS)로 옆으로 비껴 서서 본다.
    float followHeight{ 3.8f };
    float followDist{ 5.5f };
    float followSide{ -1.5f };   // 음수 = 카메라가 화면 오른쪽으로 -> 플레이어가 화면 왼쪽에 선다

    // 목표 지점을 따라가는 속도. 클수록 딱 붙고, 작을수록 느긋하게 따라온다.
    float posLambda{ 12.0f };
    float targetLambda{ 18.0f };

    // true 면 이번 프레임은 보간 없이 목표 위치로 바로 이동한다 (리셋/연출 시작 직후)
    bool snapNext{ true };
};

void ResetCamera(Camera& cam);

void UpdateCamera(Camera& cam, const Robot& player, const RobotParts& playerParts, float dt);

// view / projection 을 만들어 셰이더에 올린다
void ApplyCamera(const Camera& cam, int fbWidth, int fbHeight);
