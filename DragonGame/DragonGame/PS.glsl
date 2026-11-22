#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

out vec4 FragColor;

uniform vec3 uColor;         // 물체 고유 색 (DrawMesh 가 넘겨 준다)

uniform vec3 uLightPos;      // 광원 하나 - 연습장 천장 근처
uniform vec3 uLightColor;
uniform vec3 uEyePos;        // 카메라 위치 (반사광 계산용)

uniform int  uUnlit;         // 1 이면 조명을 건너뛰고 uColor 를 그대로 쓴다 (체력 바 같은 UI)

void main()
{
    if (uUnlit == 1)
    {
        FragColor = vec4(uColor, 1.0);
        return;
    }

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 V = normalize(uEyePos - vWorldPos);
    vec3 R = reflect(-L, N);

    // 주변광(반구 환경광) : 위에서 오는 빛과 바닥에서 반사되는 빛을 나눠서 섞는다.
    // 광원은 여전히 하나지만, 면이 향한 방향에 따라 환경광이 달라지기 때문에
    // 천장 광원을 등진 수직면(로봇의 앞/옆면)도 평평하게 죽지 않는다.
    const vec3 skyAmbient    = vec3(0.46, 0.48, 0.54);   // 위를 보는 면
    const vec3 groundAmbient = vec3(0.20, 0.22, 0.18);   // 아래를 보는 면

    float hemi = N.y * 0.5 + 0.5;
    vec3 ambient = mix(groundAmbient, skyAmbient, hemi) * uColor;

    // 확산광 : 면이 광원을 정면으로 볼수록 밝다.
    //          육면체는 면마다 법선이 하나씩이라, 면 단위로 밝기가 갈린다(평면 음영).
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * uColor * uLightColor;

    // 반사광 : 로봇에 금속 느낌을 살짝 준다
    float spec = pow(max(dot(V, R), 0.0), 32.0);
    vec3 specular = 0.25 * spec * uLightColor;

    // 거리 감쇠 : 광원에서 멀어질수록 어두워진다
    float dist = length(uLightPos - vWorldPos);
    float atten = 1.0 / (1.0 + 0.02 * dist + 0.004 * dist * dist);

    vec3 color = ambient + (diffuse + specular) * atten;

    FragColor = vec4(color, 1.0);
}
