#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <vector>
using namespace std;
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
using namespace Assimp;

struct Triangle {
    glm::vec3 v0, v1, v2; 
    glm::vec3 normal;     

    Triangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2)
        : v0(p0), v1(p1), v2(p2) {
        
        
        normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    }
};

struct SubMesh {
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    unsigned int indexCount;
    unsigned int materialIndex; 

    std::vector<glm::vec3> vertices; 
    std::vector<GLuint> indices;    
    SubMesh() : VAO(0), VBO(0), EBO(0), indexCount(0), materialIndex(0) {}
};

class Model {
private:
    std::vector<SubMesh> subMeshes;
public:
    Model(const std::string& path, const aiScene** outScene = nullptr) {
        LoadModel(path);
    }

    const std::vector<SubMesh>& GetSubMeshes() const {
        return subMeshes;
    }

    void GetAllTriangles(std::vector<Triangle>& outTriangles) const {
        outTriangles.clear();
        for (const auto& submesh : subMeshes) {
            for (size_t i = 0; i < submesh.indices.size(); i += 3) {
                if (i + 2 < submesh.indices.size()) { 
                    glm::vec3 v0 = submesh.vertices[submesh.indices[i]];
                    glm::vec3 v1 = submesh.vertices[submesh.indices[i + 1]];
                    glm::vec3 v2 = submesh.vertices[submesh.indices[i + 2]];
                    outTriangles.emplace_back(v0, v1, v2);
                }
            }
        }
    }

    ~Model() {
        for (const auto& mesh : subMeshes) {
            glDeleteVertexArrays(1, &mesh.VAO);
            glDeleteBuffers(1, &mesh.VBO);
            glDeleteBuffers(1, &mesh.EBO);
        }
    }
private:

    void LoadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        ProcessNode(scene->mRootNode, scene);
    }

    void ProcessNode(aiNode* node, const aiScene* scene) {
        for (GLuint i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            subMeshes.push_back(ProcessMesh(mesh, scene));
        }
        for (GLuint i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    SubMesh ProcessMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<GLfloat> vertexBufferData; 
        SubMesh currentSubMesh;


        for (GLuint i = 0; i < mesh->mNumVertices; i++) {
            glm::vec3 pos;
            pos.x = mesh->mVertices[i].x;
            pos.y = mesh->mVertices[i].y;
            pos.z = mesh->mVertices[i].z;
            currentSubMesh.vertices.push_back(pos);

            vertexBufferData.push_back(pos.x);
            vertexBufferData.push_back(pos.y);
            vertexBufferData.push_back(pos.z);

            if (mesh->HasNormals()) {
                vertexBufferData.push_back(mesh->mNormals[i].x);
                vertexBufferData.push_back(mesh->mNormals[i].y);
                vertexBufferData.push_back(mesh->mNormals[i].z);
            } else {
                vertexBufferData.push_back(0); vertexBufferData.push_back(0); vertexBufferData.push_back(0);
            }
            if (mesh->mTextureCoords[0]) {
                vertexBufferData.push_back(mesh->mTextureCoords[0][i].x);
                vertexBufferData.push_back(mesh->mTextureCoords[0][i].y);
            } else {
                vertexBufferData.push_back(0); vertexBufferData.push_back(0);
            }
        }

        for (GLuint i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (GLuint j = 0; j < face.mNumIndices; j++)
                currentSubMesh.indices.push_back(face.mIndices[j]);
        }

        currentSubMesh.indexCount = static_cast<unsigned int>(currentSubMesh.indices.size());
        currentSubMesh.materialIndex = mesh->mMaterialIndex;

        glGenVertexArrays(1, &currentSubMesh.VAO);
        glGenBuffers(1, &currentSubMesh.VBO);
        glGenBuffers(1, &currentSubMesh.EBO);
        glBindVertexArray(currentSubMesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, currentSubMesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferData.size() * sizeof(GLfloat), &vertexBufferData[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currentSubMesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, currentSubMesh.indices.size() * sizeof(GLuint), &currentSubMesh.indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0); // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (void*)0);
        glEnableVertexAttribArray(1); // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2); // TexCoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (void*)(6 * sizeof(GLfloat)));
        glBindVertexArray(0);
        return currentSubMesh;
    }
};


#endif // !MODEL_H