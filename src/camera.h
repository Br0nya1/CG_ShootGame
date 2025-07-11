﻿#ifndef CAMERA_H
#define CAMERA_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

const float YAW = -90.0f;			
const float PITCH = 0.0f;			
const float SPEED = 30.0f;			
const float HEIGHT = 10.0f;			
const float SENSITIVITY = 0.1f;     // Mouse movement sensitivity (reduced from 0.1f)
const float ZOOM = 45.0f;			
const float JUMPTIME = 0.1f;		
const float GRAVITY = 9.8f;			
const float JUMPSTRENGTH = 60.0f;	

class Camera {
private:
	GLFWwindow* window;

	vec3 position;				
	vec3 front;					
	vec3 right;					
	vec3 up;					
	vec3 worldUp;				

	float jumpTimer;			
	float gravity;				
	bool isJump;				

	float yaw;					
	float pitch;				

	float movementSpeed;		
	float mouseSensitivity;		
	float zoom;					

	double mouseX;
	double mouseY;
	bool firstMouse;			
public:
	Camera(GLFWwindow* window) {
		this->window = window;

		movementSpeed = SPEED;
		mouseSensitivity = SENSITIVITY;
		zoom = ZOOM;
		firstMouse = true;

		jumpTimer = 0;
		isJump = false;
		gravity = -GRAVITY;

		position = vec3(0.0f, HEIGHT, 70.0f);
		worldUp = vec3(0.0f, 1.0f, 0.0f);
		yaw = YAW;
		pitch = PITCH;

		UpdateCamera();
	}	
	void Update(float deltaTime) {
		MouseMovement();
		KeyboardInput(deltaTime);
	}

	mat4 GetViewMatrix() {
		return lookAt(position, position + front, up);
	}

	vec3 GetPosition() {
		return position;
	}

	vec3 GetFront() {
		return front;
	}

	vec3 GetRight() {
		return right;
	}

	vec3 GetUp() {
		return up;
	}

	float GetZoom() {
		return zoom;
	}
private:
	// �������
	void MouseMovement() {
		double newMouseX, newMouseY;

		glfwGetCursorPos(window, &newMouseX, &newMouseY);
		if (firstMouse) {
			mouseX = newMouseX;
			mouseY = newMouseY;
			firstMouse = false;
		}

		yaw += ((newMouseX - mouseX) * mouseSensitivity);
		pitch += ((mouseY - newMouseY) * mouseSensitivity);

		mouseX = newMouseX;
		mouseY = newMouseY;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		// ����Front��Right��Up
		UpdateCamera();
	}
	// ��������
	void KeyboardInput(float deltaTime) {
		float velocity = movementSpeed * deltaTime;
		vec3 forward = normalize(cross(worldUp, right));
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			position += forward * velocity;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			position -= forward * velocity;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			position -= right * velocity;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			position += right * velocity;

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJump) {
			jumpTimer = JUMPTIME;
			isJump = true;
		}
		if (jumpTimer > 0) {
			gravity += (JUMPSTRENGTH * (jumpTimer / JUMPTIME)) * deltaTime;
			jumpTimer -= deltaTime;
		}
		gravity -= GRAVITY * deltaTime;
		position.y += gravity * deltaTime * 10;

		if (position.y < HEIGHT) {
			position.y = HEIGHT;
			gravity = 0;
			isJump = false;
		}

		CheckCollision();
	}
	// ��ײ���
	void CheckCollision() {
		if (position.x > 180.0f)
			position.x = 180.0f;
		if (position.x < -180.0f)
			position.x = -90.0f;
		if (position.z > 180.0f)
			position.z = 180.0f;
		if (position.z < -180.0f)
			position.z = -180.0f;
	}
	// ��������ͷ���������
	void UpdateCamera() {
		vec3 front;
		front.x = cos(radians(yaw)) * cos(radians(pitch));
		front.y = sin(radians(pitch));
		front.z = sin(radians(yaw)) * cos(radians(pitch));
		this->front = normalize(front);

		this->right = normalize(cross(this->front, this->worldUp));
		this->up = normalize(cross(this->right, this->front));
	}
};

#endif // !CAMERA_H