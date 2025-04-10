#pragma once
#include <cassert>
#include <cstring>
#include <memory>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// clang-format off
#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"

class Light {
// Make it public so don't have to bother with getters
public:
  glm::vec3 pos;
  glm::vec3 color;

  Light(const glm::vec3 &pos, const glm::vec3 &color) {
    this->pos = pos;
    this->color = color;
  };
};
