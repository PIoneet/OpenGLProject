// ============================================================
//  DragonGame  -  카메라 연출형 필살기 격투 게임
//  팀 프로젝트 20팀 : 선홍재(카메라 연출/충돌), 안세현(로봇 모션/충돌)
//
//  main.cpp 는 초기화와 메인 루프만 담당한다.
//  실제 내용은 Robot / Camera / Stage / Renderer 로 나뉘어 있다.
// ============================================================

#include "Common.h"
#include "Renderer.h"
#include "Input.h"
#include "Robot.h"
#include "Camera.h"
#include "Stage.h"
#include "Hud.h"
#include "Screenshot.h"

#include <iostream>

#define WIDTH  1600
#define HEIGHT 1200

namespace
{
    int fbWidth{ WIDTH };
    int fbHeight{ HEIGHT };

    Robot      player;
    RobotParts playerParts;     // 매 프레임 한 번만 계산해서 카메라와 그리기가 함께 쓴다
    Robot      enemy;
    RobotParts enemyParts;

    Camera     camera;

    double prevFrame{ 0.0 };

    // --shot <파일> 로 실행하면 몇 프레임 뒤 자동으로 저장하고 종료한다.
    // (렌더링 결과를 확인하거나, 보고서용 캡처를 뽑을 때)
    std::string autoShotPath;
    int         autoShotCountdown{ -1 };

    // 처음에 서로 마주 보고 서는 거리 (제안서: "플레이어와 적이 마주 선 상태로 시작")
    const float startGap = 2.2f;
    const float enemyTurnSpeedDeg = 90.0f;   // 적이 플레이어를 향해 돌아서는 속도

    void FramebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
    {
        fbWidth = width;
        fbHeight = height;
        glViewport(0, 0, width, height);
    }

    void ResetAll()
    {
        // 플레이어는 -z 쪽에서 +z 를 보고, 적은 +z 쪽에서 -z 를 보며 마주 선다
        ResetRobot(player, glm::vec3(0.0f, 0.0f, -startGap), 0.0f);
        ResetRobot(enemy, glm::vec3(0.0f, 0.0f, startGap), 180.0f);
        ResetCamera(camera);
    }

    void Update(const InputState& input, float dt)
    {
        if (input.resetPressed)
        {
            ResetAll();
        }
        else
        {
            UpdateRobotMotion(player, input.move, input.turn, input.jumpPressed, dt);

            // 적은 제자리에 서서 플레이어를 바라본다. (행동 트리 없음 - 제안서대로)
            // 입력을 0 으로 줘서 중력과 스윙 감쇠만 돌린다.
            UpdateRobotMotion(enemy, glm::vec2(0.0f), 0.0f, false, dt);
            FaceTowards(enemy, player.pos, enemyTurnSpeedDeg, dt);

            // 몸통끼리 겹치면 서로 반씩 밀어낸다 (서로를 통과해 지나가지 않게)
            ResolveBodyOverlap(player, enemy);

            // 임시 : 공격 판정이 들어오기 전까지 체력 바를 눈으로 확인하는 용도
            if (input.debugDamageEnemyPressed)  ApplyDamage(enemy, 10);
            if (input.debugDamagePlayerPressed) ApplyDamage(player, 10);
        }

        // TODO(3단계) : 공격 모션 상태머신, 히트박스 판정
        // TODO(4단계) : input.specialPressed + 발동 조건 -> camera.mode 전환

        // 관절 행렬을 먼저 확정해야 카메라(1인칭 눈 위치)와 그리기가 같은 자세를 본다
        playerParts = ComputeRobotParts(player);
        enemyParts = ComputeRobotParts(enemy);

        UpdateCamera(camera, player, playerParts, dt);
    }

    void Render()
    {
        BeginFrame(glm::vec3(0.08f, 0.08f, 0.10f));

        ApplyCamera(camera, fbWidth, fbHeight);
        SetLightUniforms(StageLightPos(), StageLightColor());

        DrawStage();
        DrawRobot(playerParts, PlayerPalette());
        DrawRobot(enemyParts, EnemyPalette());

        // UI 는 3D 를 다 그린 뒤에 (깊이 테스트를 끄고 맨 위에 올린다)
        DrawHud(player, enemy, fbWidth, fbHeight);
    }

    void PrintControls()
    {
        std::cout <<
            "---------------- 조작법 ----------------\n"
            "  W / A / S / D   : 이동 (전진/좌/후진/우)\n"
            "  <- / ->         : 방향 회전\n"
            "  Space           : 점프\n"
            "  K               : 필살기 (구현 예정)\n"
            "  F12             : 스크린샷 (shot_0001.bmp ...)\n"
            "  G / H           : [임시] 플레이어 / 적 체력 -10\n"
            "  I               : 초기 상태로 리셋\n"
            "  Q / ESC         : 종료\n"
            "----------------------------------------\n";
    }
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--shot" && i + 1 < argc)
        {
            autoShotPath = argv[i + 1];
            autoShotCountdown = 30;     // 카메라 보간이 자리를 잡을 만큼 기다린다
            ++i;
        }
    }

    if (!glfwInit())
    {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "DragonGame", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "윈도우 생성 실패!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);            // 수직동기화 : dt 가 지나치게 작아지는 것을 막는다

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW 초기화 실패!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    // 깊이 테스트 : 가까운 면이 먼 면을 가린다.
    // 면 컬링     : 뒷면(시계 방향으로 보이는 면)은 그리지 않는다. 앞면 = 반시계(CCW).
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    if (!InitRenderer("VS.glsl", "PS.glsl"))
    {
        std::cerr << "렌더러 초기화 실패!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    ResetAll();
    PrintControls();

    prevFrame = glfwGetTime();

    InputState input;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        const double now = glfwGetTime();
        float dt = static_cast<float>(now - prevFrame);
        prevFrame = now;
        if (dt > 0.05f) dt = 0.05f;     // 창을 끌거나 멈췄다 돌아올 때 튀지 않게

        PollInput(window, input);
        if (input.quitPressed)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        Update(input, dt);
        Render();

        // 스왑 전에 백버퍼를 읽어야 이번 프레임 화면이 그대로 저장된다
        if (input.screenshotPressed)
        {
            std::string saved;
            if (SaveScreenshotAuto(fbWidth, fbHeight, saved))
                std::cout << "스크린샷 저장: " << saved << std::endl;
        }

        if (autoShotCountdown > 0)
        {
            --autoShotCountdown;
            if (autoShotCountdown == 0)
            {
                SaveScreenshotBMP(autoShotPath, fbWidth, fbHeight);
                std::cout << "스크린샷 저장: " << autoShotPath << std::endl;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        glfwSwapBuffers(window);
    }

    ShutdownRenderer();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
