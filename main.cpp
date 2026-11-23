#include <GL/glew.h>
#include <GL/freeglut.h>
#include "Robot.h"

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

unsigned int shaderProgram;
unsigned int cubeVAO, cubeVBO;

bool keyW = false, keyS = false, keyA = false, keyD = false;

Robot player;

void initOpenGL() {
    glEnable(GL_DEPTH_TEST);

    float vertices[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,

        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,   0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
         0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f,  -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
         -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,   0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
          0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
          -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,   0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
           0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void display() {
    glClearColor(0.18f, 0.18f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    glm::mat4 view = glm::lookAt(
        glm::vec3(4.5f, 4.0f, 7.5f),
        glm::vec3(0.0f, 1.8f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));

    player.drawBox(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(30.0f, 0.2f, 30.0f), glm::vec3(0.45f, 0.45f, 0.5f), shaderProgram, cubeVAO);

    player.render(shaderProgram, cubeVAO);

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'w' || key == 'W') keyW = true;
    if (key == 's' || key == 'S') keyS = true;
    if (key == 'a' || key == 'A') keyA = true;
    if (key == 'd' || key == 'D') keyD = true;

    if (key == ' ') player.startDodge();
    if (key == 'j' || key == 'J') player.startPunch();
    if (key == 'k' || key == 'K') player.startKick();
}

void keyboardUp(unsigned char key, int x, int y) {
    if (key == 'w' || key == 'W') keyW = false;
    if (key == 's' || key == 'S') keyS = false;
    if (key == 'a' || key == 'A') keyA = false;
    if (key == 'd' || key == 'D') keyD = false;
}

void timer(int value) {
    int moveDir = 0;
    if (keyW) moveDir += 1;
    if (keyS) moveDir -= 1;

    int turnDir = 0;
    if (keyA) turnDir += 1;
    if (keyD) turnDir -= 1;

    player.update(0.016f, moveDir, turnDir);

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Step 2: Robot Walk & Direction Movement");

    glewInit();
    initOpenGL();

    glutDisplayFunc(display);

    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);

    glutTimerFunc(0, timer, 0);
    glutMainLoop();
    return 0;
}