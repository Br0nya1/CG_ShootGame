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
	
	Bullet(vec3 pos, vec3 dir, float spd = 0.3f) 
		: position(pos), direction(normalize(dir)), speed(spd), lifetime(15.0f) {}
};

// Enemy shooter structure (independent firing timer for each enemy)
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
	GLuint score;						// Score
	GLuint gameModel;					// Game mode
	vec3 lightPos;						// Light position
	mat4 lightSpaceMatrix;				// Shadow map transformation matrix

	// Enemy bullet system
	vector<Bullet> bullets;				// All bullets
	vector<EnemyShooter> enemyShooters;	// Enemy shooters

	Camera* camera;
	// Model transformation matrices
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

	// Set game mode
	void SetGameModel(GLuint num) {
		gameModel = num;
	}
	
	// Update bullets and shooting logic
	void Update(vec3 pos, vec3 dir, bool isShoot, float deltaTime) {
		this->view = camera->GetViewMatrix();
		this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f);

		// Update enemy shooting timers and fire bullets
		UpdateEnemyShooting(deltaTime);
		
		// Update bullet positions
		UpdateBullets(deltaTime);

		// Player shooting detection
		if (isShoot) {
			CheckPlayerShooting(pos, dir);
		}
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
	
	// Update bullet positions
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
	
	// Render all currently active bullets
	void Render(Shader* shaderToUse, GLuint depthMapID = 0) { 
		if (!ball || !camera) return; // Add camera check

		const auto& ballSubMeshes = ball->GetSubMeshes(); // Get bullet model sub-mesh information
		if (ballSubMeshes.empty()) return; 

		const auto& firstBallSubMesh = ballSubMeshes[0]; // Assume bullet model has only one sub-mesh

		// Loop through this->bullets vector, not this->position
		for (size_t i = 0; i < bullets.size(); ++i) { // Use size_t
			model = glm::mat4(1.0f);
			model = glm::translate(model, bullets[i].position); // Use bullets[i].position
			// model = glm::scale(model, glm::vec3(5.0f)); // Bullet size, can be adjusted, originally vec3(5)
			model = glm::scale(model, glm::vec3(0.5f)); // Make bullet smaller, e.g., 0.5 times size

			Shader* currentShader = shaderToUse; 
			if (currentShader == NULL) {
				currentShader = ballShader; 
				currentShader->Bind();
				currentShader->SetMat4("projection", projection); // projection and view should be updated once outside the loop
				currentShader->SetMat4("view", view);
				currentShader->SetVec3("viewPos", camera->GetPosition()); // Real-time update viewPos
				// lightPos and lightSpaceMatrix are static, set in LoadModel
			}
			else {
				currentShader->Bind(); 
			}

			currentShader->SetMat4("model", model);

			// If ballShader needs to cast shadows (i.e., write to depth map, it itself needs to be rendered)
			// Or ballShader needs to receive shadows (i.e., in main rendering pass, it needs to sample shadowMap)
			if (currentShader == ballShader && depthMapID != 0) {
				glActiveTexture(GL_TEXTURE0); // ballShader's "shadowMap" uniform set to unit0
				glBindTexture(GL_TEXTURE_2D, depthMapID);
				// ballShader->SetInt("shadowMap", 0); // Already set in LoadModel, no need to repeat
			}
			// Note: ballShader uses "color" uniform, not diffuse texture.
			// If it needs a diffuse texture, you need to handle it like other models.

			glBindVertexArray(firstBallSubMesh.VAO); // Use sub-mesh's VAO
			glDrawElements(GL_TRIANGLES, firstBallSubMesh.indexCount, GL_UNSIGNED_INT, 0); // Use sub-mesh's index count
		}

		// Unbind after loop
		if (shaderToUse == NULL && ballShader) { // If using your own ballShader
			ballShader->Unbind();
		}
		// If external shader is passed in (shaderToUse != NULL), usually caller (e.g., World::RenderDepth) is responsible for unbinding its shader
		// Or according to convention, Render function always unbinds it bound shader
		// else if (shaderToUse != NULL) {
		//     shaderToUse->Unbind(); // Optional unbind
		// }
		glBindVertexArray(0); // Finally unbind VAO
	}

private:
	void LoadModel() {
		ball = new Model("res/model/dot.obj");
		ballShader = new Shader("res/shader/ball.vert", "res/shader/ball.frag");
		ballShader->Bind();
		ballShader->SetVec3("color", vec3(1.0, 0.2, 0.2));  // Red bullets
		ballShader->SetInt("shadowMap", 0);
		ballShader->SetVec3("lightPos", lightPos);
		ballShader->SetVec3("viewPos", camera->GetPosition());
		ballShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
		ballShader->Unbind();
	}
};

#endif // !BALLMANAGER_H
