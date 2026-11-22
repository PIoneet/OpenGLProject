#include "Renderer.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
    GLuint gProgram{ 0 };

    GLint gModelLoc{ -1 };
    GLint gNormalMatLoc{ -1 };
    GLint gViewLoc{ -1 };
    GLint gProjLoc{ -1 };
    GLint gColorLoc{ -1 };
    GLint gLightPosLoc{ -1 };
    GLint gLightColorLoc{ -1 };
    GLint gEyePosLoc{ -1 };
    GLint gUnlitLoc{ -1 };

    Mesh gCube;
    Mesh gPlane;

    std::string FileToBuf(const char* path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "ERROR: 셰이더 파일을 열 수 없음: " << path << std::endl;
            return "";
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    GLuint CompileShader(GLenum type, const char* path)
    {
        const std::string src = FileToBuf(path);
        if (src.empty()) return 0;

        const char* csrc = src.c_str();
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &csrc, nullptr);
        glCompileShader(shader);

        GLint ok{};
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            GLchar log[1024]{};
            glGetShaderInfoLog(shader, 1024, nullptr, log);
            std::cerr << "ERROR: 셰이더 컴파일 실패 (" << path << ")\n" << log << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }
}

bool InitRenderer(const char* vsPath, const char* psPath)
{
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsPath);
    GLuint ps = CompileShader(GL_FRAGMENT_SHADER, psPath);
    if (vs == 0 || ps == 0) return false;

    gProgram = glCreateProgram();
    glAttachShader(gProgram, vs);
    glAttachShader(gProgram, ps);
    glLinkProgram(gProgram);

    glDeleteShader(vs);
    glDeleteShader(ps);

    GLint ok{};
    glGetProgramiv(gProgram, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLchar log[1024]{};
        glGetProgramInfoLog(gProgram, 1024, nullptr, log);
        std::cerr << "ERROR: 셰이더 프로그램 연결 실패\n" << log << std::endl;
        glDeleteProgram(gProgram);
        gProgram = 0;
        return false;
    }

    glUseProgram(gProgram);

    gModelLoc = glGetUniformLocation(gProgram, "model");
    gNormalMatLoc = glGetUniformLocation(gProgram, "normalMat");
    gViewLoc = glGetUniformLocation(gProgram, "view");
    gProjLoc = glGetUniformLocation(gProgram, "projection");
    gColorLoc = glGetUniformLocation(gProgram, "uColor");
    gLightPosLoc = glGetUniformLocation(gProgram, "uLightPos");
    gLightColorLoc = glGetUniformLocation(gProgram, "uLightColor");
    gEyePosLoc = glGetUniformLocation(gProgram, "uEyePos");
    gUnlitLoc = glGetUniformLocation(gProgram, "uUnlit");

    gCube = MakeMesh(MakeCubeData());
    gPlane = MakeMesh(MakePlaneData());

    return true;
}

void ShutdownRenderer()
{
    DeleteMesh(gCube);
    DeleteMesh(gPlane);

    if (gProgram) glDeleteProgram(gProgram);
    gProgram = 0;
}

void BeginFrame(const glm::vec3& clearColor)
{
    glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(gProgram);
    glUniform1i(gUnlitLoc, 0);
}

void SetCameraUniforms(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eyePos)
{
    glUniformMatrix4fv(gViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(gProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(gEyePosLoc, 1, glm::value_ptr(eyePos));
}

void SetLightUniforms(const glm::vec3& lightPos, const glm::vec3& lightColor)
{
    glUniform3fv(gLightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(gLightColorLoc, 1, glm::value_ptr(lightColor));
}

const Mesh& CubeMesh() { return gCube; }
const Mesh& PlaneMesh() { return gPlane; }

void DrawMesh(const Mesh& m, const glm::mat4& model, const glm::vec3& color)
{
    // 팔다리처럼 한 축만 길게 늘인(비균등 스케일) 물체는 model 을 법선에 그대로 쓰면
    // 법선이 면과 수직이 아니게 되어 음영이 틀어진다. 그래서 역행렬의 전치를 따로 넘긴다.
    const glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));

    glUniformMatrix4fv(gModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(gNormalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
    glUniform3fv(gColorLoc, 1, glm::value_ptr(color));

    glBindVertexArray(m.vao);
    glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
}

void DrawCube(const glm::mat4& model, const glm::vec3& color)
{
    DrawMesh(gCube, model, color);
}

void BeginUI(int fbWidth, int fbHeight)
{
    glDisable(GL_DEPTH_TEST);   // UI 는 3D 물체에 가리지 않고 항상 위에 그린다
    glDisable(GL_CULL_FACE);    // 사각형 방향을 신경 쓰지 않아도 되게
    glUniform1i(gUnlitLoc, 1);

    // 왼쪽 위가 (0, 0), 오른쪽 아래가 (w, h) 인 좌표계
    const glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(fbWidth),
        static_cast<float>(fbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(gViewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniformMatrix4fv(gProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
}

void DrawUIRect(float x, float y, float w, float h, const glm::vec3& color)
{
    // 평면 메시는 원점 중심 1x1 이므로, 중심으로 옮기고 크기를 곱한다
    glm::mat4 model{ 1.0f };
    model = glm::translate(model, glm::vec3(x + w * 0.5f, y + h * 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));

    DrawMesh(gPlane, model, color);
}

void EndUI()
{
    glUniform1i(gUnlitLoc, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void DrawWall(const glm::vec3& position, const glm::vec3& axis, float angleDeg,
    float sizeX, float sizeY, const glm::vec3& color)
{
    glm::mat4 model{ 1.0f };
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(angleDeg), axis);
    model = glm::scale(model, glm::vec3(sizeX, sizeY, 1.0f));

    DrawMesh(gPlane, model, color);
}
