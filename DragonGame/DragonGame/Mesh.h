#pragma once

#include "Common.h"

// ============================================================
//  Mesh.h  -  정점 데이터 생성 / VAO,VBO 관리
// ============================================================

// 정점 하나 = 위치(3) + 법선(3).
// 법선(그 면이 바라보는 방향)이 있어야 조명 계산을 할 수 있다.
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
};

struct Mesh
{
    GLuint vao{}, vbo{};
    int    vertexCount{};
};

std::vector<Vertex> MakeCubeData();    // 원점 중심, 한 변 1.0 인 육면체
std::vector<Vertex> MakePlaneData();   // 원점 중심, 1x1, +z 를 바라보는 평면

Mesh MakeMesh(const std::vector<Vertex>& data);
void DeleteMesh(Mesh& m);
