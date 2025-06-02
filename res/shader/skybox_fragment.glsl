#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;
uniform float dayNightBlend; // ��ҹ������ӣ�0.0 Ϊ���죬1.0 Ϊҹ��

void main() {
    vec4 dayColor = texture(skybox, TexCoords); // ��������
    FragColor = dayColor;
}