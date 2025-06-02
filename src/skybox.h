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
    Texture* dayCubemapTexture; // Cubemap for daytime
    Texture* duskCubemapTexture; // Cubemap for dusk
    Texture* nightCubemapTexture; // Cubemap for nighttime
    std::vector<float> vertices;

public:
    Skybox() {
        // Unit cube vertex data (36 vertices, 6 faces)
        vertices = {
            // Back face
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            // Front face
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            // Left face
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            // Right face
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            // Top face
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            // Bottom face
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f
        };

        // Setup VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        // Load daytime cubemap texture (5 faces + placeholder for bottom)
        std::vector<std::string> dayFaces = {
            "res/texture/skybox/day_right.jpg",
            "res/texture/skybox/day_left.jpg",
            "res/texture/skybox/day_top.jpg",
            "res/texture/skybox/day_front.jpg", // Use front as placeholder for bottom
            "res/texture/skybox/day_front.jpg",
            "res/texture/skybox/day_back.jpg"
        };
        dayCubemapTexture = new Texture(dayFaces);
        if (!dayCubemapTexture->IsLoaded()) {
            std::cerr << "Failed to load daytime skybox cubemap" << std::endl;
        }

        // Load dusk cubemap texture (5 faces + placeholder for bottom)
        std::vector<std::string> duskFaces = {
            "res/texture/skybox/desert-evening/desert_evening_right.jpg",
            "res/texture/skybox/desert-evening/desert_evening_left.jpg",
            "res/texture/skybox/desert-evening/desert_evening_top.jpg",
            "res/texture/skybox/desert-evening/desert_evening_front.jpg", // Use front as placeholder for bottom
            "res/texture/skybox/desert-evening/desert_evening_front.jpg",
            "res/texture/skybox/desert-evening/desert_evening_back.jpg"
        };
        duskCubemapTexture = new Texture(duskFaces);
        if (!duskCubemapTexture->IsLoaded()) {
            std::cerr << "Failed to load dusk skybox cubemap" << std::endl;
        }

        // Load nighttime cubemap texture
        std::vector<std::string> nightFaces = {
            "res/texture/right.jpg",
            "res/texture/left.jpg",
            "res/texture/top.jpg",
            "res/texture/bottom.jpg",
            "res/texture/front.jpg",
            "res/texture/back.jpg"
        };
        nightCubemapTexture = new Texture(nightFaces);
        if (!nightCubemapTexture->IsLoaded()) {
            std::cerr << "Failed to load nighttime skybox cubemap" << std::endl;
        }

        // Load skybox shader
        skyboxShader = new Shader("res/shader/skybox.vert", "res/shader/skybox.frag");
    }

    void Render(const glm::mat4& view, const glm::mat4& projection, float dayNightCycle) {
        glDepthFunc(GL_LEQUAL); // Ensure skybox passes depth test
        skyboxShader->Bind();
        skyboxShader->SetMat4("view", glm::mat4(glm::mat3(view))); // Remove translation component
        skyboxShader->SetMat4("projection", projection);
        skyboxShader->SetFloat("dayNightCycle", dayNightCycle); // Pass day-night cycle factor

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, dayCubemapTexture->GetId());
        skyboxShader->SetInt("daySkybox", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, duskCubemapTexture->GetId());
        skyboxShader->SetInt("duskSkybox", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, nightCubemapTexture->GetId());
        skyboxShader->SetInt("nightSkybox", 2);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        skyboxShader->Unbind();
        glDepthFunc(GL_LESS); // Restore default depth test
    }

    ~Skybox() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        delete skyboxShader;
        delete dayCubemapTexture;
        delete duskCubemapTexture;
        delete nightCubemapTexture;
    }
};

#endif