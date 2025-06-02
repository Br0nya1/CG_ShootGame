#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightDir; // ��Դ����ģ��̫��/������
uniform vec3 lightColor; // ��Դ��ɫ����ʱ��仯��
uniform vec3 objectColor; // ������ɫ
uniform float ambientStrength; // ������ǿ��
uniform vec3 viewPos; // ���λ�ã����ڸ߹⣩

void main() {
    // ������
    vec3 ambient = ambientStrength * lightColor;

    // ������
    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor;

    // ���淴��
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirNorm, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.5 * spec * lightColor;

    // �ϲ�����Ч��
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}