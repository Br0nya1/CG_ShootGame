// place.h
#ifndef PLACE_H
#define PLACE_H

#include <glad/glad.h>
#include "model.h"   // Use the updated Model class above
#include "texture.h"
#include "shader.h"
#include "camera.h"
#include <assimp/scene.h> // Need to include assimp/scene.h to access material names
#include <assimp/Importer.hpp> // Importer needs to be managed here for safe scene access

class Place {
private:
    vec2 windowSize;
    Model* room;

    Texture* greyTexture;     // For Grey_plain.png
    Texture* orangeTexture;   // For Orange_Playground_textures.jpg

    Shader* roomShader;

    // In order to get material names based on materialIndex, we need Assimp scene object
    // Therefore, let Place class hold Importer and Scene
    Assimp::Importer importer; // Importer object, needs to exist for the entire life of the scene
    const aiScene* roomScene;  // Pointer to loaded room scene data

    // Sun (keep unchanged)
    Model* sun;
    vec3 lightPos;
    mat4 lightSpaceMatrix;
    Shader* sunShader;

    Camera* camera;
    mat4 model_matrix; // Original model variable
    mat4 projection;
    mat4 view;

public:
    Place(vec2 windowSize, Camera* camera) : roomScene(nullptr) { // Initialize roomScene
        this->windowSize = windowSize;
        this->camera = camera;
        this->lightPos = vec3(0.0, 800.0, 300.0); // Adjusted light position
        mat4 lightProjection = ortho(-250.0f, 250.0f, -250.0f, 250.0f, 1.0f, 1500.0f); // Adjusted light projection
        mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0.0, 1.0, 0.0));
        this->lightSpaceMatrix = lightProjection * lightView;

        LoadModelAndScene(); // Modified: Load model and get scene
        LoadTexture();
        LoadShader();
    }

    ~Place() {
        delete room;
        delete greyTexture;
        delete orangeTexture;
        delete roomShader;
        delete sun;
        delete sunShader;
        // importer will be destroyed on stack, scene data will also be invalidated
    }

    void Update() {
        this->model_matrix = glm::mat4(1.0f);
        // this->model_matrix = glm::scale(this->model_matrix, glm::vec3(1.0f, 0.5f, 1.0f));  // 修改地图高度为原来一半

        this->view = camera->GetViewMatrix();
        this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 2000.0f);
    }

    void RoomRender(Shader* shaderToUse, GLuint depthMapID = 0) {
        Shader* currentShader = (shaderToUse == NULL) ? roomShader : shaderToUse;
        currentShader->Bind();

        // Set common matrices (needed for both roomShader and simpleDepthShader)
        currentShader->SetMat4("model", this->model_matrix); // model_matrix should be correctly set in Update()

        if (shaderToUse == NULL) { // Indicates current is main rendering pass, using roomShader
            currentShader->SetMat4("view", view);
            currentShader->SetMat4("projection", projection);
            currentShader->SetVec3("viewPos", camera->GetPosition());
            currentShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);

            if (depthMapID != 0) {
                glActiveTexture(GL_TEXTURE1); // Activate texture unit 1 for shadow map
                glBindTexture(GL_TEXTURE_2D, depthMapID);
                currentShader->SetInt("shadowMap", 1); // roomShader uses "shadowMap"
            }
        }
        // Note: For simpleDepthShader, it usually only needs model and lightSpaceMatrix.
        // lightSpaceMatrix should be set for simpleDepthShader in World::RenderDepth().

        const auto& subMeshes = room->GetSubMeshes();
        for (const auto& submesh : subMeshes) {
            // Only set diffuse and bind multi-textures in main rendering pass (using roomShader)
            if (shaderToUse == NULL) { // Or currentShader == roomShader
                glActiveTexture(GL_TEXTURE0); // Activate texture unit 0 for diffuse map
                // Get material name based on submesh.materialIndex and select texture
                if (roomScene && submesh.materialIndex < roomScene->mNumMaterials) {
                    aiMaterial* material = roomScene->mMaterials[submesh.materialIndex];
                    aiString materialName;
                    material->Get(AI_MATKEY_NAME, materialName);
                    std::string nameStr = materialName.C_Str();

                    if (nameStr == "Grey_plain") {
                        glBindTexture(GL_TEXTURE_2D, greyTexture->GetId());
                    } else if (nameStr == "Orange_Playground_textures" || nameStr == "Orange_plain") {
                        glBindTexture(GL_TEXTURE_2D, orangeTexture->GetId());
                    } else {
                        glBindTexture(GL_TEXTURE_2D, greyTexture->GetId()); // Default texture
                    }
                } else {
                    glBindTexture(GL_TEXTURE_2D, greyTexture->GetId()); // Default texture
                }
                currentShader->SetInt("diffuse", 0); // <<< Now this call is safe
            }
            // For simpleDepthShader, we don't bind diffuse texture, nor set "diffuse" uniform

            glBindVertexArray(submesh.VAO);
            glDrawElements(GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        currentShader->Unbind();
    }


    // In place.h
    void SunRender() {
        Shader* shader = sunShader; // Use local shader pointer, good
        shader->Bind();
        shader->SetMat4("projection", projection);
        shader->SetMat4("view", view);

        mat4 sunModelMatrix = glm::translate(mat4(1.0f), lightPos);
        sunModelMatrix = glm::scale(sunModelMatrix, vec3(20.0f)); // Assume sun model needs scaling
        shader->SetMat4("model", sunModelMatrix);

        // If sun model also loaded through Model class supporting SubMesh
        const auto& sunSubMeshes = sun->GetSubMeshes();
        if (!sunSubMeshes.empty()) {
            // Assume sun model has only one submesh, or we only render first one
            // For sun model which usually has only one appearance, this is usually safe
            const auto& firstSubMesh = sunSubMeshes[0];
            glBindVertexArray(firstSubMesh.VAO);
            glDrawElements(GL_TRIANGLES, firstSubMesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        // else {
        //     std::cout << "Warning: Sun model has no renderable submesh!" << std::endl;
        // }

        shader->Unbind();
    }

private:
    void LoadModelAndScene() {
        // Use Place class importer member to load scene, so roomScene pointer is safe
        roomScene = importer.ReadFile("res/model/room7.obj", aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
        if (!roomScene || roomScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !roomScene->mRootNode) {
            std::cout << "ERROR::ASSIMP::PLACE::" << importer.GetErrorString() << std::endl;
            room = nullptr; // Mark model load failed
            return;
        }
        // Create Model object, but Model class constructor now doesn't directly use scene pointer,
        // It only processes geometry data. Material name parsing is done in Place::RoomRender.
        // Model class constructor can be simplified, or kept as it is, because we don't pass scene pointer to external.
        // Here assume Model constructor reloads once using importer, or Model constructor only handles geometry.
        // In fact, better approach is Model constructor accepts const aiScene* and aiNode* as parameters,
        // Called by Place class to build Model. To keep Model class independent, we let Model load itself.
        // But this means Assimp::Importer importer; will be created again in Model constructor.
        // To share scene data, we still let Model constructor not load file, but handle loaded scene.
        // But you previous Model class was from path loading.
        //
        // Simplest modification, keepModel from path loading, butPlace also load once scene to get material info:
        room = new Model("res/model/room7.obj"); // Model internal will use its own Importer to load geometry
        // Place uses its own Importer to load scene to get material

        sun = new Model("res/model/sun.obj");
    }

    void LoadTexture() {
        greyTexture = new Texture("res/texture/Grey_plain.png");
        orangeTexture = new Texture("res/texture/Orange_Playground_textures.png");
    }

    void LoadShader() {
        roomShader = new Shader("res/shader/room.vert", "res/shader/room.frag");
        roomShader->Bind();
        roomShader->SetInt("diffuse", 0);
        roomShader->SetInt("shadowMap", 1);
        roomShader->SetVec3("lightPos", lightPos);
        roomShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        roomShader->Unbind();

        sunShader = new Shader("res/shader/sun.vert", "res/shader/sun.frag");
        sunShader->Bind();
        sunShader->Unbind();
    }
};
#endif // !PLACE_H