#include "Mesh.h"

// 사각형 하나(= 삼각형 2개)를 추가한다.
// a -> b -> c -> d 는 "앞면에서 봤을 때 반시계(CCW)" 순서여야 한다.
//
// 법선은 두 변의 외적으로 구한다. 한 면의 네 정점이 모두 같은 법선을 갖게 되므로
// 면마다 밝기가 뚝뚝 끊기는 평면 음영(flat shading)이 된다.
// 육면체를 조합한 로봇에는 이 방식이 어울린다.
static void AddQuad(std::vector<Vertex>& data,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d)
{
    const glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));

    data.push_back(Vertex{ a, n });
    data.push_back(Vertex{ b, n });
    data.push_back(Vertex{ c, n });

    data.push_back(Vertex{ a, n });
    data.push_back(Vertex{ c, n });
    data.push_back(Vertex{ d, n });
}

std::vector<Vertex> MakeCubeData()
{
    const glm::vec3 LBF{ -0.5f, -0.5f,  0.5f },
        RBF{ 0.5f, -0.5f,  0.5f },
        RTF{ 0.5f,  0.5f,  0.5f },
        LTF{ -0.5f,  0.5f,  0.5f };

    const glm::vec3 LBK{ -0.5f, -0.5f, -0.5f },
        RBK{ 0.5f, -0.5f, -0.5f },
        RTK{ 0.5f,  0.5f, -0.5f },
        LTK{ -0.5f,  0.5f, -0.5f };

    std::vector<Vertex> data;
    data.reserve(36);

    AddQuad(data, LBF, RBF, RTF, LTF);      // 앞면   (+z 쪽에서 봤을 때 반시계)
    AddQuad(data, RBK, LBK, LTK, RTK);      // 뒷면   (-z 쪽에서)
    AddQuad(data, RBF, RBK, RTK, RTF);      // 오른쪽 (+x 쪽에서)
    AddQuad(data, LBK, LBF, LTF, LTK);      // 왼쪽   (-x 쪽에서)
    AddQuad(data, LTF, RTF, RTK, LTK);      // 윗면   (+y 쪽에서)
    AddQuad(data, LBK, RBK, RBF, LBF);      // 아랫면 (-y 쪽에서)

    return data;
}

std::vector<Vertex> MakePlaneData()
{
    std::vector<Vertex> data;
    data.reserve(6);

    AddQuad(data,
        glm::vec3(-0.5f, -0.5f, 0.0f),
        glm::vec3(0.5f, -0.5f, 0.0f),
        glm::vec3(0.5f,  0.5f, 0.0f),
        glm::vec3(-0.5f,  0.5f, 0.0f));

    return data;
}

Mesh MakeMesh(const std::vector<Vertex>& data)
{
    Mesh m;
    m.vertexCount = static_cast<int>(data.size());

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        data.size() * sizeof(Vertex), data.data(), GL_STATIC_DRAW);

    // location 0 : 위치  (Vertex 안에서 offset 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    // location 1 : 법선  (Vertex 안에서 offset 12바이트)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return m;
}

void DeleteMesh(Mesh& m)
{
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    m = Mesh{};
}
