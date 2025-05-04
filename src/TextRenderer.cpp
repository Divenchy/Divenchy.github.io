// TextRenderer.cpp
#include "TextRenderer.h"
#include <ft2build.h>
#include FT_FREETYPE_H

void TextRenderer::Init(const std::string &fontFile, GLuint fontSize,
                        std::shared_ptr<Program> &TextShader) {

  // 2) FreeType load
  FT_Library ft;
  FT_Init_FreeType(&ft);
  FT_Face face;
  FT_New_Face(ft, fontFile.c_str(), 0, &face);
  FT_Set_Pixel_Sizes(face, 0, fontSize);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // 3) load first 128 ASCII chars
  for (GLubyte c = 0; c < 128; c++) {
    FT_Load_Char(face, c, FT_LOAD_RENDER);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                 face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                 face->glyph->bitmap.buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Character ch = {tex,
                    {face->glyph->bitmap.width, face->glyph->bitmap.rows},
                    {face->glyph->bitmap_left, face->glyph->bitmap_top},
                    (GLuint)face->glyph->advance.x};
    Characters.insert(std::pair<GLchar, Character>(c, ch));
  }
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  // 4) configure VAO/VBO for quads
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr,
               GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(TextShader->getAttribute("aPos"));
  glVertexAttribPointer(TextShader->getAttribute("aPos"), 4, GL_FLOAT, GL_FALSE,
                        4 * sizeof(GLfloat), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void TextRenderer::RenderText(const std::string &text, GLfloat x, GLfloat y,
                              GLfloat scale, const glm::vec3 &color,
                              std::shared_ptr<Program> &TextShader) {
  // assume orthographic projection already set:
  glUniform3f(TextShader->getUniform("textColor"), color.x, color.y, color.z);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(VAO);

  for (auto c : text) {
    Character &ch = Characters[c];
    GLfloat xpos = x + ch.Bearing.x * scale;
    GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
    GLfloat w = ch.Size.x * scale;
    GLfloat h = ch.Size.y * scale;
    // update VBO for each glyph quad
    GLfloat vertices[6][4] = {
        {xpos, ypos + h, 0.0f, 0.0f},    {xpos, ypos, 0.0f, 1.0f},
        {xpos + w, ypos, 1.0f, 1.0f},

        {xpos, ypos + h, 0.0f, 0.0f},    {xpos + w, ypos, 1.0f, 1.0f},
        {xpos + w, ypos + h, 1.0f, 0.0f}};
    glBindTexture(GL_TEXTURE_2D, ch.TextureID);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    x += (ch.Advance >> 6) * scale; // advance.x is in 1/64 pixels
  }
  glBindVertexArray(0);
}
