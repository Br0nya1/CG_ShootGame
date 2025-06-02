#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;
uniform float dayNightBlend; // 昼夜混合因子（0.0 为白天，1.0 为夜晚）

void main() {
    vec4 dayColor = texture(skybox, TexCoords); // 白天纹理
    FragColor = dayColor;
}