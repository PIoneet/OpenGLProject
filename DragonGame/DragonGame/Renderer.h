#pragma once

#include "Common.h"
#include "Mesh.h"

// ============================================================
//  Renderer.h  -  셰이더 프로그램 + uniform + 그리기 헬퍼
//
//  Robot / Camera / Stage 코드는 OpenGL 함수를 직접 부르지 않고
//  여기에 있는 함수만 호출한다. (각자 자기 파일만 고치게 하기 위한 경계)
// ============================================================

bool InitRenderer(const char* vsPath, const char* psPath);
void ShutdownRenderer();

// 매 프레임 시작할 때 한 번
void BeginFrame(const glm::vec3& clearColor);

// 카메라가 결정한 view / projection 과 시점 위치를 올린다
void SetCameraUniforms(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eyePos);

// 광원 하나 (연습장 천장 근처)
void SetLightUniforms(const glm::vec3& lightPos, const glm::vec3& lightColor);

// 공용 메시
const Mesh& CubeMesh();
const Mesh& PlaneMesh();

// model 행렬과 색을 주고 그린다. 법선 전용 행렬은 내부에서 계산한다.
void DrawMesh(const Mesh& m, const glm::mat4& model, const glm::vec3& color);
void DrawCube(const glm::mat4& model, const glm::vec3& color);

// ---- 화면 고정 UI (체력 바 등) ----
// 직교 투영으로 바꾸고, 조명/깊이 테스트/컬링을 끈다. 3D 를 다 그린 뒤에 호출한다.
// 좌표는 픽셀 단위, 원점은 화면 왼쪽 위.
void BeginUI(int fbWidth, int fbHeight);
void DrawUIRect(float x, float y, float w, float h, const glm::vec3& color);
void EndUI();

// 평면 하나를 "위치 + 회전축/각도 + 가로/세로 크기" 로 배치해 그린다.
// 회전을 먼저, 스케일을 나중에 적용해야 로컬 축 기준으로 늘어난다.
void DrawWall(const glm::vec3& position, const glm::vec3& axis, float angleDeg,
    float sizeX, float sizeY, const glm::vec3& color);
