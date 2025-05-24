#ifndef ENEMY_H
#ifndef ENEMY_H
#endif  ENEMY_H

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
    GLuint maxNumber; // 当前场上敌人总数
    GLuint killCount; // 已击杀敌人数
    vec3 basicPos;
    vector<vec3> position;
    vector<float> angles;
    Camera* camera;
    mat4 model, projection, view;
public:
    Enemy(vec2 windowSize, Camera* camera) {
        this->windowSize = windowSize;
        this->camera = camera;
        basicPos = vec3(0.0, 0.0, 0.0);
        maxNumber = 6;
        killCount = 0;
        AddEnemy(maxNumber);
        LoadModel();
        LoadTexture();
        LoadShader();
    }

    void Update(vec3 pos, vec3 dir, bool isShoot) {
        this->view = camera->GetViewMatrix();
        this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f);

        // 更新朝向
        for (size_t i = 0; i < position.size(); ++i) {
            vec3 toPlayer = normalize(camera->GetPosition() - position[i]);
            angles[i] = atan2(toPlayer.x, toPlayer.z);
        }

        if (isShoot) {
            for (size_t i = 0; i < position.size(); ++i) {
                vec3 des = (pos.z - position[i].z) / (-dir.z) * dir + pos;
                float threshold=5;
                if (abs(position[i].x - des.x)<=threshold&&abs(position[i].y - des.y) <=threshold) {
                    // 击中，移除并补充
                    position.erase(position.begin() + i);
                    angles.erase(angles.begin() + i);
                    killCount++;
                    AddEnemy(1); // 补充一个
                    break; // 一次只打中一个
                }
            }
        }
        std::cout << "killCount: " << killCount << std::endl;
        for (size_t i = 0; i < position.size(); ++i) {
            std::cout << "Enemy " << i << " position: ("
                << position[i].x << ", "
                << position[i].y << ", "
                << position[i].z << ")" << std::endl;
        }

    }

    void Render(Shader* shader, GLuint depthMap = -1) {
        for (size_t i = 0; i < position.size(); i++) {
            model = mat4(1.0);
            model = translate(model, position[i]);
            model = rotate(model, angles[i], vec3(0, 1, 0));
            model = scale(model, vec3(2));
            if (shader == NULL) {
                shader = enemyShader;
                shader->Bind();
                shader->SetMat4("projection", projection);
                shader->SetMat4("view", view);
            }
            else {
                shader->Bind();
            }
            shader->SetMat4("model", model);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap->GetId());
            glActiveTexture(GL_TEXTURE1);
            glBindVertexArray(enemy->GetVAO());
            glDrawElements(GL_TRIANGLES, static_cast<GLuint>(enemy->GetIndices().size()), GL_UNSIGNED_INT, 0);
            shader->Unbind();
            glBindVertexArray(0);
            model = mat4(1.0);
        }
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

        enemyShader->Unbind();
    }
    void AddEnemy(GLuint count) {
        for (GLuint i = 0; i < count; i++) {
            int tryCount = 0;
            while (tryCount < 100) { // 防止死循环
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
};
#endif // !ENEMY_H
