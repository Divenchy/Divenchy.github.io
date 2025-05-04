#include "Object.h"
#include <random>

using std::make_shared, std::shared_ptr, std::string, std::vector, glm::vec3;

void Object::drawObject(std::shared_ptr<MatrixStack> &P,
                        std::shared_ptr<MatrixStack> &MV,
                        std::shared_ptr<Program> &activeProgram,
                        std::shared_ptr<Material> &activeMaterial) {

  // Use MV and program
  float minY = mesh->getMinY();
  MV->pushMatrix();
  MV->translate(translation);

  MV->translate(0.0f, -minY, 0.0f);
  vec3 baseCenter = mesh->getBaseCenter();
  MV->translate(baseCenter);
  MV->scale(this->scale);
  MV->translate(-baseCenter);
  // Push to ground

  glUniformMatrix4fv(activeProgram->getUniform("MV"), 1, GL_FALSE,
                     &MV->topMatrix()[0][0]);
  mesh->draw(activeProgram);
  MV->popMatrix();
}

// Parametrized constructor
Object::Object(std::shared_ptr<Shape> mesh, glm::vec3 translation,
               float rotAngle, glm::quat quaternion, glm::vec3 scale,
               float shearFactor) {
  this->translation = translation;
  this->rotAngle = rotAngle;
  this->quaternion = quaternion;
  this->scale = scale;
  this->ShearMat = glm::mat4(1.0f);
  this->shearFactor = shearFactor;
  this->scaleFactor = 1.0f;
  // Init mesh
  this->mesh = std::make_shared<Shape>();
  this->mesh = mesh;

  // Init material w/ random color

  // Gernerate random color (ChatGPT aid)
  // Create a random device and seed the Mersenne Twister engine.
  std::random_device rd;
  std::mt19937 gen(rd());

  // Define a uniform distribution in the range [1, 100].
  std::uniform_int_distribution<> dis(1, 100);

  // Generate a random number.
  float r = dis(gen) / 100.0f; // Make it 0 to 1
  float g = dis(gen) / 100.0f; // Make it 0 to 1
  float b = dis(gen) / 100.0f; // Make it 0 to 1

  this->material = std::make_shared<Material>(
      vec3(0.2f, 0.2f, 0.2f), vec3(r, g, b), vec3(0.0f, 0.0f, 0.0f), 200.0f);
}
