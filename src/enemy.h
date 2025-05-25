#ifndef ENEMY_H 
#define ENEMY_H

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
using namespace glm;
#include <cstdlib>
#include <ctime>
#include <vector>
using namespace std;
#include "model.h"
#include "shader.h"
#include "camera.h"
#include "texture.h"
class Enemy {
private:
    vec2 windowSize;
    Model* enemy;
    Shader* enemyShader;
    Texture* diffuseMap;
    GLuint maxNumber; // 当前场上的敌人数量
    GLuint killCount; // 已击杀敌人数
    vec3 basicPos;
    vector<vec3> position;
    vector<float> angles;
    Camera* camera;
    mat4 model, projection, view;
    mat4 lightSpaceMatrix;
    
    // 新增：定时生成敌人的系统
    float spawnTimer;       // 生成计时器
    float spawnInterval;    // 生成间隔（秒）
    GLuint maxEnemyLimit;   // 场上敌人数量上限
public:
    Enemy(vec2 windowSize, Camera* camera, mat4 lightSpaceMat) : lightSpaceMatrix(lightSpaceMat){
        this->windowSize = windowSize;
        this->camera = camera;
        basicPos = vec3(0.0, 0.0, 0.0);
        maxNumber = 6;
        killCount = 0;
        
        // 初始化定时生成系统
        spawnTimer = 0.0f;
        spawnInterval = 2.0f;   // 每2秒生成一个敌人
        maxEnemyLimit = 20;     // 场上最多20个敌人
        
        AddEnemy(maxNumber);
        LoadModel();
        LoadTexture();
        LoadShader();
    }

    void Update(vec3 pos, vec3 dir, bool isShoot, float deltaTime) {
        this->view = camera->GetViewMatrix();
        this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f);

        // 更新朝向
        for (size_t i = 0; i < position.size(); ++i) {
            vec3 toPlayer = normalize(camera->GetPosition() - position[i]);
            angles[i] = atan2(toPlayer.x, toPlayer.z);
        }

        // 处理玩家射击
        if (isShoot) {
            for (size_t i = 0; i < position.size(); ++i) {
                vec3 des = (pos.z - position[i].z) / (-dir.z) * dir + pos;
                float threshold=5;
                if (abs(position[i].x - des.x)<=threshold&&abs(position[i].y - des.y) <=threshold) {
                    // 命中，移除敌人
                    position.erase(position.begin() + i);
                    angles.erase(angles.begin() + i);
                    killCount++;
                    cout << "Enemy killed! Current kill count: " << killCount << endl;
                    break; // 一次只击杀一个
                }
            }
        }
        
        // 定时生成新敌人
        UpdateEnemySpawning(deltaTime);
    }

    void Render(Shader* shaderToUse, GLuint depthMapID = 0) { // 参数名和类型与Place类中类似
        if (!enemy) return; // 检查模型是否已加载

        const auto& enemySubMeshes = enemy->GetSubMeshes();
        if (enemySubMeshes.empty()) return; // 如果模型没有子网格，则不渲染

        const auto& firstSubMesh = enemySubMeshes[0]; // 假设敌人模型是单个子网格，或只渲染第一个

        for (size_t i = 0; i < position.size(); i++) {
            model = glm::mat4(1.0); // 明确 glm::
            model = glm::translate(model, position[i]); // 明确 glm::
            model = glm::rotate(model, angles[i], glm::vec3(0, 1, 0)); // 明确 glm::
            model = glm::scale(model, glm::vec3(2)); // 明确 glm::

            Shader* currentShader = shaderToUse;
            if (currentShader == NULL) {
                currentShader = enemyShader;
                currentShader->Bind();
                currentShader->SetMat4("projection", projection);
                currentShader->SetMat4("view", view);
                // 如果 enemyShader 需要其他 uniforms (如 viewPos, lightPos)，也应在此处设置
                currentShader->SetVec3("viewPos", camera->GetPosition());
                // lightPos 等已在 LoadShader 中为 enemyShader 设置过，如果是静态的就不用每帧传
            }
            else {
                currentShader->Bind();
            }

            currentShader->SetMat4("model", model);

            // 纹理和深度图绑定
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap->GetId()); // 敌人自己的漫反射贴图

            if (currentShader == enemyShader && depthMapID != 0) { // 如果使用 enemyShader 且有深度图
                glActiveTexture(GL_TEXTURE1); // 假设 enemyShader 的 shadowMap 在纹理单元1
                glBindTexture(GL_TEXTURE_2D, depthMapID);
                currentShader->SetInt("shadowMap_tex", 1); // 假设你的 enemyShader 中阴影贴图 uniform 名为 material.shadowMap
                // 或者根据实际 uniform 名进行设置，如 "shadowMap"
                // 你的 LoadShader 中设置的是 "material.diffuse" 和 "material.specular"
                // 你需要为 enemyShader 添加 shadowMap uniform
            }


            // 修改开始: 使用子网格数据进行渲染
            glBindVertexArray(firstSubMesh.VAO);
            glDrawElements(GL_TRIANGLES, firstSubMesh.indexCount, GL_UNSIGNED_INT, 0);
            // 修改结束

            // 不要在循环内部解绑 VAO 和 Shader，除非每个迭代都用不同的
        }

        // 在所有敌人渲染完毕后解绑
        if (shaderToUse == NULL && enemyShader) {
            enemyShader->Unbind();
        } else if (shaderToUse != NULL) {
            // shaderToUse->Unbind(); // 通常由调用RenderDepth的地方统一处理其shader解绑
        }
        glBindVertexArray(0); // 最后解绑VAO
    }

    GLuint GetKillCount() {
        return killCount;
    }
    
    // 获取当前敌人数量
    size_t GetEnemyCount() const {
        return position.size();
    }
    
    // 获取敌人数量上限
    GLuint GetMaxEnemyLimit() const {
        return maxEnemyLimit;
    }
    
    // 新增：获取所有敌人位置，供子弹系统使用
    vector<vec3> GetEnemyPositions() const {
        return position;
    }

private:
    void LoadModel() {
        enemy = new Model("res/model/airen.obj");

    }
    void LoadTexture() {
        diffuseMap = new Texture("res/texture/airen.jpg");

    }
    void LoadShader()
    {
        enemyShader = new Shader("res/shader/enemy.vert", "res/shader/enemy.frag");
        enemyShader->Bind();
        enemyShader->SetInt("material.diffuse", 0);
        enemyShader->SetInt("material.specular", 1);
        enemyShader->SetFloat("material.shininess", 64.0);
        enemyShader->SetVec3("light.position", vec3(0.0, 400.0, 150.0));
        enemyShader->SetVec3("light.ambient", vec3(0.2));
        enemyShader->SetVec3("light.diffuse", vec3(0.65));
        enemyShader->SetVec3("light.specular", vec3(1.0));
        enemyShader->SetVec3("viewPos", camera->GetPosition());
        enemyShader->SetInt("shadowMap_tex", 1); // 告诉着色器 shadowMap_tex 在单元1
        enemyShader->SetMat4("lightSpaceMatrix", this->lightSpaceMatrix); // <<< 新增：传递 lightSpaceMatrix

        enemyShader->Unbind();
    }
    void AddEnemy(GLuint count) {
        for (GLuint i = 0; i < count; i++) {
            int tryCount = 0;
            while (tryCount < 100) { // ֹѭ
                float x = (rand() % 60) - 30;
                float z = (rand() % 60) - 30;
                float y = 11;
                vec3 pos = vec3(x,y,z);
                if (CheckPosition(pos)) {
                    position.push_back(pos);
                    angles.push_back(0.0f);
                    break;
                }
                tryCount++;
            }
        }
    }
    bool CheckPosition(vec3 pos) {
        for (size_t i = 0; i < position.size(); i++) {
            float away = pow(position[i].x - pos.x, 2) + pow(position[i].y - pos.y, 2);
            if (away < 100)
                return false;
        }
        return true;
    }
    
    // 新增：定时生成敌人的方法
    void UpdateEnemySpawning(float deltaTime) {
        spawnTimer += deltaTime;
        
        if (spawnTimer >= spawnInterval && position.size() < maxEnemyLimit) {
            AddEnemy(1);
            spawnTimer = 0.0f;
            cout << "New enemy spawned! Current enemy count: " << position.size() << endl;
        }
    }
};
#endif // !ENEMY_H
