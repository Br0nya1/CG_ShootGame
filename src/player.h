#ifndef PLAYER_H
#define PLAYER_H

#include <glad/glad.h>
#include "texture.h"
#include "shader.h"
#include "model.h"
#include "camera.h"

class Player {
private:
	vec2 windowSize;					// Window size
	// Gun
	Model* gun;
	vec3 gunPos;						// Gun position
	Shader* gunShader;
	mat4 gunModel;						// Gun model transformation matrix
	Texture* diffuseMap;				// Diffuse texture map
	Texture* specularMap;				// Specular reflection map
	float gunRecoil;					// Gun recoil
	// Crosshair
	Model* dot;							
	Shader* dotShader;
	mat4 dotModel;						// Crosshair model transformation matrix
	// Camera
	Camera* camera;
	// Transformation matrices
	mat4 projection;
	mat4 view;
public:
	Player(vec2 windowSize, Camera* camera) {
		this->windowSize = windowSize;
		this->camera = camera;
		this->gunRecoil = 10.0f;
		this->dotModel = mat4(1.0);
		this->gunModel = mat4(1.0);

		LoadGun();
		LoadTexture();
		LoadShader();
	}
	// Update transformation matrices based on camera position and orientation
	void Update(float deltaTime,  bool isShoot) {
		if (isShoot)
			gunRecoil = 10.0f;
		else
			gunRecoil = 0.0f;

		view = camera->GetViewMatrix();
		projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f);
		dotModel = mat4(1.0);
		dotModel[3] = vec4(camera->GetPosition(), 1.0);
		dotModel = translate(dotModel, camera->GetFront());
		dotModel = scale(dotModel, vec3(0.005));
		vec3 gunPos = (camera->GetFront() * 0.25f) + (camera->GetRight() * 0.2f) + (camera->GetUp() * -0.125f) + camera->GetPosition();
		gunModel = mat4(1.0);
		gunModel[0] = vec4(camera->GetRight(), 0.0);
		gunModel[1] = vec4(camera->GetUp(), 0.0);
		gunModel[2] = vec4(-camera->GetFront(), 0.0);
		gunModel[3] = vec4(gunPos, 1.0);
		gunModel = rotate(gunModel, radians(gunRecoil), vec3(1.0, 0.0, 0.0));
		gunModel = scale(gunModel, vec3(0.225));
		gunModel = translate(gunModel, vec3(-0.225, 0.0, -0.225));
		gunModel = rotate(gunModel, radians(-170.0f), vec3(0.0, 1.0, 0.0));
	}
	// Render player components
	void Render() {
		// --- Render crosshair (dot) ---
		dotShader->Bind();
		dotShader->SetMat4("projection", projection);
		dotShader->SetMat4("view", view);
		dotShader->SetMat4("model", dotModel);

		// Modified: Use GetSubMeshes() to render dot model
		if (dot) { // Ensure dot model is loaded
			const auto& dotSubMeshes = dot->GetSubMeshes();
			if (!dotSubMeshes.empty()) {
				const auto& firstSubMesh = dotSubMeshes[0]; // Render the first dot submesh
				glBindVertexArray(firstSubMesh.VAO);
				glDrawElements(GL_TRIANGLES, firstSubMesh.indexCount, GL_UNSIGNED_INT, 0);
			}
		}
		// Modified end

		// --- Render gun (gun) ---
		gunShader->Bind();
		gunShader->SetMat4("projection", projection);
		gunShader->SetMat4("view", view);
		gunShader->SetMat4("model", gunModel);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap->GetId());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap->GetId());

		// Modified: Use GetSubMeshes() to render gun model
		if (gun) { // Ensure gun model is loaded
			const auto& gunSubMeshes = gun->GetSubMeshes();
			if (!gunSubMeshes.empty()) {
				const auto& firstSubMesh = gunSubMeshes[0]; // Render the first gun submesh
				glBindVertexArray(firstSubMesh.VAO);
				glDrawElements(GL_TRIANGLES, firstSubMesh.indexCount, GL_UNSIGNED_INT, 0);
			}
		}
		// Modified end

		glBindVertexArray(0); // Unbind VAO to prevent further rendering
		gunShader->Unbind(); // Unbind shader to prevent further rendering
		// dotShader should also be unbound, but it's not necessary to check if it's used by gunShader
		// If dotShader and gunShader are not sharing, it's OK to leave dotShader unbound when rendering gun
	}
private:
	// Load gun model
	void LoadGun() {
		gun = new Model("res/model/gun.obj");
		dot = new Model("res/model/dot.obj");
	}
	// Load textures
	void LoadTexture() {
		diffuseMap = new Texture("res/texture/diffuse.png");
		specularMap = new Texture("res/texture/specular.jpg");
	}
	// Load shaders
	void LoadShader() {
		gunShader = new Shader("res/shader/gun.vert", "res/shader/gun.frag");
		gunShader->Bind();
		gunShader->SetInt("material.diffuse", 0);
		gunShader->SetInt("material.specular", 1);
		gunShader->SetFloat("material.shininess", 64.0);
		gunShader->SetVec3("light.position", vec3(0.0, 400.0, 150.0));
		gunShader->SetVec3("light.ambient", vec3(0.2));
		gunShader->SetVec3("light.diffuse", vec3(0.65));
		gunShader->SetVec3("light.specular", vec3(1.0));
		gunShader->SetVec3("viewPos", camera->GetPosition());
		gunShader->Unbind();

		dotShader = new Shader("res/shader/ball.vert", "res/shader/ball.frag");
		dotShader->Bind();
		dotShader->SetVec3("color", vec3(0.0, 1.0, 0.0));
		dotShader->Unbind();
	}
};

#endif // !PLAYER_H
