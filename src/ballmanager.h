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

// 子弹结构体
struct Bullet {
	vec3 position;		// 子弹位置
	vec3 direction;		// 子弹飞行方向
	float speed;		// 子弹速度
	float lifetime;		// 子弹生存时间
	
	Bullet(vec3 pos, vec3 dir, float spd = 0.3f) 
		: position(pos), direction(normalize(dir)), speed(spd), lifetime(15.0f) {}
};

// 敌人射击器结构体（每个敌人独立的射击计时器）
struct EnemyShooter {
	vec3 enemyPosition;
	float fireTimer;
	float fireRate;
	
	EnemyShooter(vec3 pos, float rate = 2.0f) 
		: enemyPosition(pos), fireTimer(0.0f), fireRate(rate) {}
};

class BallManager {
private:
	vec2 windowSize;

	Model* ball;
	Shader* ballShader;
	GLuint score;						// 得分
	GLuint gameModel;					// 游戏模式
	vec3 lightPos;						// 光源位置
	mat4 lightSpaceMatrix;				// 阴影贴图转换为以光源为中心的坐标

	// 敌人子弹系统
	vector<Bullet> bullets;				// 所有子弹
	vector<EnemyShooter> enemyShooters;	// 每个敌人的射击器

	Camera* camera;
	// 模型变换矩阵
	mat4 model;
	mat4 projection;
	mat4 view;
public:
	BallManager(vec2 windowSize, Camera* camera) {
		this->windowSize = windowSize;
		this->camera = camera;
		score = 0;
		this->lightPos = vec3(0.0, 400.0, 150.0);
		mat4 lightProjection = ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 500.0f);
		mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0.0, 1.0, 0.0));
		this->lightSpaceMatrix = lightProjection * lightView;
		
		LoadModel();
	}
	
	// 更新敌人射击器位置
	void UpdateEnemyPositions(const vector<vec3>& enemyPositions) {
		// 如果敌人数量发生变化，才重新创建射击器
		if (enemyShooters.size() != enemyPositions.size()) {
			enemyShooters.clear();
			
			// 为每个敌人创建射击器
			for (const auto& pos : enemyPositions) {
				// 每个敌人有稍微不同的射击频率，增加随机性
				float randomFireRate = 1.5f + (rand() % 200) / 100.0f; // 1.5-3.5秒
				enemyShooters.push_back(EnemyShooter(pos, randomFireRate));
			}
		} else {
			// 只更新现有射击器的位置
			for (size_t i = 0; i < enemyPositions.size() && i < enemyShooters.size(); i++) {
				enemyShooters[i].enemyPosition = enemyPositions[i];
			}
		}
	}
	
	// 添加单个子弹
	void AddBullet(vec3 enemyPos, vec3 playerPos) {
		vec3 direction = playerPos - enemyPos;
		// 稍微提高子弹起始位置，避免与地面碰撞
		vec3 bulletStartPos = enemyPos + vec3(0.0f, 2.0f, 0.0f);
		bullets.push_back(Bullet(bulletStartPos, direction));
	}
	
	// 检查子弹是否击中玩家
	bool CheckBulletHitPlayer(float hitRadius = 5.0f) {
		vec3 playerPos = camera->GetPosition();
		
		for (size_t i = 0; i < bullets.size(); i++) {
			float distance = length(bullets[i].position - playerPos);
			if (distance <= hitRadius) {
				cout << "Player hit! Distance: " << distance << ", Radius: " << hitRadius << endl;
				bullets.erase(bullets.begin() + i);
				return true;  // 玩家被击中
			}
		}
		return false;
	}

	// 设置游戏模式
	void SetGameModel(GLuint num) {
		gameModel = num;
	}
	
	// 更新子弹和射击逻辑
	void Update(vec3 pos, vec3 dir, bool isShoot, float deltaTime) {
		this->view = camera->GetViewMatrix();
		this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f);

		// 更新敌人射击计时器并发射子弹
		UpdateEnemyShooting(deltaTime);
		
		// 更新子弹位置
		UpdateBullets(deltaTime);

		// 玩家射击检测
		if (isShoot) {
			CheckPlayerShooting(pos, dir);
		}
	}
	
	// 更新敌人射击逻辑
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
	
	// 更新子弹位置
	void UpdateBullets(float deltaTime) {
		for (size_t i = 0; i < bullets.size(); i++) {
			// 移动子弹
			bullets[i].position += bullets[i].direction * bullets[i].speed;
			bullets[i].lifetime -= deltaTime;
			
			// 移除超时或超出范围的子弹
			if (bullets[i].lifetime <= 0.0f || 
				length(bullets[i].position) > 200.0f) {
				bullets.erase(bullets.begin() + i);
				i--;
			}
		}
	}
	
	// 检查玩家射击是否击中子弹
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

	// 判断游戏是否结束（现在基于是否有太多子弹或玩家被击中）
	bool IsOver() {
		// 可以根据需要添加游戏结束条件
		// 比如子弹数量过多，或在World类中处理玩家生命值
		return false;
	}

	GLuint GetScore() {
		return score;
	}
	
	// 获取当前子弹数量（用于调试或UI显示）
	size_t GetBulletCount() const {
		return bullets.size();
	}
	
	// 渲染所有当前激活的子弹
	void Render(Shader* shaderToUse, GLuint depthMapID = 0) { 
		if (!ball || !camera) return; // 增加 camera 检查

		const auto& ballSubMeshes = ball->GetSubMeshes(); // 获取子弹模型的子网格信息
		if (ballSubMeshes.empty()) return; 

		const auto& firstBallSubMesh = ballSubMeshes[0]; // 假设子弹模型只有一个子网格

		// 遍历的是 this->bullets 向量，而不是 this->position
		for (size_t i = 0; i < bullets.size(); ++i) { // 使用 size_t
			model = glm::mat4(1.0f);
			model = glm::translate(model, bullets[i].position); // 使用 bullets[i].position
			// model = glm::scale(model, glm::vec3(5.0f)); // 子弹大小，可以调整，原先是vec3(5)
			model = glm::scale(model, glm::vec3(0.5f)); // 让子弹小一点，比如0.5的尺寸

			Shader* currentShader = shaderToUse; 
			if (currentShader == NULL) {
				currentShader = ballShader; 
				currentShader->Bind();
				currentShader->SetMat4("projection", projection); // projection 和 view 应在循环外更新一次
				currentShader->SetMat4("view", view);
				currentShader->SetVec3("viewPos", camera->GetPosition()); // 实时更新 viewPos
				// lightPos 和 lightSpaceMatrix 是静态的，已在LoadModel中设置
			}
			else {
				currentShader->Bind(); 
			}

			currentShader->SetMat4("model", model);

			// 如果 ballShader 也需要进行阴影投射（即写入深度图时，它本身也需要被渲染）
			// 或者 ballShader 需要接收阴影（即在主渲染通道，它需要采样 shadowMap）
			if (currentShader == ballShader && depthMapID != 0) {
				glActiveTexture(GL_TEXTURE0); // ballShader 的 "shadowMap" uniform 设置为单元0
				glBindTexture(GL_TEXTURE_2D, depthMapID);
				// ballShader->SetInt("shadowMap", 0); // 已在LoadModel中设置，此处无需重复
			}
			// 注意：ballShader 使用的是 "color" uniform，而不是漫反射纹理。
			// 如果它也需要一个漫反射纹理，你需要像其他模型一样处理。

			glBindVertexArray(firstBallSubMesh.VAO); // 使用子网格的 VAO
			glDrawElements(GL_TRIANGLES, firstBallSubMesh.indexCount, GL_UNSIGNED_INT, 0); // 使用子网格的索引数量
		}

		// 在循环结束后解绑
		if (shaderToUse == NULL && ballShader) { // 如果用的是自己的ballShader
			ballShader->Unbind();
		}
		// 如果是外部传入的shader (shaderToUse != NULL)，通常由调用者（如World::RenderDepth）负责解绑其shader
		// 或者按照约定，Render函数总是解绑它绑定的shader
		// else if (shaderToUse != NULL) {
		//     shaderToUse->Unbind(); // 可选的解绑
		// }
		glBindVertexArray(0); // 最后解绑VAO
	}

private:
	void LoadModel() {
		ball = new Model("res/model/dot.obj");
		ballShader = new Shader("res/shader/ball.vert", "res/shader/ball.frag");
		ballShader->Bind();
		ballShader->SetVec3("color", vec3(1.0, 0.2, 0.2));  // 红色子弹
		ballShader->SetInt("shadowMap", 0);
		ballShader->SetVec3("lightPos", lightPos);
		ballShader->SetVec3("viewPos", camera->GetPosition());
		ballShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
		ballShader->Unbind();
	}
};

#endif // !BALLMANAGER_H
