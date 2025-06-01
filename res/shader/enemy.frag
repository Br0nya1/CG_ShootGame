// enemy.frag
#version 330 core

in vec3 Normal;
in vec2 TexCoord;
in vec3 Position;
in vec4 PosLightSpace; // <<< ���������� PosLightSpace

out vec4 FragColor;

uniform sampler2D shadowMap_tex; // ��� shadowMap uniform

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

// �� room.frag ���ƹ����� ShadowCalculation ���� (�����Լ�ʵ�ֵİ汾)
float ShadowCalculation(vec4 fragPosLightSpace, sampler2D shadowMapSampler) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMapSampler, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    float bias = 0.005; // ���Ը�����Ҫ����
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    if(projCoords.z > 1.0) shadow = 0.0;
    return shadow;
}

void main() {
    // ������
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoord).rgb;

    // ������
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - Position);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseColor = light.diffuse * diff * texture(material.diffuse, TexCoord).rgb; // ����������

    // ���淴��
    vec3 viewDir = normalize(viewPos - Position);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specularColor = light.specular * spec * texture(material.specular, TexCoord).rgb; // ����������

    // ������Ӱ
    float shadow = ShadowCalculation(PosLightSpace, shadowMap_tex); // <<< ʹ�� shadowMap_tex

    // �ϳ�������ɫ��Ӧ����Ӱ
    vec3 lighting = ambient + (1.0 - shadow) * (diffuseColor + specularColor);
    FragColor = vec4(lighting, 1.0);
    vec3 texColor = texture(material.diffuse, TexCoord).rgb;
    vec3 finalAmbient = light.ambient * texColor; // ������ͨ��Ҳ���������ɫ
    vec3 finalDiffuse = light.diffuse * diff * texColor;
    vec3 finalSpecular = light.specular * spec * texture(material.specular, TexCoord).rgb; // ����ֻ�� light.specular * spec ���������ͼ�����ڻ���;��ͬ

    FragColor = vec4(finalAmbient + (1.0 - shadow) * (finalDiffuse + finalSpecular), 1.0);

}