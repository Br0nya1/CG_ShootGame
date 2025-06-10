#ifndef BALLMANAGER_H
#define BALLMANAGER_H

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

// Bullet structure
struct Bullet {
	vec3 position;		// Bullet position
	vec3 direction;		// Bullet flight direction
	float speed;		// Bullet speed
	float lifetime;		// Bullet lifetime
	
	// 动画相关
	int currentFrameIndex; // 当前动画帧的索引 (0 到 N-1)
	float frameTimer;      // 当前帧已显示的时间
	float frameDuration;   // 每帧的持续时间 (例如，0.1秒)

	Bullet(glm::vec3 pos, glm::vec3 dir, float spd = 100.0f, float frameDur = 0.1f) // 增加了默认子弹速度和帧持续时间
		: position(pos), direction(glm::normalize(dir)), speed(spd), lifetime(5.0f),
		currentFrameIndex(0), frameTimer(0.0f), frameDuration(frameDur) {}
};

float firerate=2.0f;
// Enemy shooter structure (independent firing timer for each enemy)
struct EnemyShooter {
	vec3 enemyPosition;
	float fireTimer;
	float fireRate;
	
	EnemyShooter(vec3 pos, float rate ) 
		: enemyPosition(pos), fireTimer(0.0f), fireRate(firerate) {}
};

class BallManager {
private:
	glm::vec2 windowSize;

	// Model* ball; // 不再是单个模型，而是一个模型数组
	std::vector<Model*> bulletFrames; // 存储子弹的所有动画帧模型
	int numBulletFrames;              // 子弹动画的总帧数

	Shader* ballShader; // 子弹共用一个着色器
	GLuint score;
	// GLuint gameModel; // 如果游戏模式影响子弹行为，则保留
	glm::vec3 lightPos;
	glm::mat4 lightSpaceMatrix;

	std::vector<Bullet> bullets;
	std::vector<EnemyShooter> enemyShooters;

	Camera* camera;
	glm::mat4 model_matrix_temp; // 避免与 Model 类名冲突，并明确是临时变量
	glm::mat4 projection;
	glm::mat4 view;
public:
	BallManager(glm::vec2 windowSize, Camera* camera) {
		this->windowSize = windowSize;
		this->camera = camera;
		score = 0;
		this->lightPos = glm::vec3(0.0, 400.0, 150.0);
		glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 500.0f);
		glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		this->lightSpaceMatrix = lightProjection * lightView;

		numBulletFrames = 0; // 初始化帧数
		LoadModelsAndShader(); // 修改函数名，加载多个模型和着色器
	}

	~BallManager() { // 添加析构函数来释放模型资源
		for (Model* frame : bulletFrames) {
			delete frame;
		}
		bulletFrames.clear();
		delete ballShader;
	}
	
	// Update enemy shooter positions
	void UpdateEnemyPositions(const vector<vec3>& enemyPositions) {
		// Only recreate shooters if enemy count changes
		if (enemyShooters.size() != enemyPositions.size()) {
			enemyShooters.clear();
			
			// Create shooter for each enemy
			for (const auto& pos : enemyPositions) {
				// Each enemy has slightly different firing rate for randomness
				float randomFireRate = 1.5f + (rand() % 200) / 100.0f; // 1.5-3.5 seconds
				enemyShooters.push_back(EnemyShooter(pos, randomFireRate));
			}
		} else {
			// Only update existing shooter positions
			for (size_t i = 0; i < enemyPositions.size() && i < enemyShooters.size(); i++) {
				enemyShooters[i].enemyPosition = enemyPositions[i];
			}
		}
	}
	
	// Add single bullet
	void AddBullet(vec3 enemyPos, vec3 playerPos) {
		vec3 direction = playerPos - enemyPos;
		// Slightly raise bullet start position to avoid ground collision
		vec3 bulletStartPos = enemyPos + vec3(0.0f, 2.0f, 0.0f);
		bullets.push_back(Bullet(bulletStartPos, direction));
	}
	
	// Check if bullet hits player
	bool CheckBulletHitPlayer(float hitRadius = 5.0f) {  // Increased collision radius from 2.0f to 5.0f
		vec3 playerPos = camera->GetPosition();
		
		for (size_t i = 0; i < bullets.size(); i++) {
			float distance = length(bullets[i].position - playerPos);
			if (distance <= hitRadius) {
				cout << "Player hit! Distance: " << distance << ", Radius: " << hitRadius << endl;
				bullets.erase(bullets.begin() + i);
				return true;  // Player hit
			}
		}
		return false;
	}
	
	// Update bullets and shooting logic
	void Update(float deltaTime,GLuint score) {
		this->view = camera->GetViewMatrix();
		this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f);
		firerate =firerate*score/(score+1.0f);

		// Update enemy shooting timers and fire bullets
		UpdateEnemyShooting(deltaTime);
		
		// Update bullet positions
		UpdateBullets(deltaTime);

	}
	
	// Update enemy shooting logic
	void UpdateEnemyShooting(float deltaTime) {
		vec3 playerPos = camera->GetPosition();
		
		for (auto& shooter : enemyShooters) {
			shooter.fireTimer += deltaTime;
			
			if (shooter.fireTimer >= shooter.fireRate) {
				AddBullet(shooter.enemyPosition, playerPos);
				shooter.fireTimer = 0.0f;
				
				// 重置射击间隔，增加一些随机性
				shooter.fireRate = 1.5f + (rand() % 200) / 100.0f;
			}
		}
	}
	
	// 修改 UpdateBullets 方法以处理动画帧更新
	void UpdateBullets(float deltaTime) {
		for (size_t i = 0; i < bullets.size(); ++i) { // 使用 ++i
			// 移动子弹
			bullets[i].position += bullets[i].direction * bullets[i].speed * deltaTime; // 确保乘以 deltaTime
			bullets[i].lifetime -= deltaTime;

			// 更新动画帧
			if (numBulletFrames > 0) { // 仅当有动画帧时才更新
				bullets[i].frameTimer += deltaTime;
				if (bullets[i].frameTimer >= bullets[i].frameDuration) {
					bullets[i].frameTimer -= bullets[i].frameDuration; // 或者 bullets[i].frameTimer = 0.0f;
					bullets[i].currentFrameIndex = (bullets[i].currentFrameIndex + 1) % numBulletFrames;
				}
			}

			// 移除超时或超出范围的子弹
			if (bullets[i].lifetime <= 0.0f ||
				glm::length(bullets[i].position) > 500.0f) { // 调整子弹最大活动范围
				bullets.erase(bullets.begin() + i);
				i--; // 因为删除了元素，调整索引
			}
		}
	}
	
	// Check player shooting
	void CheckPlayerShooting(vec3 pos, vec3 dir) {
		for (size_t i = 0; i < bullets.size(); i++) {
			// 计算射线与子弹的距离
			vec3 des = (pos.z - bullets[i].position.z) / (-dir.z) * dir + pos;
			float distance = pow(bullets[i].position.x - des.x, 2) + pow(bullets[i].position.y - des.y, 2);
			
			if (distance <= 50) {  // 击中判定范围
				bullets.erase(bullets.begin() + i);
				score++;
				i--;
			}
		}
	}

	// Check if game is over (now based on whether there are too many bullets or player is hit)
	bool IsOver() {
		// Can add game over conditions based on need
		// For example, if too many bullets or player is hit in World class
		return false;
	}

	GLuint GetScore() {
		return score;
	}
	
	// Get current bullet count (for debugging or UI display)
	size_t GetBulletCount() const {
		return bullets.size();
	}
	
	// 修改 Render 方法以渲染正确的动画帧
	void Render(Shader* shaderToUse, GLuint depthMapID = 0) {
		if (bulletFrames.empty() || !camera) return; // 如果没有加载模型帧或相机无效则返回

		// projection 和 view 应该在循环外更新一次，因为它们对于所有子弹都是相同的
		// （已在 BallManager::Update 中更新了 this->projection 和 this->view）

		for (size_t i = 0; i < bullets.size(); ++i) {
			if (bullets[i].currentFrameIndex >= numBulletFrames) continue; // 安全检查

			Model* currentFrameModel = bulletFrames[bullets[i].currentFrameIndex];
			if (!currentFrameModel) continue; // 安全检查

			const auto& subMeshes = currentFrameModel->GetSubMeshes();
			if (subMeshes.empty()) continue;
			const auto& firstSubMesh = subMeshes[0];

			model_matrix_temp = glm::mat4(1.0f);
			model_matrix_temp = glm::translate(model_matrix_temp, bullets[i].position);
			// 如果子弹需要朝向飞行方向，这里还需要计算旋转
			// glm::mat4 rotationMatrix = glm::lookAt(glm::vec3(0.0f), bullets[i].direction, camera->GetUp()); // GetUp()可能不合适，用worldUp
			// model_matrix_temp *= glm::inverse(rotationMatrix); // lookAt 返回的是视图矩阵，需要逆
			// 更简单的方式是用四元数或直接构建旋转矩阵，但如果dot模型本身是对称的，可能不需要旋转。
			model_matrix_temp = glm::scale(model_matrix_temp, glm::vec3(0.5f)); // 子弹大小

			Shader* currentShader = shaderToUse;
			if (currentShader == NULL) {
				currentShader = ballShader;
				currentShader->Bind();
				currentShader->SetMat4("projection", projection); // 使用成员变量
				currentShader->SetMat4("view", view);             // 使用成员变量
				currentShader->SetVec3("viewPos", camera->GetPosition());
				currentShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
			} else {
				currentShader->Bind();
			}

			currentShader->SetMat4("model", model_matrix_temp);

			if (currentShader == ballShader && depthMapID != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, depthMapID);
				// ballShader->SetInt("shadowMap", 0); // 已在LoadModelsAndShader中设置
			}

			glBindVertexArray(firstSubMesh.VAO);
			glDrawElements(GL_TRIANGLES, firstSubMesh.indexCount, GL_UNSIGNED_INT, 0);
		}

		if (shaderToUse == NULL && ballShader) {
			ballShader->Unbind();
		}
		glBindVertexArray(0);
	}

private:
	// 修改 LoadModel 为 LoadModelsAndShader
	void LoadModelsAndShader() {
		// 加载所有子弹动画帧模型
		std::string frameNames[] = {"dot1.obj", "dot2.obj", "dot3.obj", "dot4.obj", "dot5.obj"};
		numBulletFrames = sizeof(frameNames) / sizeof(frameNames[0]);

		for (int i = 0; i < numBulletFrames; ++i) {
			Model* frameModel = new Model("res/model/" + frameNames[i]);
			if (frameModel) { // TODO: 检查模型是否成功加载 (例如，通过GetSubMeshes().empty())
				bulletFrames.push_back(frameModel);
			} else {
				std::cout << "错误: 无法加载子弹模型帧 " << frameNames[i] << std::endl;
				// 处理加载失败的情况，例如将numBulletFrames设置为0或抛出异常
			}
		}
		if(bulletFrames.empty()){ // 如果一个都没加载成功
			numBulletFrames = 0;
			std::cout << "警告: 没有成功加载任何子弹模型帧!" << std::endl;
		}


		ballShader = new Shader("res/shader/ball.vert", "res/shader/ball.frag"); // 假设子弹使用 ball 着色器
		ballShader->Bind();
		ballShader->SetVec3("color", glm::vec3(1.0f, 0.2f, 0.2f));  // 例如，红色子弹
		ballShader->SetInt("shadowMap", 0); // 告诉 ballShader 从纹理单元0读取阴影贴图
		ballShader->SetVec3("lightPos", lightPos);
		// ballShader->SetVec3("viewPos", camera->GetPosition()); // viewPos 在渲染时动态更新
		ballShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
		ballShader->Unbind();
	}
};

#endif // !BALLMANAGER_H
