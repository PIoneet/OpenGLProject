#version 330 core

layout (location = 0) in vec3 aPos;      // 정점 위치
layout (location = 1) in vec3 aNormal;   // 정점 법선 (그 면이 바라보는 방향)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMat;                  // transpose(inverse(mat3(model)))

out vec3 vWorldPos;                      // 조명 계산은 월드 좌표에서 한다
out vec3 vNormal;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // 팔다리처럼 한 축만 길게 늘인(비균등 스케일) 물체는
    // model 을 법선에 그대로 곱하면 법선이 면과 수직이 아니게 된다.
    // 그래서 역행렬의 전치(normalMat)를 따로 쓴다.
    vNormal = normalMat * aNormal;

    // 정점에는 오른쪽부터 적용 : 모델 -> 뷰(카메라) -> 투영
    gl_Position = projection * view * worldPos;
}
