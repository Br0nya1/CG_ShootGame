#ifndef WORLD_H
#define WORLD_H
#include "textrenderer.h"
#include <irrklang/irrKlang.h>
using namespace irrklang;
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "place.h"
#include "player.h"
#include "camera.h"
#include "ballmanager.h"
#include "enemy.h"
#include "skybox.h"
#include "healthpackmanager.h"

ISoundEngine* gangguan = createIrrKlangDevice();

class World {
private:
    GLFWwindow* window;
    glm::vec2 windowSize;

    Place* place;
    Player* player;
    Camera* camera;
    BallManager* ball;
    Enemy* enemy;
    Skybox* skybox; // Skybox for rendering dynamic sky
    HealthPackManager* healthPacks;

    GLuint depthMap;
    GLuint depthMapFBO;
    Shader* simpleDepthShader;
    Shader* textShader;
    TextRenderer* textRenderer;
    glm::mat4 lightSpaceMatrix;

    int playerHealth;
    int maxPlayerHealth;
    bool gameOver;

    // Day-night cycle variables
    float gameTime; // Tracks game time for day-night cycle
    glm::vec3 lightDir; // Light direction (simulates sun/moon)
    glm::vec3 lightColor; // Light color (changes with time)
    float ambientStrength; // Ambient light strength
    float dayNightCycle; // Normalized [0, 1] for day-night transition

public:
    World(GLFWwindow* window, glm::vec2 windowSize) : gameTime(0.0f) {
        this->window = window;
        this->windowSize = windowSize;

        playerHealth = 10;
        maxPlayerHealth = 10;
        gameOver = false;

        // Initialize shaders
        textShader = new Shader("res/shader/text.vert", "res/shader/text.frag");
        textRenderer = new TextRenderer("res/font/msyh.ttf", textShader->GetProgram());
        simpleDepthShader = new Shader("res/shader/shadow.vert", "res/shader/shadow.frag");

        // Initialize light parameters
        lightDir = glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f));
        lightColor = glm::vec3(1.0f, 1.0f, 0.9f); // Default: daylight
        ambientStrength = 0.3f;
        dayNightCycle = 0.0f;

        // Initialize light space matrix for shadow mapping
        glm::vec3 lightPos(0.0f, 800.0f, 300.0f);
        glm::mat4 lightProjection = glm::ortho(-250.0f, 250.0f, -250.0f, 250.0f, 1.0f, 1500.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        lightSpaceMatrix = lightProjection * lightView;

        // Initialize game objects
        camera = new Camera(window);
        place = new Place(windowSize, camera);
        player = new Player(windowSize, camera);
        ball = new BallManager(windowSize, camera);
        enemy = new Enemy(windowSize, camera, this->lightSpaceMatrix);
        healthPacks = new HealthPackManager(windowSize, camera);
        skybox = new Skybox();

        // Initialize shadow map framebuffer
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
            std::cout << "Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~World() {
        delete place;
        delete player;
        delete camera;
        delete ball;
        delete enemy;
        delete healthPacks;
        delete skybox;
        delete simpleDepthShader;
        delete textShader;
        glDeleteTextures(1, &depthMap);
        glDeleteFramebuffers(1, &depthMapFBO);
    }

    void Update(float deltaTime) {

        // Update day-night cycle
        gameTime += deltaTime;
        float cycleTime = fmod(gameTime, 60.0f); // 60-second cycle
        float t = 0.5f * (1.0f - cos(cycleTime * glm::pi<float>() / 30.0f)); // Smooth curve
        if (cycleTime < 30.0f) { // Day: 0-30 seconds
            dayNightCycle = (t * cycleTime / 30.0f) * 0.5f;
        }
        else if (cycleTime < 45.0f) { // Dusk: 30-45 seconds
            dayNightCycle = 0.5f + (t * (cycleTime - 30.0f) / 15.0f) * 0.25f;
        }
        else { // Night: 45-60 seconds
            dayNightCycle = 0.75f + (t * (cycleTime - 45.0f) / 15.0f) * 0.25f;
        }

        float angle = dayNightCycle * 2.0f * glm::pi<float>(); // Convert to radians for light direction
        lightDir = glm::normalize(glm::vec3(cos(angle), sin(angle), -1.0f)); // Simulate sun/moon movement
        if (dayNightCycle < 0.5f) { // Day (extended to 0-0.5)
            lightColor = glm::vec3(1.0f, 1.0f, 0.9f); // Warm white
            ambientStrength = 0.3f;
        }
        else if (dayNightCycle < 0.75f) { // Dusk (0.5-0.75)
            lightColor = glm::vec3(0.8f, 0.5f, 0.3f); // Orange-red for dusk
            ambientStrength = 0.2f;
        }
        else { // Night (0.75-1.0)
            lightColor = glm::vec3(0.2f, 0.2f, 0.5f); // Cool blue
            ambientStrength = 0.1f;
        }

        // Update light space matrix dynamically
        glm::vec3 lightPos = lightDir * -800.0f; // Position light based on direction
        glm::mat4 lightProjection = glm::ortho(-250.0f, 250.0f, -250.0f, 250.0f, 1.0f, 1500.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        lightSpaceMatrix = lightProjection * lightView;

        // Update game objects
        camera->Update(deltaTime);
        place->Update();
        std::vector<glm::vec3> currentEnemyPositions = enemy->GetEnemyPositions();
        ball->UpdateEnemyPositions(currentEnemyPositions);
        ball->Update(deltaTime, GetScore());
        bool playerIsShooting = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        enemy->Update(camera->GetPosition(), camera->GetFront(), playerIsShooting, deltaTime);
        player->Update(deltaTime, playerIsShooting);
        healthPacks->Update(deltaTime);

        // Handle health pack pickup (E key)
        static bool e_key_was_pressed = false;
        bool e_key_currently_pressed = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
        if (e_key_currently_pressed && !e_key_was_pressed) {
            if (healthPacks->TryPickupHealthPack(camera->GetPosition())) {
                if (playerHealth < maxPlayerHealth) {
                    playerHealth++;
                    std::cout << "Health restored! Current health: " << playerHealth << "/" << maxPlayerHealth << std::endl;
                }
                else {
                    std::cout << "Health is already full!" << std::endl;
                }
            }
        }
        e_key_was_pressed = e_key_currently_pressed;

        // Handle bullet collision with player
        if (ball->CheckBulletHitPlayer()) {
            playerHealth--;
            gangguan->play2D("res/audio/gangguan.mp3", GL_FALSE);
            std::cout << "Player hit! Remaining health: " << playerHealth << "/" << maxPlayerHealth << std::endl;
            if (playerHealth <= 0) {
                gameOver = true;
                std::cout << "Game Over! Player died!" << std::endl;
            }
        }
    }

    void Render() {


        // Render shadow map
        RenderDepth();

        // Render skybox
        glDepthMask(GL_FALSE);
        skybox->Render(camera->GetViewMatrix(), glm::perspective(glm::radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f), dayNightCycle);
        glDepthMask(GL_TRUE);

        // Render scene with dynamic lighting
        place->RoomRender(NULL, depthMap);
        place->SunRender();
        enemy->Render(NULL, depthMap);
        ball->Render(NULL, depthMap);
        healthPacks->Render(NULL, depthMap);
        player->Render();

        // Render UI text
        std::wstring scoreStr = L"得分: " + std::to_wstring(GetScore());
        std::wstring healthStr = L"血量: " + std::to_wstring(GetPlayerHealth()) + L"/" + std::to_wstring(GetMaxPlayerHealth());
        textRenderer->RenderText(scoreStr, 25.0f, windowSize.y - 50.0f, 1.0f, glm::vec3(1, 1, 0), windowSize.x, windowSize.y);
        textRenderer->RenderText(healthStr, 25.0f, windowSize.y - 100.0f, 1.0f, glm::vec3(0, 1, 0), windowSize.x, windowSize.y);
        if (gameOver) {
            if (gameOver) {
                std::wstring gameover = L"游戏结束,按q键退出";
                float scale = 2.5f;
                // 简单估算文本宽度（每个字符大约占30像素*缩放）
                float textWidth = gameover.length() * 30.0f * scale;
                float x = (windowSize.x - textWidth) / 2.0f;
                float y = windowSize.y / 2.0f;
                textRenderer->RenderText(gameover, x, y, scale, glm::vec3(1, 0, 0), windowSize.x, windowSize.y);
                return;
            }
        }
    }

    GLuint GetScore() { return enemy->GetKillCount(); }
    bool IsOver() { return gameOver; }
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
        // healthPacks->Render(simpleDepthShader, 0); // Optional

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, (GLsizei)windowSize.x, (GLsizei)windowSize.y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
};

#endif // WORLD_H