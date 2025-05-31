// world.h (移除了开门逻辑)
#ifndef WORLD_H
#define WORLD_H

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "place.h"
#include "player.h"
#include "camera.h"        // 使用你提供的不依赖 Place* 的 Camera.h
#include "ballmanager.h"
#include "enemy.h"
#include "healthpackmanager.h"

class World {
private:
    GLFWwindow* window;
    glm::vec2 windowSize;

    Place* place;
    Player* player;
    Camera* camera;
    BallManager* ball;
    Enemy* enemy;
    HealthPackManager* healthPacks;

    GLuint depthMap;
    GLuint depthMapFBO;
    Shader* simpleDepthShader;
    glm::mat4 lightSpaceMatrix;

    int playerHealth;
    int maxPlayerHealth;
    bool gameOver;

public:
    World(GLFWwindow* window, glm::vec2 windowSize) {
        this->window = window;
        this->windowSize = windowSize;

        playerHealth = 99;
        maxPlayerHealth = 99;
        gameOver = false;

        simpleDepthShader = new Shader("res/shader/shadow.vert", "res/shader/shadow.frag");

        glm::vec3 lightPos(0.0, 800.0, 300.0);
        glm::mat4 lightProjection = glm::ortho(-250.0f, 250.0f, -250.0f, 250.0f, 1.0f, 1500.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        camera = new Camera(window); // Camera构造函数不再需要Place*
        place = new Place(windowSize, camera);
        player = new Player(windowSize, camera);
        ball = new BallManager(windowSize, camera);
        enemy = new Enemy(windowSize, camera, this->lightSpaceMatrix);
        healthPacks = new HealthPackManager(windowSize, camera);

        glGenFramebuffers(1, &depthMapFBO);
        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "错误::帧缓冲:: 深度帧缓冲不完整!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~World() {
        delete place;
        delete player;
        delete camera;
        delete ball;
        delete enemy;
        delete healthPacks;
        delete simpleDepthShader;
        glDeleteTextures(1, &depthMap);
        glDeleteFramebuffers(1, &depthMapFBO);
    }

    void Update(float deltaTime) {
        if (gameOver) return;

        camera->Update(deltaTime);
        place->Update();

        std::vector<glm::vec3> currentEnemyPositions = enemy->GetEnemyPositions();
        ball->UpdateEnemyPositions(currentEnemyPositions);
        ball->Update(deltaTime); // BallManager 更新 (敌人射击、子弹飞行)

        bool playerIsShooting = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        enemy->Update(camera->GetPosition(), camera->GetFront(), playerIsShooting, deltaTime);
        player->Update(deltaTime, playerIsShooting);
        healthPacks->Update(deltaTime);

        // 交互：拾取医疗包 (E键)
        static bool e_key_was_pressed = false;
        bool e_key_currently_pressed = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
        if (e_key_currently_pressed && !e_key_was_pressed) {
            if (healthPacks->TryPickupHealthPack(camera->GetPosition())) {
                if (playerHealth < maxPlayerHealth) {
                    playerHealth++;
                    std::cout << "Health restored! Current health: " << playerHealth << "/" << maxPlayerHealth << std::endl;
                } else {
                    std::cout << "Health is already full!" << std::endl;
                }
            }
            //  移除了这里的开门逻辑: // if(place) place->TryOpenDoor(camera->GetPosition());
        }
        e_key_was_pressed = e_key_currently_pressed;

        // 碰撞：敌人子弹击中玩家
        if (ball->CheckBulletHitPlayer()) {
            playerHealth--;
            std::cout << "Player hit! Remaining health: " << playerHealth << "/" << maxPlayerHealth << std::endl;
            if (playerHealth <= 0) {
                gameOver = true;
                std::cout << "Game Over! Player died!" << std::endl;
            }
        }

    }

    void Render() {
        if (gameOver) {
            glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            return;
        }
        RenderDepth();
        place->RoomRender(NULL, depthMap);
        place->SunRender();
        enemy->Render(NULL, depthMap);
        ball->Render(NULL, depthMap);
        healthPacks->Render(NULL, depthMap);
        player->Render();
    }

    GLuint GetScore() { return enemy->GetKillCount(); }
    bool IsOver() { return gameOver; }
    void SetGameModel(GLuint num) { /* ... */ }
    int GetPlayerHealth() const { return playerHealth; }
    int GetMaxPlayerHealth() const { return maxPlayerHealth; }
    size_t GetActiveHealthPackCount() const { return healthPacks->GetActiveHealthPackCount(); }

private:
    void RenderDepth() {
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        simpleDepthShader->Bind();
        simpleDepthShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);

        place->RoomRender(simpleDepthShader, 0);
        enemy->Render(simpleDepthShader, 0);
        ball->Render(simpleDepthShader, 0);
        // healthPacks->Render(simpleDepthShader, 0); // 可选

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, (GLsizei)windowSize.x, (GLsizei)windowSize.y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
};

#endif // WORLD_H