// TextRenderer.h
#pragma once

#include "Program.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <map>
#include <string>

struct Character {
  GLuint TextureID;   // ID handle of the glyph texture
  glm::ivec2 Size;    // size of glyph
  glm::ivec2 Bearing; // offset from baseline to left/top of glyph
  GLuint Advance;     // horizontal offset to advance to next glyph
};

class TextRenderer {
public:
  // holds a map of pre‑loaded Characters
  std::map<GLchar, Character> Characters;
  GLuint VAO, VBO;

  // Initialize: load font at given size, compile text shader
  void Init(const std::string &fontFile, GLuint fontSize,
            std::shared_ptr<Program> &TextShader);

  // Render text at (x,y) in pixels from lower‑left corner
  void RenderText(const std::string &text, GLfloat x, GLfloat y, GLfloat scale,
                  const glm::vec3 &color, std::shared_ptr<Program> &TextShader);
};
