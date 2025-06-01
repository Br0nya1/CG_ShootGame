// enemy.vert
#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec2 TexCoord;
out vec3 Position;       // 世界空间位置
out vec4 PosLightSpace;  // 位置在光源裁剪空间下 (为了阴影)

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceMatrix; // <<< 新增：接收用于阴影计算的矩阵

void main() {
    Position = vec3(model * vec4(aPosition, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal; // 如果模型有非等比缩放，这个是正确的
    TexCoord = aTexCoord;

    PosLightSpace = lightSpaceMatrix * vec4(Position, 1.0); // <<< 新增：计算PosLightSpace

    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}