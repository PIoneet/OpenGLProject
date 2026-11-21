#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define WIDTH  1600
#define HEIGHT 1200

const float PI = 3.14159265f;

struct Mesh
{
    GLuint vao{}, vbo{};
    int vertexCount{};
};


// ---------- 선언 ----------
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

std::vector<float> MakeCubeData();
std::vector<float> MakePlaneData();
Mesh makeMesh(const std::vector<float>& data);
void DeleteMesh(Mesh& m);

float RandRange(float lo, float hi);
void InitRobot();
void InitObstacles();
void ResetAll();

bool OverlapXZ(float x, float z, float obstacleX, float obstacleZ, float halfW, float halfD);
bool Blocked(float x, float z, float feetY);
float GroundHeightAt(float x, float z, float feetY);
void TurnAway();
float SwingAmp();

void UpdateFrontWall(float dt);
void UpdateWalkPhase(float dt);
void MoveRobot(float dt);
void TurnBody(float dt);
void UpdateVertical(float dt);
void UpdateCamera(float dt);
void UpdateAnimation();

void UploadCamera();
void DrawMesh(const Mesh& m, const glm::mat4& model, const glm::vec3& color);
void DrawWall(const glm::vec3& position, const glm::vec3& axis, float angleDeg, float sizeX, float sizeY, const glm::vec3& color);
void DrawStage();
void DrawObstacles();
void DrawRobot();
void DrawScene();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void CharCallback(GLFWwindow* window, unsigned int codepoint);



GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
GLint  modelLoc{ -1 }, viewLoc{ -1 }, projLoc{ -1 }, colorLoc{ -1 };

Mesh cubeMesh, planeMesh;

double prevFrame{};
int fbWidth{ WIDTH }, fbHeight{ HEIGHT };


const float roomW = 12.0f;                  // 가로 (x)
const float roomD = 12.0f;                  // 깊이 (z)
const float roomH = 6.0f;                   // 높이 (y)

bool  frontWantOpen{ false };               // o 키 : 앞벽이 "열려야 하는가" (스위치)
float frontOpen{ 0.0f };                    //        열림 진행도. 0 = 닫힘, 1 = 완전히 열림
const float frontSpeed = 0.8f;              //        진행도가 초당 변하는 양


struct Robot
{
    glm::vec3 pos{ 0.0f };                  // 발바닥 중심의 월드 위치 (pos.y = 지금 서 있는 높이)
    float     yawDeg{ 0.0f };               // 몸이 바라보는 방향(도). 로봇의 앞 = 로컬 +z
    glm::vec3 dir{ 1.0f, 0.0f, 0.0f };      // 이동 방향 (x, z 만 사용, 길이 1)
    float     vy{ 0.0f };                   // 위아래 속도 (점프할 때 위로, 떨어질 때 아래로)
    float     walkSpeed{ 2.0f };            // 걷는 속도 (초당 거리)
    float     walkPhase{ 0.0f };            // 팔다리 흔들기 위상 (계속 커진다. sin 안에서만 쓴다)
};
Robot robot;

const float robotR = 0.45f;                 // 발바닥 충돌 반폭 (팔이 튀어나온 폭까지 감안)
const float speedMin = 0.5f, speedMax = 6.0f, speedStep = 0.5f;
const float swingMinDeg = 25.0f, swingMaxDeg = 65.0f;   // 가장 느릴 때 / 가장 빠를 때 팔다리 흔드는 각도
const float stepFreq = 2.5f;                // 이동 거리 1 당 위상이 커지는 양 -> 빨리 걸을수록 빨리 흔든다
const float turnSpeedDeg = 540.0f;          // 방향 바꿀 때 몸이 도는 속도 (도/초)
const float gravityAcc = 15.0f;             // 중력 가속도
const float jumpSpeed = 7.0f;               // 점프 시작 속도 (최대 높이 = v*v / (2g) = 약 1.6)


const glm::vec3 torsoCenter{ 0.0f, 1.4f, 0.0f };
const glm::vec3 torsoSize{ 0.7f, 1.0f, 0.4f };
const glm::vec3 torsoColor{ 0.85f, 0.85f, 0.90f };

const glm::vec3 headCenter{ 0.0f, 2.15f, 0.0f };
const glm::vec3 headSize{ 0.5f, 0.5f, 0.5f };
const glm::vec3 headColor{ 0.95f, 0.80f, 0.65f };

const glm::vec3 noseOffset{ 0.0f, -0.03f, 0.325f };    // 머리 중심 기준. 0.325 = 머리 반 두께 0.25 + 코 반 두께 0.075
const glm::vec3 noseSize{ 0.12f, 0.12f, 0.15f };
const glm::vec3 noseColor{ 0.90f, 0.25f, 0.15f };

const float armLength = 0.8f, armThick = 0.18f;
const glm::vec3 leftShoulder{ 0.47f, 1.85f, 0.0f };    // 어깨 = 팔의 회전 중심 (로봇 기준 위치)
const glm::vec3 rightShoulder{ -0.47f, 1.85f, 0.0f };
const glm::vec3 leftArmColor{ 0.90f, 0.20f, 0.20f };   // 왼팔 빨강 / 오른팔 파랑 -> 방향을 구분하기 쉽다
const glm::vec3 rightArmColor{ 0.20f, 0.30f, 0.90f };

const float legLength = 0.9f, legThick = 0.24f;
const glm::vec3 leftHip{ 0.18f, 0.9f, 0.0f };          // 엉덩이 = 다리의 회전 중심
const glm::vec3 rightHip{ -0.18f, 0.9f, 0.0f };
const glm::vec3 leftLegColor{ 0.95f, 0.60f, 0.10f };
const glm::vec3 rightLegColor{ 0.20f, 0.70f, 0.80f };


struct Obstacle
{
    float x{}, z{};                         // 중심의 (x, z)
    float sizeX{ 1.0f }, sizeY{ 1.0f }, sizeZ{ 1.0f };
    glm::vec3 color{ 1.0f };
};
std::vector<Obstacle> obstacles;

// ---------- 카메라 ----------
const glm::vec3 initCameraPos{ 0.0f, 7.0f, 17.0f };
const glm::vec3 initCameraTarget{ 0.0f, 1.5f, 0.0f };
glm::vec3 cameraPos{ initCameraPos };
glm::vec3 cameraTarget{ initCameraTarget };
glm::vec3 cameraUp{ 0.0f, 1.0f, 0.0f };
int camOrbitDir{ 0 };                       // y/Y : 공전 방향  +1, -1, 0(정지)


int main()
{
    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Practice 23", nullptr, nullptr);
    if (!window) {
        std::cerr << "윈도우생성실패!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // 깊이 테스트 : 가까운 면이 먼 면을 가린다.
    // 면 컬링     : 뒷면(시계 방향으로 보이는 면)은 그리지 않는다. 앞면 = 반시계(CCW).
    //               벽은 "안쪽이 앞면" 이 되도록 돌려 놓았기 때문에, 바깥에서 보면 벽이 사라지고 안쪽만 보인다.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCharCallback(window, CharCallback);

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    modelLoc = glGetUniformLocation(shaderProgramID, "model");
    viewLoc = glGetUniformLocation(shaderProgramID, "view");
    projLoc = glGetUniformLocation(shaderProgramID, "projection");
    colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    cubeMesh = makeMesh(MakeCubeData());
    planeMesh = makeMesh(MakePlaneData());

    srand((unsigned int)time(nullptr));     // 난수 씨앗 : 실행할 때마다 장애물 배치가 달라지게
    ResetAll();
    prevFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        UpdateAnimation();
        DrawScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    DeleteMesh(cubeMesh);
    DeleteMesh(planeMesh);
    glDeleteProgram(shaderProgramID);

    glfwDestroyWindow(window);
    glfwTerminate();
}


// ---------- 셰이더 (파일 읽기 -> 컴파일 -> 링크 : 복사해서 쓰는 부분) ----------
void make_vertexShaders()
{
    std::string vertexSource = filetobuf("VS.glsl");
    const char* source = vertexSource.c_str();

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &source, NULL);
    glCompileShader(vertexShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

void make_fragmentShaders()
{
    std::string fragmentSource = filetobuf("PS.glsl");
    const char* source = fragmentSource.c_str();

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &source, NULL);
    glCompileShader(fragmentShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
        std::cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

GLuint make_shaderProgram()
{
    GLint result;
    GLchar errorLog[512];

    GLuint shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);
    glLinkProgram(shaderID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
        std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
        return 0;
    }

    glUseProgram(shaderID);
    return shaderID;
}

std::string filetobuf(const char* file)
{
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open())
        return "";
    std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    shaderFile.close();
    return source;
}


void AddQuad(std::vector<float>& data, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d)
{
    data.insert(data.end(), { a.x, a.y, a.z });
    data.insert(data.end(), { b.x, b.y, b.z });
    data.insert(data.end(), { c.x, c.y, c.z });

    data.insert(data.end(), { a.x, a.y, a.z });
    data.insert(data.end(), { c.x, c.y, c.z });
    data.insert(data.end(), { d.x, d.y, d.z });
}



std::vector<float> MakeCubeData()
{
    const glm::vec3 LBF{ -0.5f, -0.5f,  0.5f }, RBF{ 0.5f, -0.5f,  0.5f }, RTF{ 0.5f, 0.5f,  0.5f }, LTF{ -0.5f, 0.5f,  0.5f };
    const glm::vec3 LBK{ -0.5f, -0.5f, -0.5f }, RBK{ 0.5f, -0.5f, -0.5f }, RTK{ 0.5f, 0.5f, -0.5f }, LTK{ -0.5f, 0.5f, -0.5f };

    std::vector<float> data;
    AddQuad(data, LBF, RBF, RTF, LTF);      // 앞면   (+z 쪽에서 봤을 때 반시계)
    AddQuad(data, RBK, LBK, LTK, RTK);      // 뒷면   (-z 쪽에서 봤을 때 반시계)
    AddQuad(data, RBF, RBK, RTK, RTF);      // 오른쪽 (+x 쪽에서)
    AddQuad(data, LBK, LBF, LTF, LTK);      // 왼쪽   (-x 쪽에서)
    AddQuad(data, LTF, RTF, RTK, LTK);      // 윗면   (+y 쪽에서)
    AddQuad(data, LBK, RBK, RBF, LBF);
    return data;
}



std::vector<float> MakePlaneData()
{
    std::vector<float> data;
    AddQuad(data, glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(-0.5f, 0.5f, 0.0f));
    return data;
}


Mesh makeMesh(const std::vector<float>& data)
{
    Mesh m;
    m.vertexCount = (int)(data.size() / 3);

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    return m;
}

void DeleteMesh(Mesh& m)
{
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    m = Mesh{};
}




float RandRange(float lo, float hi)
{
    float t = (float)rand() / (float)RAND_MAX;
    return lo + (hi - lo) * t;
}

void InitRobot()
{
    robot = Robot{};
    robot.dir = glm::vec3(1.0f, 0.0f, 0.0f);

    robot.yawDeg = glm::degrees(atan2f(robot.dir.x, robot.dir.z));
}



void InitObstacles()
{
    obstacles.clear();

    int tries = 0;
    while ((int)obstacles.size() < 3 && tries < 2000)
    {
        tries++;

        Obstacle o;
        o.sizeX = RandRange(0.8f, 1.6f);
        o.sizeY = RandRange(0.4f, 1.1f);
        o.sizeZ = RandRange(0.8f, 1.6f);
        o.color = glm::vec3(RandRange(0.3f, 1.0f), RandRange(0.3f, 1.0f), RandRange(0.3f, 1.0f));

        // 벽에 붙지 않도록 중심이 갈 수 있는 범위
        float limX = roomW / 2 - 0.3f - o.sizeX / 2;
        float limZ = roomD / 2 - 0.3f - o.sizeZ / 2;
        o.x = RandRange(-limX, limX);
        o.z = RandRange(-limZ, limZ);

        bool ok = true;

        if (fabsf(o.x) < o.sizeX / 2 + robotR + 0.6f && fabsf(o.z) < o.sizeZ / 2 + robotR + 0.6f)
            ok = false;


        for (int i = 0; i < (int)obstacles.size(); ++i)
        {
            const Obstacle& p = obstacles[i];
            if (fabsf(o.x - p.x) < (o.sizeX + p.sizeX) / 2 + 0.8f &&
                fabsf(o.z - p.z) < (o.sizeZ + p.sizeZ) / 2 + 0.8f)
                ok = false;
        }

        if (ok)
            obstacles.push_back(o);
    }
}

void ResetAll()
{
    InitRobot();
    InitObstacles();

    frontWantOpen = false;
    frontOpen = 0.0f;

    cameraPos = initCameraPos;
    cameraTarget = initCameraTarget;
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    camOrbitDir = 0;
}



bool OverlapXZ(float x, float z, float obstacleX, float obstacleZ, float halfW, float halfD)
{

    float minX = obstacleX - (halfW + robotR);
    float maxX = obstacleX + (halfW + robotR);
    float minZ = obstacleZ - (halfD + robotR);
    float maxZ = obstacleZ + (halfD + robotR);

    bool overlapX = (x > minX) && (x < maxX);
    bool overlapZ = (z > minZ) && (z < maxZ);
    return overlapX && overlapZ;
}


bool Blocked(float x, float z, float feetY)
{
    for (int i = 0; i < (int)obstacles.size(); ++i)
    {
        const Obstacle& o = obstacles[i];
        bool overlap = OverlapXZ(x, z, o.x, o.z, o.sizeX / 2, o.sizeZ / 2);
        bool feetBelowTop = feetY < o.sizeY - 0.01f;
        if (overlap && feetBelowTop)
            return true;
    }
    return false;
}

// 로봇 발 밑의 바닥 높이를 알아야 하기에 그것을 반환한다. 
float GroundHeightAt(float x, float z, float feetY)
{
    // feetY가 로봇의 발바닥 높이다. 

    float highest = 0.0f;
    for (int i = 0; i < (int)obstacles.size(); ++i)
    {
        const Obstacle& o = obstacles[i];
        bool overlap = OverlapXZ(x, z, o.x, o.z, o.sizeX / 2, o.sizeZ / 2);
        bool topFeet = o.sizeY <= feetY + 0.05f;

        if (overlap && topFeet && o.sizeY > highest)
            highest = o.sizeY;
    }
    return highest;
}


void TurnAway()
{
    float dx = robot.dir.x;
    float dz = robot.dir.z;
    if (rand() % 2 == 0)
        robot.dir = glm::vec3(dz, 0.0f, -dx);
    else
        robot.dir = glm::vec3(-dz, 0.0f, dx);
}



float SwingAmp()
{
    float t = (robot.walkSpeed - speedMin) / (speedMax - speedMin);
    return swingMinDeg + (swingMaxDeg - swingMinDeg) * t;
}



void CharCallback(GLFWwindow* window, unsigned int codepoint)
{
    switch (codepoint)
    {
    case 'q':
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case 'o':
        frontWantOpen = !frontWantOpen;
        break;

        // 이동 방향 : 월드 축 방향. 화면 위쪽(w) = 화면 안쪽 = -z
    case 'w': robot.dir = glm::vec3(0.0f, 0.0f, -1.0f); break;
    case 's': robot.dir = glm::vec3(0.0f, 0.0f, 1.0f);  break;
    case 'a': robot.dir = glm::vec3(-1.0f, 0.0f, 0.0f); break;
    case 'd': robot.dir = glm::vec3(1.0f, 0.0f, 0.0f);  break;

        // 걷는 속도 : '+' 는 Shift 와 '=' 를 같이 눌러야 해서 '=' 도 받는다. 범위를 벗어나지 않게 자른다.
    case '+':
    case '=':
        robot.walkSpeed = robot.walkSpeed + speedStep;
        if (robot.walkSpeed > speedMax) robot.walkSpeed = speedMax;
        break;
    case '-':
        robot.walkSpeed = robot.walkSpeed - speedStep;
        if (robot.walkSpeed < speedMin) robot.walkSpeed = speedMin;
        break;

    case 'j':                               // 점프 : 땅(또는 장애물 윗면) 에 서 있을 때만 시작 속도를 준다
    {
        float ground = GroundHeightAt(robot.pos.x, robot.pos.z, robot.pos.y);
        bool standing = (robot.vy == 0.0f) && (fabsf(robot.pos.y - ground) < 0.01f);
        if (standing)
            robot.vy = jumpSpeed;           // 공중에서 또 누르면 standing 이 거짓 -> 이중 점프 방지
        break;
    }


    case 'z': cameraPos.z += 0.3f; cameraTarget.z += 0.3f; break;
    case 'Z': cameraPos.z -= 0.3f; cameraTarget.z -= 0.3f; break;
    case 'x': cameraPos.x += 0.3f; cameraTarget.x += 0.3f; break;
    case 'X': cameraPos.x -= 0.3f; cameraTarget.x -= 0.3f; break;


    case 'y':
        if (camOrbitDir == 1) camOrbitDir = 0; else camOrbitDir = 1;
        break;
    case 'Y':
        if (camOrbitDir == -1) camOrbitDir = 0; else camOrbitDir = -1;
        break;

    case 'i':
        ResetAll();
        break;
    }
}



void UpdateAnimation()
{
    double now = glfwGetTime();
    float dt = (float)(now - prevFrame);
    prevFrame = now;
    if (dt > 0.05f) dt = 0.05f;


    UpdateFrontWall(dt);
    UpdateWalkPhase(dt);
    MoveRobot(dt);
    TurnBody(dt);
    UpdateVertical(dt);
    UpdateCamera(dt);
}



void UpdateFrontWall(float dt)
{
    if (frontWantOpen)
        frontOpen = frontOpen + frontSpeed * dt;
    else
        frontOpen = frontOpen - frontSpeed * dt;

    if (frontOpen > 1.0f) frontOpen = 1.0f;
    if (frontOpen < 0.0f) frontOpen = 0.0f;
}



void UpdateWalkPhase(float dt)
{
    robot.walkPhase = robot.walkPhase + dt * robot.walkSpeed * stepFreq;
}




void MoveRobot(float dt)
{
    float stepX = robot.dir.x * robot.walkSpeed * dt;
    float stepZ = robot.dir.z * robot.walkSpeed * dt;
    float newX = robot.pos.x + stepX;
    float newZ = robot.pos.z + stepZ;

    float limitX = roomW / 2 - robotR;      // 가장자리 한계 = 방 반폭 - 로봇 반폭
    float limitZ = roomD / 2 - robotR;

    bool outOfRoom = fabsf(newX) > limitX || fabsf(newZ) > limitZ;

    if (outOfRoom)
    {
        robot.dir = -robot.dir;
    }
    else if (Blocked(newX, newZ, robot.pos.y))
    {
        TurnAway();
    }
    else
    {
        robot.pos.x = newX;
        robot.pos.z = newZ;
    }
}



void TurnBody(float dt)
{
    float targetYaw = glm::degrees(atan2f(robot.dir.x, robot.dir.z));

    float diff = targetYaw - robot.yawDeg;
    while (diff > 180.0f)  diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;

    float maxTurn = turnSpeedDeg * dt;
    if (diff > maxTurn)  diff = maxTurn;
    if (diff < -maxTurn) diff = -maxTurn;

    robot.yawDeg = robot.yawDeg + diff;
}



void UpdateVertical(float dt)
{
    float support = GroundHeightAt(robot.pos.x, robot.pos.z, robot.pos.y);

    bool inAir = (robot.pos.y > support) || (robot.vy > 0.0f);

    if (inAir)
    {
        robot.vy = robot.vy - gravityAcc * dt;
        robot.pos.y = robot.pos.y + robot.vy * dt;

        bool landed = (robot.pos.y <= support) && (robot.vy <= 0.0f);
        if (landed)
        {
            robot.pos.y = support;
            robot.vy = 0.0f;
        }
    }
    else
    {
        robot.pos.y = support;
        robot.vy = 0.0f;
    }
}


void UpdateCamera(float dt)
{
    if (camOrbitDir == 0)
        return;

    float angleDeg = camOrbitDir * 45.0f * dt;
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(angleDeg), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 offset = cameraPos - cameraTarget;
    glm::vec4 rotated = rot * glm::vec4(offset, 0.0f);
    cameraPos = cameraTarget + glm::vec3(rotated);
}



void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    fbWidth = width;
    fbHeight = height;
    glViewport(0, 0, width, height);
}


void UploadCamera()
{
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
}


void DrawMesh(const Mesh& m, const glm::mat4& model, const glm::vec3& color)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(colorLoc, color.x, color.y, color.z);
    glBindVertexArray(m.vao);
    glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
}


void DrawWall(const glm::vec3& position, const glm::vec3& axis, float angleDeg, float sizeX, float sizeY, const glm::vec3& color)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(angleDeg), axis);
    model = glm::scale(model, glm::vec3(sizeX, sizeY, 1.0f));
    DrawMesh(planeMesh, model, color);
}


void DrawStage()
{
    const glm::vec3 X(1.0f, 0.0f, 0.0f);
    const glm::vec3 Y(0.0f, 1.0f, 0.0f);


    DrawWall(glm::vec3(0.0f, roomH / 2, -roomD / 2), Y, 0.0f, roomW, roomH, glm::vec3(0.75f, 0.28f, 0.28f));


    float frontY = roomH / 2 + frontOpen * roomH;

    // 안쪽 면 : 방 "안" 에서 보이는 면 (앞면이 -z 를 본다)
    DrawWall(glm::vec3(0.0f, frontY, roomD / 2), Y, 180.0f, roomW, roomH, glm::vec3(0.90f, 0.80f, 0.30f));

    // 바깥쪽 면 : 방 "밖" 에서 보이는 면 (앞면이 +z 를 본다). 처음 시점(z = 17) 에서는 이 면이 방 안을 가린다.
    //   같은 벽을 한 장 더, 반대 방향(Ry 0) 으로 그리는 것이다. 컬링 때문에 한 번에 한 면만 보인다.
    //   z 를 0.01 바깥으로 밀어서 두 면이 같은 자리에 겹쳐 깜빡이는 일을 막는다.
    //   o 키로 올라가는 위치(frontY) 는 안쪽 면과 같으므로 두 면이 같이 슬라이딩한다.
    DrawWall(glm::vec3(0.0f, frontY, roomD / 2 + 0.01f), Y, 0.0f, roomW, roomH, glm::vec3(0.80f, 0.70f, 0.25f));


    DrawWall(glm::vec3(-roomW / 2, roomH / 2, 0.0f), Y, 90.0f, roomD, roomH, glm::vec3(0.30f, 0.60f, 0.35f));
    DrawWall(glm::vec3(roomW / 2, roomH / 2, 0.0f), Y, -90.0f, roomD, roomH, glm::vec3(0.25f, 0.30f, 0.70f));


    DrawWall(glm::vec3(0.0f, 0.0f, 0.0f), X, -90.0f, roomW, roomD, glm::vec3(0.45f, 0.58f, 0.42f));
    DrawWall(glm::vec3(0.0f, roomH, 0.0f), X, 90.0f, roomW, roomD, glm::vec3(0.80f, 0.80f, 0.84f));
}



void DrawObstacles()
{
    for (int i = 0; i < (int)obstacles.size(); ++i)
    {
        const Obstacle& o = obstacles[i];

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(o.x, o.sizeY / 2, o.z));
        model = glm::scale(model, glm::vec3(o.sizeX, o.sizeY, o.sizeZ));
        DrawMesh(cubeMesh, model, o.color);
    }
}



void DrawRobot()
{

    glm::mat4 robotMat = glm::mat4(1.0f);
    robotMat = glm::translate(robotMat, robot.pos);
    robotMat = glm::rotate(robotMat, glm::radians(robot.yawDeg), glm::vec3(0.0f, 1.0f, 0.0f));


    glm::mat4 torsoMat = glm::translate(robotMat, torsoCenter);
    DrawMesh(cubeMesh, glm::scale(torsoMat, torsoSize), torsoColor);


    glm::mat4 headMat = glm::translate(robotMat, headCenter);
    DrawMesh(cubeMesh, glm::scale(headMat, headSize), headColor);


    glm::mat4 noseMat = glm::translate(headMat, noseOffset);
    DrawMesh(cubeMesh, glm::scale(noseMat, noseSize), noseColor);


    float swing = SwingAmp() * sinf(robot.walkPhase);

    glm::vec3 xAxis(1.0f, 0.0f, 0.0f);



    glm::mat4 leftLegMat = glm::translate(robotMat, leftHip);                                   // 4) 엉덩이 위치로 이동
    leftLegMat = glm::rotate(leftLegMat, glm::radians(swing), xAxis);                           // 3) 엉덩이(원점) 기준 회전
    leftLegMat = glm::translate(leftLegMat, glm::vec3(0.0f, -legLength / 2, 0.0f));             // 2) 윗끝이 원점에 오게 아래로
    leftLegMat = glm::scale(leftLegMat, glm::vec3(legThick, legLength, legThick));              // 1) 크기
    DrawMesh(cubeMesh, leftLegMat, leftLegColor);


    glm::mat4 rightLegMat = glm::translate(robotMat, rightHip);
    rightLegMat = glm::rotate(rightLegMat, glm::radians(-swing), xAxis);
    rightLegMat = glm::translate(rightLegMat, glm::vec3(0.0f, -legLength / 2, 0.0f));
    rightLegMat = glm::scale(rightLegMat, glm::vec3(legThick, legLength, legThick));
    DrawMesh(cubeMesh, rightLegMat, rightLegColor);


    glm::mat4 leftArmMat = glm::translate(robotMat, leftShoulder);
    leftArmMat = glm::rotate(leftArmMat, glm::radians(-swing), xAxis);
    leftArmMat = glm::translate(leftArmMat, glm::vec3(0.0f, -armLength / 2, 0.0f));
    leftArmMat = glm::scale(leftArmMat, glm::vec3(armThick, armLength, armThick));
    DrawMesh(cubeMesh, leftArmMat, leftArmColor);


    glm::mat4 rightArmMat = glm::translate(robotMat, rightShoulder);
    rightArmMat = glm::rotate(rightArmMat, glm::radians(swing), xAxis);
    rightArmMat = glm::translate(rightArmMat, glm::vec3(0.0f, -armLength / 2, 0.0f));
    rightArmMat = glm::scale(rightArmMat, glm::vec3(armThick, armLength, armThick));
    DrawMesh(cubeMesh, rightArmMat, rightArmColor);
}

void DrawScene()
{
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    UploadCamera();

    DrawStage();
    DrawObstacles();
    DrawRobot();
}