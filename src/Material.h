#include <cassert>
#include <cstring>
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
#include "Shape.h"
// clang-format on

class Material {
private:
  glm::vec3 ke; // emmisive
  glm::vec3 kd; // diffusion
  glm::vec3 ks; // specular
  float s;      // specular scale/strength

public:
  Material(const glm::vec3 &ke, const glm::vec3 &kd, const glm::vec3 &ks,
           float s);
  glm::vec3 getMaterialKE();
  glm::vec3 getMaterialKD();
  glm::vec3 getMaterialKS();
  float getMaterialS();
};
