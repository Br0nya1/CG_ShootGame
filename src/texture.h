#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <stb_image/stb_image.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Texture {
private:
    GLuint id;
    bool isLoaded;
    GLenum textureType;

public:
    // 构造函数：加载 2D 纹理
    Texture(const char* path) : isLoaded(false), textureType(GL_TEXTURE_2D) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = 0;
            if (nrComponents == 1 || nrComponents == 2)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            isLoaded = true;
        }
        else {
            cout << "Texture failed to load at path: " << path << endl;
        }
        stbi_image_free(data);
    }

    Texture(const std::vector<std::string>& faces) : isLoaded(false), textureType(GL_TEXTURE_CUBE_MAP) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);

        int width, height, nrComponents;
        for (unsigned int i = 0; i < faces.size(); i++) {
            unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
            if (data) {
                GLenum format = (nrComponents == 3) ? GL_RGB : GL_RGBA;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
                isLoaded = true;
            }
            else {
                cout << "Cubemap texture failed to load at path: " << faces[i] << endl;
                stbi_image_free(data);
                isLoaded = false;
                return;
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    GLuint GetId() const {
        return id;
    }

    bool IsLoaded() const {
        return isLoaded;
    }

    GLenum GetTextureType() const {
        return textureType;
    }

    ~Texture() {
        glDeleteTextures(1, &id);
    }
};

#endif