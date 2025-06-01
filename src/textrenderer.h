#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <glad/glad.h>
#include <map>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>

struct Character {
    GLuint TextureID;   // ��������ID
    glm::ivec2 Size;    // ���δ�С
    glm::ivec2 Bearing; // ���ߵ�������/��������
    GLuint Advance;     // ���ǰ������
};

class TextRenderer {
public:
    std::map<wchar_t, Character> Characters;
    GLuint VAO, VBO;
    GLuint shaderID;
    TextRenderer(const char* fontPath, GLuint shaderID);
    ~TextRenderer();
    void RenderText(const std::wstring& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, GLuint windowWidth, GLuint windowHeight);
    void PrintLoadedCharacters() const;
};

#endif
