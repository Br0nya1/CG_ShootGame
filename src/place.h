// place.h
#ifndef PLACE_H
#define PLACE_H

#include <glad/glad.h>
#include "model.h"   // 使用上面更新的 Model 类
#include "texture.h"
#include "shader.h"
#include "camera.h"
#include <assimp/scene.h> // 需要包含 assimp/scene.h 来访问材质名称
#include <assimp/Importer.hpp> // Importer 需要在这里管理，以便安全访问 scene

class Place {
private:
    vec2 windowSize;
    Model* room;

    Texture* greyTexture;     // 用于 Grey_plain.png
    Texture* orangeTexture;   // 用于 Orange_Playground_textures.jpg

    Shader* roomShader;

    // 为了能够根据 materialIndex 获取材质名称，我们需要 Assimp scene 对象
    // 因此，让 Place 类持有 Importer 和 Scene
    Assimp::Importer importer; // Importer 对象，需要在 scene 的整个生命周期内存在
    const aiScene* roomScene;  // 指向加载的房间场景数据

    // 太阳 (保持不变)
    Model* sun;
    vec3 lightPos;
    mat4 lightSpaceMatrix;
    Shader* sunShader;

    Camera* camera;
    mat4 model_matrix; // 原来的 model 变量
    mat4 projection;
    mat4 view;

public:
    Place(vec2 windowSize, Camera* camera) : roomScene(nullptr) { // 初始化 roomScene
        this->windowSize = windowSize;
        this->camera = camera;
        this->lightPos = vec3(0.0, 800.0, 300.0); // 调整后的光源位置
        mat4 lightProjection = ortho(-250.0f, 250.0f, -250.0f, 250.0f, 1.0f, 1500.0f); // 调整后的光源投影
        mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0.0, 1.0, 0.0));
        this->lightSpaceMatrix = lightProjection * lightView;

        LoadModelAndScene(); // 修改：加载模型并获取scene
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
        // importer 会在栈上自动销毁，它加载的 scene 数据也会随之失效
    }

    void Update() {
        this->model_matrix = mat4(1.0);
        this->view = camera->GetViewMatrix();
        this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 2000.0f);
    }

    void RoomRender(Shader* shaderToUse, GLuint depthMapID = 0) {
        Shader* currentShader = (shaderToUse == NULL) ? roomShader : shaderToUse;
        currentShader->Bind();

        // 设置通用矩阵 (对 roomShader 和 simpleDepthShader 都需要)
        currentShader->SetMat4("model", this->model_matrix); // model_matrix 应该在 Update() 中正确设置

        if (shaderToUse == NULL) { // 表示当前是主渲染通道，使用的是 roomShader
            currentShader->SetMat4("view", view);
            currentShader->SetMat4("projection", projection);
            currentShader->SetVec3("viewPos", camera->GetPosition());

            if (depthMapID != 0) {
                glActiveTexture(GL_TEXTURE1); // 激活纹理单元1给阴影贴图
                glBindTexture(GL_TEXTURE_2D, depthMapID);
                currentShader->SetInt("shadowMap", 1); // roomShader 使用 "shadowMap"
            }
        }
        // 注意：对于 simpleDepthShader，它通常只需要 model 和 lightSpaceMatrix。
        // lightSpaceMatrix 应该在 World::RenderDepth() 中为 simpleDepthShader 设置。

        const auto& subMeshes = room->GetSubMeshes();
        for (const auto& submesh : subMeshes) {
            // 只有在主渲染通道 (使用 roomShader) 时，才设置 diffuse 和绑定多纹理
            if (shaderToUse == NULL) { // 或者 currentShader == roomShader
                glActiveTexture(GL_TEXTURE0); // 激活纹理单元0给漫反射贴图s
                // 根据 submesh.materialIndex 获取材质名称并选择纹理
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
                        glBindTexture(GL_TEXTURE_2D, greyTexture->GetId()); // 默认纹理
                    }
                } else {
                    glBindTexture(GL_TEXTURE_2D, greyTexture->GetId()); // 默认纹理
                }
                currentShader->SetInt("diffuse", 0); // <<< 现在这个调用是安全的
            }
            // 对于 simpleDepthShader，我们不绑定漫反射纹理，也不设置 "diffuse" uniform

            glBindVertexArray(submesh.VAO);
            glDrawElements(GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        currentShader->Unbind();
    }


    // 在 place.h 中
    void SunRender() {
        Shader* shader = sunShader; // 使用局部的 shader 指针，很好
        shader->Bind();
        shader->SetMat4("projection", projection);
        shader->SetMat4("view", view);

        mat4 sunModelMatrix = glm::translate(mat4(1.0f), lightPos);
        sunModelMatrix = glm::scale(sunModelMatrix, vec3(20.0f)); // 假设太阳模型需要缩放
        shader->SetMat4("model", sunModelMatrix);

        // 如果 sun 模型也通过支持 SubMesh 的 Model 类加载
        const auto& sunSubMeshes = sun->GetSubMeshes();
        if (!sunSubMeshes.empty()) {
            // 假设太阳模型只有一个子网格，或者我们只渲染第一个
            // 对于太阳这种通常只有一种外观的模型，这通常是安全的
            const auto& firstSubMesh = sunSubMeshes[0];
            glBindVertexArray(firstSubMesh.VAO);
            glDrawElements(GL_TRIANGLES, firstSubMesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        // else {
        //     std::cout << "警告: 太阳模型没有可渲染的子网格!" << std::endl;
        // }

        shader->Unbind();
    }

private:
    void LoadModelAndScene() {
        // 使用 Place 类的 importer 成员来加载场景，这样 roomScene 指针才安全
        roomScene = importer.ReadFile("res/model/room7.obj", aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
        if (!roomScene || roomScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !roomScene->mRootNode) {
            std::cout << "ERROR::ASSIMP::PLACE::" << importer.GetErrorString() << std::endl;
            room = nullptr; // 标记模型加载失败
            return;
        }
        // 创建 Model 对象，但 Model 类的构造函数现在不直接使用 scene 指针了，
        // 它只是处理几何数据。材质名称的解析在 Place::RoomRender 中进行。
        // Model类的构造函数可以简化，或者保持原样，因为我们不再通过它传递scene指针给外部。
        // 这里假设 Model 构造函数会用 importer 重新加载一次，或者 Model 构造函数只负责处理几何。
        // 实际上，更优的做法是 Model 构造函数接受 const aiScene* 和 aiNode* 作为参数，
        // 由 Place 类调用来构建 Model。为了保持 Model 类的独立性，我们让 Model 内部自己加载。
        // 但这意味着 Assimp::Importer importer; 会在 Model 构造函数中再次创建。
        // 为了共享 scene 数据，我们还是让 Model 构造函数不加载文件，而是处理已加载的 scene。
        // 不过，你之前的 Model 类是从路径加载的。
        //
        // 最简单的修改，保持Model从路径加载，但Place也加载一次scene以获取材质信息：
        room = new Model("res/model/room7.obj"); // Model 内部会用自己的Importer加载几何
        // Place 使用自己的Importer加载scene以获取材质

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