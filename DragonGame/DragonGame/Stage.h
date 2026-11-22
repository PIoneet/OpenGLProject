#pragma once

#include "Common.h"

// ============================================================
//  Stage.h  -  연습장 (바닥 + 벽 4면 + 천장) 과 공간 판정
// ============================================================

void DrawStage();

// 연습장 안쪽으로 x, z 를 제한한다. (y 는 건드리지 않는다 - 점프 처리와 겹치지 않게)
void ClampInsideRoom(glm::vec3& pos, float radius);

// 카메라가 벽/천장 밖으로 나가지 않게 x, y, z 를 모두 제한한다.
// (벽은 안쪽만 앞면이라, 카메라가 밖으로 나가면 벽이 컬링되어 방이 뚫려 보인다)
void ClampCameraInsideRoom(glm::vec3& pos, float margin);

// 필살기 조건 중 "인근에 벽이 있을 때" 판정에 쓴다.
bool IsNearWall(const glm::vec3& pos, float margin);

// 광원 하나 (천장 가운데 근처)
glm::vec3 StageLightPos();
glm::vec3 StageLightColor();
