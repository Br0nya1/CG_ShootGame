#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "shader.h"
#include "texture.h"

class Skybox {
private:
    GLuint VAO, VBO;
    Shader* skyboxShader;
    Texture* cubemapTexture;
    std::vector<float> vertices;

public:
    Skybox() {
        // ��λ�����嶥�����ݣ�36�����㣬6���棩
        vertices = {
            // ����
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            // ǰ��
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            // ����
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            // ����
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             // ����
             -1.0f,  1.0f,  1.0f,
             -1.0f,  1.0f, -1.0f,
              1.0f,  1.0f, -1.0f,
              1.0f,  1.0f, -1.0f,
              1.0f,  1.0f,  1.0f,
             -1.0f,  1.0f,  1.0f,
             // ����
             -1.0f, -1.0f, -1.0f,
             -1.0f, -1.0f,  1.0f,
              1.0f, -1.0f,  1.0f,
              1.0f, -1.0f,  1.0f,
              1.0f, -1.0f, -1.0f,
             -1.0f, -1.0f, -1.0f
        };

        // ���� VAO �� VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        // ���� Cubemap ����
        std::vector<std::string> faces = {
            "res/texture/skybox/right.jpg",
            "res/texture/skybox/left.jpg",
            "res/texture/skybox/top.jpg",
            "res/texture/skybox/bottom.jpg",
            "res/texture/skybox/front.jpg",
            "res/texture/skybox/back.jpg"
        };
        cubemapTexture = new Texture(faces);
        if (!cubemapTexture->IsLoaded()) {
            std::cerr << "Failed to load skybox cubemap" << std::endl;
        }

        // ������պ���ɫ��
        skyboxShader = new Shader("res/shader/skybox.vert", "res/shader/skybox.frag");
    }

    void Render(const glm::mat4& view, const glm::mat4& projection) {
        glDepthFunc(GL_LEQUAL); // ȷ����պ�ͨ����Ȳ���
        skyboxShader->Bind();
        skyboxShader->SetMat4("view", glm::mat4(glm::mat3(view))); // �Ƴ�ƽ�Ʒ���
        skyboxShader->SetMat4("projection", projection);

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture->GetId());
        skyboxShader->SetInt("skybox", 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        skyboxShader->Unbind();
        glDepthFunc(GL_LESS); // �ָ�Ĭ����Ȳ���
    }

    ~Skybox() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        delete skyboxShader;
        delete cubemapTexture;
    }
};

#endif