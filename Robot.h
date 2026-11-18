#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>

class Robot {
public:
    glm::vec3 position = glm::vec3(0.0f, 2.4f, 0.0f);
    float bodyYaw = 0.0f;
    glm::vec3 mainColor = glm::vec3(0.2f, 0.45f, 0.85f);

    // 숨쉬기 리듬용 높이 오프셋 및 타이머
    float idleTimer = 0.0f;
    float bodyBobOffsetY = 0.0f;

    float torsoTwistY = 0.0f;
    float torsoPitchX = 0.0f;
    float headYaw = 0.0f, headPitch = 0.0f;

    float rShoulderX = 0.0f, rShoulderZ = 0.0f, rElbowX = 0.0f;
    float lShoulderX = 0.0f, lShoulderZ = 0.0f, lElbowX = 0.0f;

    float rHipX = 0.0f, rKneeX = 0.0f, rAnkleX = 0.0f;
    float lHipX = 0.0f, lKneeX = 0.0f, lAnkleX = 0.0f;

    void updateIdle(float dt) {
        idleTimer += dt * 4.5f; // 숨쉬기 리듬 속도

        float bob = sin(idleTimer);
        bodyBobOffsetY = bob * 0.04f;

        torsoTwistY = -20.0f;
        headYaw = 20.0f;

        rShoulderX = -45.0f + bob * 3.0f;
        rShoulderZ = 15.0f;
        rElbowX = -100.0f - bob * 3.0f;

        lShoulderX = -60.0f - bob * 3.0f;
        lShoulderZ = -15.0f;
        lElbowX = -85.0f + bob * 3.0f;

        rHipX = 12.0f;  rKneeX = 20.0f; rAnkleX = -8.0f;
        lHipX = -15.0f; lKneeX = 20.0f; lAnkleX = -5.0f;
    }

    void drawBox(glm::mat4 jointMat, glm::vec3 pivotOffset, glm::vec3 size, glm::vec3 color, unsigned int shaderID, unsigned int vao) {
        glm::mat4 renderMat = glm::translate(jointMat, pivotOffset);
        renderMat = glm::scale(renderMat, size);

        int modelLoc = glGetUniformLocation(shaderID, "uModel");
        int colorLoc = glGetUniformLocation(shaderID, "uColor");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderMat));
        glBindVertexArray(vao);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glUniform3fv(colorLoc, 1, glm::value_ptr(color));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDisable(GL_POLYGON_OFFSET_FILL);

        glUniform3f(colorLoc, 0.1f, 0.1f, 0.1f);
        glLineWidth(1.5f);
        glDrawArrays(GL_LINES, 36, 24);
    }

    void render(unsigned int shaderID, unsigned int vao) {
        glm::vec3 white(0.92f, 0.92f, 0.92f);
        glm::vec3 red(0.85f, 0.22f, 0.22f);
        glm::vec3 darkGray(0.28f, 0.28f, 0.32f);

        glm::vec3 currentPos = position + glm::vec3(0.0f, bodyBobOffsetY, 0.0f);
        glm::mat4 torsoMat = glm::translate(glm::mat4(1.0f), currentPos);
        torsoMat = glm::rotate(torsoMat, glm::radians(bodyYaw + torsoTwistY), glm::vec3(0, 1, 0));
        torsoMat = glm::rotate(torsoMat, glm::radians(torsoPitchX), glm::vec3(1, 0, 0));
        drawBox(torsoMat, glm::vec3(0.0f), glm::vec3(1.0f, 1.2f, 0.5f), mainColor, shaderID, vao);

        glm::mat4 neckJoint = glm::translate(torsoMat, glm::vec3(0.0f, 0.6f, 0.0f));
        neckJoint = glm::rotate(neckJoint, glm::radians(headYaw), glm::vec3(0, 1, 0));
        neckJoint = glm::rotate(neckJoint, glm::radians(headPitch), glm::vec3(1, 0, 0));

        drawBox(neckJoint, glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f), white, shaderID, vao);
        drawBox(neckJoint, glm::vec3(0.0f, 0.3f, 0.26f), glm::vec3(0.4f, 0.12f, 0.05f), red, shaderID, vao);

        glm::mat4 rShoulderJoint = glm::translate(torsoMat, glm::vec3(0.65f, 0.5f, 0.0f));
        rShoulderJoint = glm::rotate(rShoulderJoint, glm::radians(rShoulderX), glm::vec3(1, 0, 0));
        rShoulderJoint = glm::rotate(rShoulderJoint, glm::radians(rShoulderZ), glm::vec3(0, 0, 1));
        drawBox(rShoulderJoint, glm::vec3(0.0f, -0.35f, 0.0f), glm::vec3(0.3f, 0.7f, 0.3f), white, shaderID, vao);

        glm::mat4 rElbowJoint = glm::translate(rShoulderJoint, glm::vec3(0.0f, -0.7f, 0.0f));
        rElbowJoint = glm::rotate(rElbowJoint, glm::radians(rElbowX), glm::vec3(1, 0, 0));
        drawBox(rElbowJoint, glm::vec3(0.0f, -0.3f, 0.0f), glm::vec3(0.25f, 0.6f, 0.25f), mainColor, shaderID, vao);

        glm::mat4 rWristJoint = glm::translate(rElbowJoint, glm::vec3(0.0f, -0.6f, 0.0f));
        drawBox(rWristJoint, glm::vec3(0.0f, -0.15f, 0.0f), glm::vec3(0.32f, 0.32f, 0.32f), red, shaderID, vao);

        glm::mat4 lShoulderJoint = glm::translate(torsoMat, glm::vec3(-0.65f, 0.5f, 0.0f));
        lShoulderJoint = glm::rotate(lShoulderJoint, glm::radians(lShoulderX), glm::vec3(1, 0, 0));
        lShoulderJoint = glm::rotate(lShoulderJoint, glm::radians(lShoulderZ), glm::vec3(0, 0, 1));
        drawBox(lShoulderJoint, glm::vec3(0.0f, -0.35f, 0.0f), glm::vec3(0.3f, 0.7f, 0.3f), white, shaderID, vao);

        glm::mat4 lElbowJoint = glm::translate(lShoulderJoint, glm::vec3(0.0f, -0.7f, 0.0f));
        lElbowJoint = glm::rotate(lElbowJoint, glm::radians(lElbowX), glm::vec3(1, 0, 0));
        drawBox(lElbowJoint, glm::vec3(0.0f, -0.3f, 0.0f), glm::vec3(0.25f, 0.6f, 0.25f), mainColor, shaderID, vao);

        glm::mat4 lWristJoint = glm::translate(lElbowJoint, glm::vec3(0.0f, -0.6f, 0.0f));
        drawBox(lWristJoint, glm::vec3(0.0f, -0.15f, 0.0f), glm::vec3(0.32f, 0.32f, 0.32f), red, shaderID, vao);

        glm::mat4 rHipJoint = glm::translate(torsoMat, glm::vec3(0.3f, -0.6f, 0.0f));
        rHipJoint = glm::rotate(rHipJoint, glm::radians(rHipX), glm::vec3(1, 0, 0));
        drawBox(rHipJoint, glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.35f, 0.8f, 0.35f), darkGray, shaderID, vao);

        glm::mat4 rKneeJoint = glm::translate(rHipJoint, glm::vec3(0.0f, -0.8f, 0.0f));
        rKneeJoint = glm::rotate(rKneeJoint, glm::radians(rKneeX), glm::vec3(1, 0, 0));
        drawBox(rKneeJoint, glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.3f, 0.8f, 0.3f), white, shaderID, vao);

        glm::mat4 rAnkleJoint = glm::translate(rKneeJoint, glm::vec3(0.0f, -0.8f, 0.0f));
        rAnkleJoint = glm::rotate(rAnkleJoint, glm::radians(rAnkleX), glm::vec3(1, 0, 0));
        drawBox(rAnkleJoint, glm::vec3(0.0f, -0.1f, 0.1f), glm::vec3(0.35f, 0.2f, 0.5f), red, shaderID, vao);

        glm::mat4 lHipJoint = glm::translate(torsoMat, glm::vec3(-0.3f, -0.6f, 0.0f));
        lHipJoint = glm::rotate(lHipJoint, glm::radians(lHipX), glm::vec3(1, 0, 0));
        drawBox(lHipJoint, glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.35f, 0.8f, 0.35f), darkGray, shaderID, vao);

        glm::mat4 lKneeJoint = glm::translate(lHipJoint, glm::vec3(0.0f, -0.8f, 0.0f));
        lKneeJoint = glm::rotate(lKneeJoint, glm::radians(lKneeX), glm::vec3(1, 0, 0));
        drawBox(lKneeJoint, glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.3f, 0.8f, 0.3f), white, shaderID, vao);

        glm::mat4 lAnkleJoint = glm::translate(lKneeJoint, glm::vec3(0.0f, -0.8f, 0.0f));
        lAnkleJoint = glm::rotate(lAnkleJoint, glm::radians(lAnkleX), glm::vec3(1, 0, 0));
        drawBox(lAnkleJoint, glm::vec3(0.0f, -0.1f, 0.1f), glm::vec3(0.35f, 0.2f, 0.5f), red, shaderID, vao);
    }
};