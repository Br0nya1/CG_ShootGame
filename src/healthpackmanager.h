#ifndef HEALTHPACKMANAGER_H
#define HEALTHPACKMANAGER_H

#include <irrklang/irrKlang.h>
using namespace irrklang;
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
ISoundEngine* xuebao= createIrrKlangDevice();

// Health pack structure
struct HealthPack {
    vec3 position;      // Health pack position
    float rotationY;    // Y-axis rotation for visual effect
    bool isActive;      // Whether the health pack is active
    
    HealthPack(vec3 pos) 
        : position(pos), rotationY(0.0f), isActive(true) {}
};

class HealthPackManager {
private:
    vec2 windowSize;
    
    Model* healthPack;
    Shader* healthPackShader;
    vector<HealthPack> healthPacks;     // All health packs
    
    float spawnTimer;                   // Spawn timer
    float spawnInterval;                // Spawn interval (seconds)
    GLuint maxHealthPacks;              // Maximum health packs on field
    float pickupRadius;                 // Pickup radius
    
    vec3 lightPos;                      // Light position
    mat4 lightSpaceMatrix;              // Light space matrix
    
    Camera* camera;
    // Model transformation matrices
    mat4 model;
    mat4 projection;
    mat4 view;
    
public:
    HealthPackManager(vec2 windowSize, Camera* camera) {
        this->windowSize = windowSize;
        this->camera = camera;
        
        spawnTimer = 0.0f;
        spawnInterval = 3.0f;           // Spawn one health pack every 3 seconds (for debugging)
        maxHealthPacks = 10;            // Maximum 10 health packs on field (for debugging)
        pickupRadius = 10.0f;           // Larger pickup radius for easier collection
        
        this->lightPos = vec3(0.0, 400.0, 150.0);
        mat4 lightProjection = ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 500.0f);
        mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0.0, 1.0, 0.0));
        this->lightSpaceMatrix = lightProjection * lightView;
        
        LoadModel();
        
        // Spawn initial health packs for debugging
        for (int i = 0; i < 3; i++) {
            SpawnHealthPack();
        }
    }
    
    // Update health pack spawning and rotation
    void Update(float deltaTime) {
        this->view = camera->GetViewMatrix();
        this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f);
        
        // Update spawn timer
        UpdateSpawning(deltaTime);
        
        // Update health pack rotation for visual effect
        for (auto& pack : healthPacks) {
            if (pack.isActive) {
                pack.rotationY += 90.0f * deltaTime; // Rotate 90 degrees per second
                if (pack.rotationY >= 360.0f) {
                    pack.rotationY -= 360.0f;
                }
            }
        }
    }
    
    // Check if player can pickup health pack (E key pressed)
    bool TryPickupHealthPack(vec3 playerPos) {
        float closestDistance = pickupRadius + 1.0f; // Initialize to beyond pickup range
        int closestIndex = -1;
        
        // Find closest health pack within pickup radius
        for (size_t i = 0; i < healthPacks.size(); i++) {
            if (healthPacks[i].isActive) {
                float distance = length(healthPacks[i].position - playerPos);
                if (distance <= pickupRadius && distance < closestDistance) {
                    closestDistance = distance;
                    closestIndex = i;
                }
            }
        }
        
        // If found a health pack within range, pick it up
        if (closestIndex != -1) {
            healthPacks[closestIndex].isActive = false;
            xuebao->play2D("res/audio/xuebao.mp3",GL_FALSE);
            cout << "Health pack picked up! Distance: " << closestDistance << endl;
            return true;
        }
        
        return false;
    }
    
    // Render all active health packs
    void Render(Shader* shaderToUse, GLuint depthMapID = 0) {
        if (!healthPack || !camera) return;
        
        const auto& packSubMeshes = healthPack->GetSubMeshes();
        if (packSubMeshes.empty()) return;
        
        for (const auto& pack : healthPacks) {
            if (!pack.isActive) continue; // Skip inactive health packs
            
            model = glm::mat4(1.0f);
            model = glm::translate(model, pack.position);
            // Try multiple rotations to orient the pentagram correctly
            model = glm::rotate(model, glm::radians(pack.rotationY), glm::vec3(0.0f, 1.0f, 0.0f)); // Y rotation for spinning
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // X rotation to face up
            model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Z rotation if needed
            model = glm::scale(model, glm::vec3(0.6f, 0.6f, 0.6f)); // Larger scale for better visibility
            
            Shader* currentShader = shaderToUse;
            if (currentShader == NULL) {
                currentShader = healthPackShader;
                currentShader->Bind();
                currentShader->SetMat4("projection", projection);
                currentShader->SetMat4("view", view);
                currentShader->SetVec3("viewPos", camera->GetPosition());
            }
            else {
                currentShader->Bind();
            }
            
            currentShader->SetMat4("model", model);
            
            // Handle texture binding like enemies do
            if (currentShader == healthPackShader && depthMapID != 0) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, depthMapID);
                currentShader->SetInt("shadowMap_tex", 1);
            }
            
            // Render ALL submeshes to make sure we get the complete model
            for (const auto& subMesh : packSubMeshes) {
                glBindVertexArray(subMesh.VAO);
                glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, 0);
            }
        }
        
        // Unbind after rendering
        if (shaderToUse == NULL && healthPackShader) {
            healthPackShader->Unbind();
        }
        glBindVertexArray(0);
    }
    
    // Get number of active health packs
    size_t GetActiveHealthPackCount() const {
        size_t count = 0;
        for (const auto& pack : healthPacks) {
            if (pack.isActive) count++;
        }
        return count;
    }
    
private:
    void LoadModel() {
        // Switch back to pentagram model with proper transformations
        healthPack = new Model("res/model/WS_Pentagram_obj.obj");
        // Use enemy shader for proper material and lighting
        healthPackShader = new Shader("res/shader/enemy.vert", "res/shader/enemy.frag");
        healthPackShader->Bind();
        // Set up material properties like enemies do
        healthPackShader->SetInt("material.diffuse", 0);
        healthPackShader->SetInt("material.specular", 1);
        healthPackShader->SetFloat("material.shininess", 64.0);
        healthPackShader->SetVec3("light.position", lightPos);
        healthPackShader->SetVec3("light.ambient", vec3(0.2));
        healthPackShader->SetVec3("light.diffuse", vec3(0.65));
        healthPackShader->SetVec3("light.specular", vec3(1.0));
        healthPackShader->SetVec3("viewPos", camera->GetPosition());
        healthPackShader->SetInt("shadowMap_tex", 1);
        healthPackShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        healthPackShader->Unbind();
    }
    
    // Update health pack spawning
    void UpdateSpawning(float deltaTime) {
        spawnTimer += deltaTime;
        
        if (spawnTimer >= spawnInterval && GetActiveHealthPackCount() < maxHealthPacks) {
            SpawnHealthPack();
            spawnTimer = 0.0f;
        }
    }
    
    // Spawn a new health pack at random position
    void SpawnHealthPack() {
        int tryCount = 0;
        while (tryCount < 100) { // Prevent infinite loop
            float x = (rand() % 40) - 20; // Random X between -20 and 20 (closer to player)
            float z = (rand() % 40) - 20; // Random Z between -20 and 20 (closer to player)
            float y = 8.0f; // Higher position for better visibility
            vec3 pos = vec3(x, y, z);
            
            if (CheckValidPosition(pos)) {
                healthPacks.push_back(HealthPack(pos));
                cout << "Health pack spawned at position: (" << x << ", " << y << ", " << z << ")" << endl;
                
                // Debug: Print submesh count
                if (healthPack) {
                    cout << "Health pack model has " << healthPack->GetSubMeshes().size() << " submeshes" << endl;
                }
                break;
            }
            tryCount++;
        }
    }
    
    // Check if position is valid (not too close to other health packs)
    bool CheckValidPosition(vec3 pos) {
        for (const auto& pack : healthPacks) {
            if (pack.isActive) {
                float distance = length(pack.position - pos);
                if (distance < 15.0f) { // Minimum distance between health packs
                    return false;
                }
            }
        }
        return true;
    }
};

#endif // !HEALTHPACKMANAGER_H 