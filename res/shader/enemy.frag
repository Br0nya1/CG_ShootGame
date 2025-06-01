// enemy.frag
#version 330 core

in vec3 Normal;
in vec2 TexCoord;
in vec3 Position;
in vec4 PosLightSpace; // <<< 新增：接收 PosLightSpace

out vec4 FragColor;

uniform sampler2D shadowMap_tex; // 你的 shadowMap uniform

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};
uniform Material material;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

uniform vec3 viewPos;

// 从 room.frag 复制过来的 ShadowCalculation 函数 (或你自己实现的版本)
float ShadowCalculation(vec4 fragPosLightSpace, sampler2D shadowMapSampler) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMapSampler, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    float bias = 0.005; // 可以根据需要调整
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    if(projCoords.z > 1.0) shadow = 0.0;
    return shadow;
}

void main() {
    // 环境光
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoord).rgb;

    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - Position);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseColor = light.diffuse * diff * texture(material.diffuse, TexCoord).rgb; // 变量名区分

    // 镜面反射
    vec3 viewDir = normalize(viewPos - Position);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specularColor = light.specular * spec * texture(material.specular, TexCoord).rgb; // 变量名区分

    // 计算阴影
    float shadow = ShadowCalculation(PosLightSpace, shadowMap_tex); // <<< 使用 shadowMap_tex

    // 合成最终颜色，应用阴影
    vec3 lighting = ambient + (1.0 - shadow) * (diffuseColor + specularColor);
    FragColor = vec4(lighting, 1.0);
    vec3 texColor = texture(material.diffuse, TexCoord).rgb;
    vec3 finalAmbient = light.ambient * texColor; // 环境光通常也乘以物体基色
    vec3 finalDiffuse = light.diffuse * diff * texColor;
    vec3 finalSpecular = light.specular * spec * texture(material.specular, TexCoord).rgb; // 或者只用 light.specular * spec 如果镜面贴图不存在或用途不同

    FragColor = vec4(finalAmbient + (1.0 - shadow) * (finalDiffuse + finalSpecular), 1.0);

}