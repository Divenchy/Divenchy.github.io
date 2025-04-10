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

void drawGrid(std::shared_ptr<Program> &activeProgram, std::shared_ptr<MatrixStack> &P, std::shared_ptr<MatrixStack> &MV);
void drawFrustrum(std::shared_ptr<Program> &activeProgram, std::shared_ptr<Camera> &camera, std::shared_ptr<MatrixStack> &P,
                  std::shared_ptr<MatrixStack> &MV, std::shared_ptr<Shape> &frustrum, int width, int height);
void centerCam(std::shared_ptr<MatrixStack> &MV); // Applies transformation
void createShaders(std::string RESOURCE_DIR, std::vector<std::shared_ptr<Program>> &programs);
void createMaterials(std::vector<std::shared_ptr<Material>> &materials);
void texturesBind(std::shared_ptr<Program> &prog, std::shared_ptr<Texture> &texture0, std::shared_ptr<Texture> &texture1,
                  std::shared_ptr<Texture> &texture2);
void texturesUnbind(std::shared_ptr<Program> &prog, std::shared_ptr<Texture> &texture0, std::shared_ptr<Texture> &texture1,
                    std::shared_ptr<Texture> &texture2);

// Free-look world
void createSceneObjects(std::vector<std::shared_ptr<Object>> &objects, std::string RESOURCE_DIR);
void drawSceneObjects(std::vector<std::shared_ptr<Object>> &objects, std::shared_ptr<MatrixStack> &P,
                      std::shared_ptr<MatrixStack> &MV, std::shared_ptr<Program> &activeProgram,
                      std::shared_ptr<Material> &activeMaterial, double t);
