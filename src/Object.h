#include "glm/matrix.hpp"
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
#include "Shape.h"
#include "Material.h"
// clang-format on

class Object {
private:
  // Mesh
  std::shared_ptr<Shape> mesh;
  // Transforms
  glm::vec3 translation;
  float rotAngle;
  glm::quat quaternion;
  glm::vec3 scale;
  float scaleFactor;
  glm::mat4 ShearMat;
  float shearFactor;

  // Material
  std::shared_ptr<Material> material;

public:
  Object(std::shared_ptr<Shape> mesh, glm::vec3 translation, float rotAngle,
         glm::quat quaternion, glm::vec3 scale, float shearFactor);
  void drawObject(std::shared_ptr<MatrixStack> &P,
                  std::shared_ptr<MatrixStack> &MV,
                  std::shared_ptr<Program> &activeProgram,
                  std::shared_ptr<Material> &activeMaterial);
  std::shared_ptr<Material> getMaterial() { return material; }
  void setScale(const glm::vec3 &scale) { this->scale = scale; }
  void setFactor(const float &factor) { this->scaleFactor = factor; }
  void setTranslation(const glm::vec3 &translation) {
    this->translation = translation;
  };
  void setRotation(float angle) { this->rotAngle = angle; };
  float getFactor() { return this->scaleFactor; }
};
