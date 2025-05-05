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
                      int width, int height, float dt) {

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
    structure->updateDebris(dt);
    structure->renderDebris(activeProg);
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
                        std::vector<std::shared_ptr<Bunny>> &bunnies, int width,
                        int height) {
  // std::cout << "Drawing " << bunnies.size() << " bunnies\n";
  for (auto &bunny : bunnies) {
    if (bunny->alive) {
      bunny->drawObject(P, MV, activeProg, activeMaterial);
    }
  }
}

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
inline void
initOuterAndFloors(std::vector<std::shared_ptr<Structure>> &structures,
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
  std::shared_ptr<Structure> demoWall =
      std::make_shared<Wall>(cubeMesh, 20, 20, glm::vec3(-20.0f, 0.0f, -20.0f));
  structures.push_back(demoWall);
}

inline void initMaze(std::vector<std::shared_ptr<Structure>> &structures,
                     std::shared_ptr<Shape> &cubeMesh, float Y) {
  std::shared_ptr<Structure> wallZero =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(30.0f, Y, 20.0f));
  structures.push_back(wallZero);
  std::shared_ptr<Structure> wallOne =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(10.0f, Y, 15.0f));
  structures.push_back(wallOne);
  std::shared_ptr<Structure> wallTwo =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 15.0f), 90.0f);
  structures.push_back(wallTwo);
  std::shared_ptr<Structure> wallThree =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 10.0f), 90.0f);
  structures.push_back(wallThree);
  std::shared_ptr<Structure> wallFour =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 20.0f), 90.0f);
  structures.push_back(wallFour);
  std::shared_ptr<Structure> wallFive =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(20.0f, Y, 20.0f), 0.0f);
  structures.push_back(wallFive);
  std::shared_ptr<Structure> wallSix =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 20.0f), 0.0f);
  structures.push_back(wallSix);
  std::shared_ptr<Structure> wallSeven =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 5.0f), 0.0f);
  structures.push_back(wallSeven);
  std::shared_ptr<Structure> wallEight =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(35.0f, Y, 35.0f), 0.0f);
  structures.push_back(wallEight);
  std::shared_ptr<Structure> wallNine =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(30.0f, Y, 35.0f), 0.0f);
  structures.push_back(wallNine);
  std::shared_ptr<Structure> wallTen =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 35.0f), 0.0f);
  structures.push_back(wallTen);
  std::shared_ptr<Structure> wallEleven =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(20.0f, Y, 35.0f), 0.0f);
  structures.push_back(wallEleven);
  std::shared_ptr<Structure> wallTwelve =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 35.0f), 0.0f);
  structures.push_back(wallTwelve);
  std::shared_ptr<Structure> wallThirteen =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 35.0f), 90.0f);
  structures.push_back(wallThirteen);
  std::shared_ptr<Structure> wallFourteen =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 35.0f), 90.0f);
  structures.push_back(wallFourteen);
  std::shared_ptr<Structure> wallFifthteen =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 30.0f), 90.0f);
  structures.push_back(wallFifthteen);
  std::shared_ptr<Structure> wallSixteen =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 25.0f), 90.0f);
  structures.push_back(wallSixteen);
  std::shared_ptr<Structure> wallSeventeen =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(10.0f, Y, 35.0f), 0.0f);
  structures.push_back(wallSeventeen);
  std::shared_ptr<Structure> wallEighteen =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(5.0f, Y, 5.0f), 0.0f);
  structures.push_back(wallEighteen);
  std::shared_ptr<Structure> wallNineteen =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(10.0f, Y, 5.0f), 0.0f);
  structures.push_back(wallNineteen);
  std::shared_ptr<Structure> wallTwenty =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 5.0f), 0.0f);
  structures.push_back(wallTwenty);
  std::shared_ptr<Structure> wallTwentyOne =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(20.0f, Y, 5.0f), 0.0f);
  structures.push_back(wallTwentyOne);
  std::shared_ptr<Structure> wallTwentyTwo =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 5.0f), 0.0f);
  structures.push_back(wallTwentyTwo);
  std::shared_ptr<Structure> wallTwentyThree =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(30.0f, Y, 5.0f), 0.0f);
  structures.push_back(wallTwentyThree);
  std::shared_ptr<Structure> wallTwentyFour = std::make_shared<Wall>(
      cubeMesh, 5, 5, glm::vec3(20.0f, Y, 5.0f), -180.0f);
  structures.push_back(wallTwentyFour);
  std::shared_ptr<Structure> wallTwentyFive = std::make_shared<Wall>(
      cubeMesh, 5, 5, glm::vec3(20.0f, Y, 10.0f), -180.0f);
  structures.push_back(wallTwentyFive);
  std::shared_ptr<Structure> wallTwentySix = std::make_shared<Wall>(
      cubeMesh, 5, 5, glm::vec3(20.0f, Y, 15.0f), -180.0f);
  structures.push_back(wallTwentySix);
  std::shared_ptr<Structure> wallTwentySeven =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(10.0f, Y, 25.0f), 0.0f);
  structures.push_back(wallTwentySeven);
  std::shared_ptr<Structure> wallTwentyEight =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 25.0f), 0.0f);
  structures.push_back(wallTwentyEight);
  std::shared_ptr<Structure> wallTwentyNine =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(20.0f, Y, 25.0f), 0.0f);
  structures.push_back(wallTwentyNine);
  std::shared_ptr<Structure> wallThirty =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(35.0f, Y, 5.0f), -90.0f);
  structures.push_back(wallThirty);
  std::shared_ptr<Structure> wallThirtyOne = std::make_shared<Wall>(
      cubeMesh, 5, 5, glm::vec3(35.0f, Y, 10.0f), -90.0f);
  structures.push_back(wallThirtyOne);
  std::shared_ptr<Structure> wallThirtyTwo =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(20.0f, Y, 15.0f), 0.0f);
  structures.push_back(wallThirtyTwo);
  std::shared_ptr<Structure> wallThirtyThree =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 15.0f), 0.0f);
  structures.push_back(wallThirtyThree);
  std::shared_ptr<Structure> wallThirtyFour =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(20.0f, Y, 5.0f), -90.0f);
  structures.push_back(wallThirtyFour);
  std::shared_ptr<Structure> wallThirtyFive =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 5.0f), -90.0f);
  structures.push_back(wallThirtyFive);
  std::shared_ptr<Structure> wallThirtySix = std::make_shared<Wall>(
      cubeMesh, 5, 5, glm::vec3(25.0f, Y, 10.0f), -90.0f);
  structures.push_back(wallThirtySix);
  std::shared_ptr<Structure> wallThirtySeven =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(30.0f, Y, 30.0f), 0.0f);
  structures.push_back(wallThirtySeven);
  std::shared_ptr<Structure> wallThirtyEight =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(25.0f, Y, 30.0f), 0.0f);
  structures.push_back(wallThirtyEight);
  std::shared_ptr<Structure> wallThirtyNine =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(20.0f, Y, 30.0f), 0.0f);
  structures.push_back(wallThirtyNine);
  std::shared_ptr<Structure> wallForty =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(15.0f, Y, 30.0f), 0.0f);
  structures.push_back(wallForty);
  std::shared_ptr<Structure> wallFortyOne =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(10.0f, Y, 30.0f), 0.0f);
  structures.push_back(wallFortyOne);
  std::shared_ptr<Structure> wallFortyTwo =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(5.0f, Y, 30.0f), 0.0f);
  structures.push_back(wallFortyTwo);
  std::shared_ptr<Structure> wallFortyThree =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(0.0f, Y, 30.0f), 0.0f);
  structures.push_back(wallFortyThree);
  std::shared_ptr<Structure> wallFortyFour = std::make_shared<Wall>(
      cubeMesh, 5, 5, glm::vec3(20.0f, Y, 35.0f), -90.0f);
  structures.push_back(wallFortyFour);
  std::shared_ptr<Structure> wallFortyFive =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(5.0f, Y, 15.0f), -90.0f);
  structures.push_back(wallFortyFive);
  std::shared_ptr<Structure> wallFortySix =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(5.0f, Y, 20.0f), -90.0f);
  structures.push_back(wallFortySix);
  std::shared_ptr<Structure> wallFortySeven =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(5.0f, Y, 25.0f), -90.0f);
  structures.push_back(wallFortySeven);
  std::shared_ptr<Structure> wallFortyEight =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(5.0f, Y, 10.0f), -90.0f);
  structures.push_back(wallFortyEight);
  std::shared_ptr<Structure> wallFortyNine =
      std::make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(5.0f, Y, 15.0f), 0.0f);
  structures.push_back(wallFortyNine);
}

inline void initBunnies(std::vector<std::shared_ptr<Bunny>> &bunnies,
                        std::shared_ptr<Shape> &bunny, int Y) {

  // Create bunnies
  for (int i = 0; i < 8; i++) {
    std::shared_ptr<Bunny> bunnyTarget = std::make_shared<Bunny>(
        bunny, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), glm::vec3(1.0f), 0.0f);
    bunnyTarget->setScale(glm::vec3(1.0f));
    bunnies.push_back(bunnyTarget);
  }
  // Now move bunnies to their locations
  bunnies[0]->setTranslation(glm::vec3(38.0f, Y, 38.0f));
  bunnies[1]->setTranslation(glm::vec3(12.0f, Y, 14.0f));
  bunnies[2]->setTranslation(glm::vec3(17.3f, Y, 6.5f));
  bunnies[3]->setTranslation(glm::vec3(12.0f, Y, 2.0f));
  bunnies[4]->setTranslation(glm::vec3(27.4f, Y, 32.8f));
  bunnies[5]->setTranslation(glm::vec3(32.0f, Y, 12.0f));
  bunnies[6]->setTranslation(glm::vec3(20.0f, Y, 32.0f));
  bunnies[7]->setTranslation(glm::vec3(17.9f, Y, 17.2f));

  float Y_new = Y - 15.0f;
  for (int i = 0; i < 8; i++) {
    std::shared_ptr<Bunny> bunnyTarget = std::make_shared<Bunny>(
        bunny, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), glm::vec3(1.0f), 0.0f);
    bunnyTarget->setScale(glm::vec3(1.0f));
    bunnies.push_back(bunnyTarget);
  }
  // Now move bunnies to their locations
  bunnies[8]->setTranslation(glm::vec3(38.0f, Y_new, 38.0f));
  bunnies[9]->setTranslation(glm::vec3(12.0f, Y_new, 14.0f));
  bunnies[10]->setTranslation(glm::vec3(17.3f, Y_new, 6.5f));
  bunnies[11]->setTranslation(glm::vec3(12.0f, Y_new, 2.0f));
  bunnies[12]->setTranslation(glm::vec3(27.4f, Y_new, 32.8f));
  bunnies[13]->setTranslation(glm::vec3(32.0f, Y_new, 12.0f));
  bunnies[14]->setTranslation(glm::vec3(20.0f, Y_new, 32.0f));
  bunnies[15]->setTranslation(glm::vec3(17.9f, Y_new, 17.2f));

  Y_new = Y_new - 15.0f;
  for (int i = 0; i < 8; i++) {
    std::shared_ptr<Bunny> bunnyTarget = std::make_shared<Bunny>(
        bunny, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), glm::vec3(1.0f), 0.0f);
    bunnyTarget->setScale(glm::vec3(1.0f));
    bunnies.push_back(bunnyTarget);
  }
  // Now move bunnies to their locations
  bunnies[16]->setTranslation(glm::vec3(38.0f, Y_new, 38.0f));
  bunnies[17]->setTranslation(glm::vec3(12.0f, Y_new, 14.0f));
  bunnies[18]->setTranslation(glm::vec3(17.3f, Y_new, 6.5f));
  bunnies[19]->setTranslation(glm::vec3(12.0f, Y_new, 2.0f));
  bunnies[20]->setTranslation(glm::vec3(27.4f, Y_new, 32.8f));
  bunnies[21]->setTranslation(glm::vec3(32.0f, Y_new, 12.0f));
  bunnies[22]->setTranslation(glm::vec3(20.0f, Y_new, 32.0f));
  bunnies[23]->setTranslation(glm::vec3(17.9f, Y_new, 17.2f));
}

inline void bunnyCollisions(std::shared_ptr<BulletManager> &bulletManager,
                            std::vector<std::shared_ptr<Bunny>> &bunnies,
                            int &NUM_BUNNIES) {
  const float bulletRadius = 0.5f; // same as in your manager
  const float bunnyRadius = 1.0f;  // tweak to fit your mesh
  auto &bullets = bulletManager->getBullets();
  for (auto &b : bullets) {
    if (!b.alive)
      continue;
    for (auto &bun : bunnies) {
      if (!bun->alive)
        continue;
      float d = glm::distance(b.position, bun->getTranslation());
      if (d < bulletRadius + bunnyRadius) {
        // collision!
        bun->hit();
        b.alive = false;
        NUM_BUNNIES--;
        break; // stop testing this bullet
      }
    }
  }
}

inline void drawReticle(int width, int height) {

  glDisable(GL_DEPTH_TEST);

  // save current matrices
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, width,  // left, right
          0.0, height, // bottom, top
          -1.0, 1.0);  // near, far

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // parameters
  float cx = width * 0.5f;  // screen center X
  float cy = height * 0.5f; // screen center Y
  float r = 20.0f;          // radius in pixels
  const int segments = 64;  // how smooth the circle is

  glColor3f(1.0f, 1.0f, 1.0f);
  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < segments; ++i) {
    float theta = 2.0f * 3.14159265f * float(i) / float(segments);
    float x = cx + cosf(theta) * r;
    float y = cy + sinf(theta) * r;
    glVertex2f(x, y);
  }
  glEnd();

  // restore
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_DEPTH_TEST);
}
