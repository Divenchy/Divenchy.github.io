#include "BulletManager.h"
#include "GLM_EIGEN_COMPATIBILITY_LAYER.h"
#include "Platform.h"
#include "Structure.h"
#include "Wall.h"
#include "common.h"
#include <cassert>
#include <cstring>
#include <memory>
#include <random>
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
#include "Light.h"
#include "Object.h"
#include "Texture.h"
// clang-format on

void drawGrid(std::shared_ptr<Program> &activeProgram,
              std::shared_ptr<MatrixStack> &P,
              std::shared_ptr<MatrixStack> &MV);
void drawFrustrum(std::shared_ptr<Program> &activeProgram,
                  std::shared_ptr<Camera> &camera,
                  std::shared_ptr<MatrixStack> &P,
                  std::shared_ptr<MatrixStack> &MV,
                  std::shared_ptr<Shape> &frustrum, int width, int height);
void centerCam(std::shared_ptr<MatrixStack> &MV); // Applies transformation
void createShaders(std::string RESOURCE_DIR,
                   std::vector<std::shared_ptr<Program>> &programs);
void createMaterials(std::vector<std::shared_ptr<Material>> &materials);
void texturesBind(std::shared_ptr<Program> &prog,
                  std::shared_ptr<Texture> &texture0,
                  std::shared_ptr<Texture> &texture1,
                  std::shared_ptr<Texture> &texture2);
void texturesUnbind(std::shared_ptr<Program> &prog,
                    std::shared_ptr<Texture> &texture0,
                    std::shared_ptr<Texture> &texture1,
                    std::shared_ptr<Texture> &texture2);

// Free-look world
void createSceneObjects(std::vector<std::shared_ptr<Object>> &objects,
                        std::string RESOURCE_DIR);
void drawSceneObjects(std::vector<std::shared_ptr<Object>> &objects,
                      std::shared_ptr<MatrixStack> &P,
                      std::shared_ptr<MatrixStack> &MV,
                      std::shared_ptr<Program> &activeProgram,
                      std::shared_ptr<Material> &activeMaterial, double t);

inline void drawLevel(std::shared_ptr<Program> &activeProg,
                      std::shared_ptr<MatrixStack> &P,
                      std::shared_ptr<MatrixStack> &MV, glm::mat4 &T,
                      std::vector<std::shared_ptr<Light>> &lights,
                      std::vector<glm::vec3> &viewLightPositions,
                      std::vector<glm::vec3> &lightsColors,
                      std::shared_ptr<Material> &activeMaterial,
                      std::vector<std::shared_ptr<Material>> &materials,
                      std::vector<std::shared_ptr<Structure>> &structures,
                      std::vector<std::shared_ptr<Texture>> &textures,
                      int width, int height) {

  // Back to original shader
  glUniformMatrix4fv(activeProg->getUniform("P"), 1, GL_FALSE,
                     glm::value_ptr(P->topMatrix()));
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(MV->topMatrix()));
  glUniformMatrix3fv(activeProg->getUniform("T"), 1, GL_FALSE,
                     glm::value_ptr(T));

  // Now pass the transformed positions to the shader.
  glUniform3fv(activeProg->getUniform("lightsPos"), lights.size(),
               glm::value_ptr(viewLightPositions[0]));
  glUniform3fv(activeProg->getUniform("lightsColor"), lights.size(),
               glm::value_ptr(lightsColors[0]));

  // Set material uniforms from activeMaterial
  glUniform3f(activeProg->getUniform("ke"), activeMaterial->getMaterialKE().x,
              activeMaterial->getMaterialKE().y,
              activeMaterial->getMaterialKE().z);
  glUniform3f(activeProg->getUniform("kd"), activeMaterial->getMaterialKD().x,
              activeMaterial->getMaterialKD().y,
              activeMaterial->getMaterialKD().z);
  glUniform3f(activeProg->getUniform("ks"), activeMaterial->getMaterialKS().x,
              activeMaterial->getMaterialKS().y,
              activeMaterial->getMaterialKS().z);
  glUniform1f(activeProg->getUniform("s"), activeMaterial->getMaterialS());
  textures[0]->bind(activeProg->getUniform("texture0"));
  MV->pushMatrix();
  for (auto structure : structures) {
    structure->renderStructure(activeProg);
  }
  textures[0]->unbind();
  MV->popMatrix();
};

inline void drawGridLines(std::shared_ptr<Program> &activeProg,
                          std::shared_ptr<MatrixStack> &P,
                          std::shared_ptr<MatrixStack> &MV, glm::mat4 &T) {
  drawGrid(activeProg, P, MV);
};

inline void drawBullets(std::shared_ptr<Program> &activeProg,
                        std::shared_ptr<MatrixStack> &P,
                        std::shared_ptr<MatrixStack> &MV, float dt,
                        std::shared_ptr<BulletManager> &bulletManager,
                        std::vector<std::shared_ptr<Structure>> &structures) {
  // advance & fracture
  MV->pushMatrix();
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(MV->topMatrix()));
  bulletManager->renderBullets(activeProg);
  MV->popMatrix();
}

inline void drawBunnies(std::shared_ptr<Program> &activeProg,
                        std::shared_ptr<MatrixStack> &P,
                        std::shared_ptr<MatrixStack> &MV,
                        std::vector<std::shared_ptr<Light>> &lights,
                        std::vector<glm::vec3> &viewLightPositions,
                        std::vector<glm::vec3> &lightsColors,
                        std::shared_ptr<Material> &activeMaterial,
                        std::vector<std::shared_ptr<Material>> &materials,
                        std::vector<std::shared_ptr<Object>> &bunnies,
                        int width, int height) {
  for (auto &bunny : bunnies) {
    bunny->drawObject(P, MV, activeProg, activeMaterial);
  }
}

inline void drawFreeCubes(std::shared_ptr<Program> &activeProg,
                          std::shared_ptr<MatrixStack> &P,
                          std::shared_ptr<MatrixStack> &MV, float &oldFrameTime,
                          std::shared_ptr<BulletManager> &bulletManager,
                          std::vector<std::shared_ptr<Structure>> &structures) {
  auto freeCubes = structures[0]->getFreeCubes();
  for (auto &cc : freeCubes) {
    cc.velocity +=
        glmVec3ToEigen(glm::vec3(0.0f, -9.8f, 0.0f) * (float)STEPS_H);
    cc.position += cc.velocity * STEPS_H;
    // optional ground collision
    if (cc.position.y() < 0) {
      cc.position.y() = 0;
      cc.velocity.y() *= -0.4; // bounce
    }
    // draw a cube at cc.position with size cc.size
    MV->pushMatrix();
    MV->translate(glm::vec3(cc.position.x(), cc.position.y(), cc.position.z()));
    MV->scale(cc.size);
    glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                       glm::value_ptr(MV->topMatrix()));
    structures[0]->getMesh()->draw(activeProg);
    MV->popMatrix();
  }
};

inline void
drawHUD(GLFWwindow *window, int width, int height,
        std::shared_ptr<Program> &activeProg,
        std::shared_ptr<MatrixStack> &HUDP, std::shared_ptr<MatrixStack> &HUDMV,
        std::vector<std::shared_ptr<Light>> &lights,
        glm::vec4 &lightPosCamSpace, std::shared_ptr<Material> &activeMaterial,
        std::vector<std::shared_ptr<Material>> &materials,
        std::shared_ptr<Shape> &hudBunny, std::shared_ptr<Shape> &hudTeapot) {
  HUDP->pushMatrix();
  float fovy = glm::radians(45.0f);
  HUDP->multMatrix(glm::perspective(fovy, (float)width / (float)height, 0.1f,
                                    100.0f)); // Projection
  HUDMV->pushMatrix();
  HUDMV->multMatrix(glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0),
                                glm::vec3(0, 1, 0))); // ModelView

  // --- Begin HUD Rendering ---
  glUniformMatrix4fv(activeProg->getUniform("P"), 1, GL_FALSE,
                     glm::value_ptr(HUDP->topMatrix()));
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(HUDMV->topMatrix()));

  // Set light position uniform on the active program
  glUniform3f(activeProg->getUniform("lightPos"), lightPosCamSpace.x,
              lightPosCamSpace.y, lightPosCamSpace.z);

  // Set material uniforms from activeMaterial
  glUniform3f(activeProg->getUniform("ke"), activeMaterial->getMaterialKE().x,
              activeMaterial->getMaterialKE().y,
              activeMaterial->getMaterialKE().z);
  glUniform3f(activeProg->getUniform("kd"), activeMaterial->getMaterialKD().x,
              activeMaterial->getMaterialKD().y,
              activeMaterial->getMaterialKD().z);
  glUniform3f(activeProg->getUniform("ks"), activeMaterial->getMaterialKS().x,
              activeMaterial->getMaterialKS().y,
              activeMaterial->getMaterialKS().z);
  glUniform1f(activeProg->getUniform("s"), activeMaterial->getMaterialS());

  // Get the current window size.
  glfwGetFramebufferSize(window, &width, &height);

  // Disable depth testing so the HUD always appears on top.
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  // --- Upper Left HUD Object (Bunny) ---

  // Choose the depth at which to place the HUD objects.
  float d = 5.0f;
  float halfHeight = d * tan(fovy / 2.0f);
  float halfWidth = halfHeight * ((float)width / (float)height);

  // Define margins in world units.
  float marginX = -0.9f;
  float marginY = -0.7f;

  // Compute positions in view space (assuming camera at origin, looking down
  // -Z).
  glm::vec3 upperLeftPos(-halfWidth + marginX, halfHeight - marginY, -d);
  glm::vec3 upperRightPos(halfWidth - marginX, halfHeight - marginY, -d);

  float hudObjectSize = 0.6f;                 // desired size for HUD objects
  float rotationAngle = (float)glfwGetTime(); // rotation angle based on time

  // glm::mat4 hudMV = glm::mat4(1.0f);
  HUDMV->pushMatrix();
  HUDMV->translate(upperLeftPos);
  HUDMV->rotate(rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
  HUDMV->scale(glm::vec3(hudObjectSize, hudObjectSize, hudObjectSize));
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(HUDMV->topMatrix()));
  hudBunny->draw(activeProg);
  HUDMV->popMatrix();

  // --- Upper Right HUD Object (Teapot) ---
  // hudMV = glm::mat4(1.0f);
  HUDMV->pushMatrix();
  HUDMV->translate(upperRightPos);
  HUDMV->translate(glm::vec3(0.0f, 0.2f, 0.0f));
  HUDMV->rotate(rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
  HUDMV->scale(glm::vec3(hudObjectSize, hudObjectSize, hudObjectSize));
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(HUDMV->topMatrix()));
  hudTeapot->draw(activeProg);
  HUDMV->popMatrix();

  // Re-enable depth testing after HUD pass.
  glEnable(GL_DEPTH_TEST);

  HUDMV->popMatrix();
  HUDP->popMatrix();
  // --- End HUD Rendering ---
};

// Levels
inline void initFloorOne(std::vector<std::shared_ptr<Structure>> &structures,
                         std::shared_ptr<Shape> &cubeMesh) {

  std::shared_ptr<Structure> floorOne =
      std::make_shared<Platform>(cubeMesh, 40, 40, glm::vec3(0.0f));
  floorOne->setFracturable(false);
  structures.push_back(floorOne);
  std::shared_ptr<Structure> floorTwo = std::make_shared<Platform>(
      cubeMesh, 40, 40, glm::vec3(0.0f, 15.0f, 0.0f));
  structures.push_back(floorTwo);
  std::shared_ptr<Structure> floorThree = std::make_shared<Platform>(
      cubeMesh, 40, 40, glm::vec3(0.0f, 30.0f, 0.0f));
  structures.push_back(floorThree);
  std::shared_ptr<Structure> outerWallOne =
      std::make_shared<Wall>(cubeMesh, 40, 50, glm::vec3(0.0f, 0.0f, 0.0f));
  outerWallOne->setFracturable(false);
  structures.push_back(outerWallOne);
  std::shared_ptr<Structure> outerWallTwo =
      std::make_shared<Wall>(cubeMesh, 40, 50, glm::vec3(0.0f, 0.0f, 40.0f));
  outerWallTwo->setFracturable(false);
  structures.push_back(outerWallTwo);
  std::shared_ptr<Structure> outerWallThree =
      std::make_shared<Wall>(cubeMesh, 40, 50, glm::vec3(0.0f, 0.0f, 0.0f));
  outerWallThree->setFracturable(false);
  outerWallThree->rotate(glm::radians(-90.0f), GLM_AXIS_Y);
  structures.push_back(outerWallThree);
  std::shared_ptr<Structure> outerWallFour =
      std::make_shared<Wall>(cubeMesh, 40, 50, glm::vec3(0.0f, 0.0f, -40.0f));
  outerWallFour->setFracturable(false);
  outerWallFour->rotate(glm::radians(-90.0f), GLM_AXIS_Y);
  structures.push_back(outerWallFour);
}

inline void initBunnies(std::vector<std::shared_ptr<Object>> &bunnies,
                        std::shared_ptr<Shape> &bunny) {

  // Create bunnies
  for (int i = 0; i < 8; i++) {
    std::shared_ptr<Object> bunnyTarget = std::make_shared<Object>(
        bunny, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), glm::vec3(1.0f), 0.0f);
    bunnies.push_back(bunnyTarget);
  }
  // Now move bunnies to their locations
  bunnies[0]->setTranslation(glm::vec3(20.0f, 40.0f, 20.0f));
}
