#ifndef WORLD_H
#define WORLD_H

#include <GLFW/glfw3.h>
#include "place.h"
#include "player.h"
#include "camera.h"
#include "ballmanager.h"
#include "enemy.h"

class World {
private:
	GLFWwindow* window;
	vec2 windowSize;

	Place* place;				// 场地
	Player* player;				// 玩家
	Camera* camera;				// 摄像头
	BallManager* ball;			// 小球
	Enemy* enemy;
	// 阴影
	GLuint depthMap;
	GLuint depthMapFBO;
	Shader* simpleDepthShader;
	mat4 lightSpaceMatrix;
	
	// 新增：玩家生命值系统
	int playerHealth;
	bool gameOver;
public:
	World(GLFWwindow* window, vec2 windowSize) {
		this->window = window;
		this->windowSize = windowSize;
		
		// 初始化玩家生命值系统
		playerHealth = 3;  // 玩家有3条命
		gameOver = false;

		simpleDepthShader = new Shader("res/shader/shadow.vert", "res/shader/shadow.frag");

		vec3 lightPos(0.0, 400.0, 150.0);
		mat4 lightProjection = ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 500.0f);
		mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;

		camera = new Camera(window);
		place = new Place(windowSize, camera);
		player = new Player(windowSize, camera);
		ball = new BallManager(windowSize, camera);
		enemy = new Enemy(windowSize, camera, this->lightSpaceMatrix);

		glGenFramebuffers(1, &depthMapFBO);
		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	// 更新场景
	void Update(float deltaTime) {
		if (gameOver) return;  // 如果游戏结束，停止更新
		
		camera->Update(deltaTime);
		
		// 获取敌人位置并更新BallManager中的敌人射击器
		vector<vec3> enemyPositions = enemy->GetEnemyPositions();
		ball->UpdateEnemyPositions(enemyPositions);
		
		// 检查敌人子弹是否击中玩家
		if (ball->CheckBulletHitPlayer()) {
			playerHealth--;
			cout << "Player hit! Remaining health: " << playerHealth << endl;
			
			if (playerHealth <= 0) {
				gameOver = true;
				cout << "Game Over! Player died!" << endl;
			}
		}
		
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			ball->Update(camera->GetPosition(), camera->GetFront(), true, deltaTime);
			enemy->Update(camera->GetPosition(), camera->GetFront(), true, deltaTime);
			player->Update(deltaTime, true);
		}
		else {
			ball->Update(camera->GetPosition(), camera->GetFront(), false, deltaTime);
			enemy->Update(camera->GetPosition(), camera->GetFront(), false, deltaTime);
			player->Update(deltaTime, false);
		}
		place->Update();
	}
	// 阴影模式
	void Render() {
		RenderDepth();
		player->Render();
		place->RoomRender(NULL, depthMap);
		place->SunRender();
		ball->Render(NULL, depthMap);
		enemy->Render(NULL, depthMap);
	}

	GLuint GetScore() {
		return ball->GetScore();
	}
	// 判断游戏是否结束
	bool IsOver() {
		return gameOver || ball->IsOver();
	}
	// 设置游戏模式
	void SetGameModel(GLuint num) {
		ball->SetGameModel(num);
	}
	// 新增：获取玩家生命值
	int GetPlayerHealth() const {
		return playerHealth;
	}
private:
	// 阴影渲染
	void RenderDepth() {
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		simpleDepthShader->Bind();
		simpleDepthShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, 1024, 1024);
		glClear(GL_DEPTH_BUFFER_BIT);
		place->RoomRender(simpleDepthShader);
		ball->Render(simpleDepthShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, windowSize.x, windowSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
};

#endif
