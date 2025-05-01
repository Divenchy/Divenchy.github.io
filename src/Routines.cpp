#include "Routines.h"

using std::make_shared, std::shared_ptr, std::string, std::vector, glm::vec3,
    glm::vec4, glm::mat4;

// Draw grid
// TODO: BUILDING DIMENSIONS 15m x 5m
void drawGrid(shared_ptr<Program> &activeProgram, shared_ptr<MatrixStack> &P,
              shared_ptr<MatrixStack> &MV) {
  glUniformMatrix4fv(activeProgram->getUniform("P"), 1, GL_FALSE,
                     glm::value_ptr(P->topMatrix()));
  glUniformMatrix4fv(activeProgram->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(MV->topMatrix()));
  glLineWidth(2.0f);
  float x0 = -100.0f;
  float x1 = 100.0f;
  float z0 = -100.0f;
  float z1 = 100.0f;
  int gridSize = 100;
  glLineWidth(1.0f);
  glBegin(GL_LINES);
  for (int i = 1; i < gridSize; ++i) {
    if (i == gridSize / 2) {
      glColor3f(0.1f, 0.1f, 0.1f);
    } else {
      glColor3f(0.8f, 0.8f, 0.8f);
    }
    float x = x0 + i / (float)gridSize * (x1 - x0);
    glVertex3f(x, 0.0f, z0);
    glVertex3f(x, 0.0f, z1);
  }
  for (int i = 1; i < gridSize; ++i) {
    if (i == gridSize / 2) {
      glColor3f(0.1f, 0.1f, 0.1f);
    } else {
      glColor3f(0.8f, 0.8f, 0.8f);
    }
    float z = z0 + i / (float)gridSize * (z1 - z0);
    glVertex3f(x0, 0.0f, z);
    glVertex3f(x1, 0.0f, z);
  }
  glEnd();
  glColor3f(0.4f, 0.4f, 0.4f);
  glBegin(GL_LINE_LOOP);
  glVertex3f(x0, 0.0f, z0);
  glVertex3f(x1, 0.0f, z0);
  glVertex3f(x1, 0.0f, z1);
  glVertex3f(x0, 0.0f, z1);
  glEnd();
}

// Draw frustrum
void drawFrustrum(std::shared_ptr<Program> &activeProgram,
                  std::shared_ptr<Camera> &camera,
                  std::shared_ptr<MatrixStack> &P,
                  std::shared_ptr<MatrixStack> &MV,
                  std::shared_ptr<Shape> &frustrum, int width, int height) {

  activeProgram->bind();
  glUniformMatrix4fv(activeProgram->getUniform("P"), 1, GL_FALSE,
                     glm::value_ptr(P->topMatrix()));
  glUniformMatrix4fv(activeProgram->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(MV->topMatrix()));
  glUniform3f(activeProgram->getUniform("kd"), 1.0f, 0.0f, 0.0f);

  // Draw frustrum
  // drawFrustrum(activeProg, TOPDOWNP, TOPDOWNMV, frustrum);
  MV->pushMatrix();

  // Obtain inverse of camera matrix
  glm::mat4 cameraMatInv = glm::inverse(camera->getViewMatrixFreeLook());
  MV->multMatrix(cameraMatInv);

  // So frustum always on top
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  // Set frustrum FOV
  float fovy =
      glm::radians(45.0f); // replace with your current FOV if different
  float aspect = (float)width / (float)height;
  // The frustum appears correctly only when FOV equals the design FOV. To
  // correct for any difference, compute scale factors in X and Y. (Here we
  // assume the frustum was modeled for a FOV of 45°.)
  float scaleX = tan(fovy / 2.0f) * aspect;
  float scaleY = tan(fovy / 2.0f);
  MV->scale(scaleX, scaleY, 1.0f);

  // Draw shape
  glUniformMatrix4fv(activeProgram->getUniform("MV"), 1, GL_FALSE,
                     &MV->topMatrix()[0][0]);
  frustrum->draw(activeProgram);

  MV->popMatrix();

  glViewport(0, 0, width, height);
  activeProgram->unbind();
}

// Create shaders
void createShaders(string RESOURCE_DIR, vector<shared_ptr<Program>> &programs) {

  // Blinn_phong
  std::shared_ptr<Program> blingProg = make_shared<Program>();
  blingProg->setShaderNames(RESOURCE_DIR + "bling_phong_vert.glsl",
                            RESOURCE_DIR + "bling_phong_frag.glsl");
  blingProg->setVerbose(true);
  blingProg->init();
  blingProg->addAttribute("aPos");
  blingProg->addAttribute("aNor");
  blingProg->addAttribute("aInstMat0");
  blingProg->addAttribute("aInstMat1");
  blingProg->addAttribute("aInstMat2");
  blingProg->addAttribute("aInstMat3");
  blingProg->addUniform("MV");
  blingProg->addUniform("P");
  blingProg->addUniform("normalMatrix"); // New uniform for transforming normals
  blingProg->addUniform("lightPos");
  blingProg->addUniform("ka");
  blingProg->addUniform("kd");
  blingProg->addUniform("ks");
  blingProg->addUniform("s");
  blingProg->setVerbose(false);
  programs.push_back(blingProg);

  // Default
  std::shared_ptr<Program> prog = make_shared<Program>();
  prog->setShaderNames(RESOURCE_DIR + "normal_vert.glsl",
                       RESOURCE_DIR + "normal_frag.glsl");
  prog->setVerbose(true);
  prog->init();
  prog->addAttribute("aPos");
  prog->addAttribute("aNor");
  prog->addUniform("MV");
  prog->addUniform("P");
  prog->setVerbose(false);

  programs.push_back(prog);

  // HUD Shader
  std::shared_ptr<Program> progHUD = make_shared<Program>();
  progHUD->setShaderNames(RESOURCE_DIR + "hud_vert.glsl",
                          RESOURCE_DIR + "hud_frag.glsl");
  progHUD->setVerbose(true);
  progHUD->init();
  progHUD->addAttribute("aPos");
  progHUD->addAttribute("aNor");
  progHUD->addUniform("MV");
  progHUD->addUniform("P");
  progHUD->addUniform("normalMatrix"); // New uniform for transforming normals
  progHUD->addUniform("lightPos");
  progHUD->addUniform("ka");
  progHUD->addUniform("kd");
  progHUD->addUniform("ks");
  progHUD->addUniform("s");
  progHUD->setVerbose(false);
  programs.push_back(progHUD);

  // Imported from lab 9
  std::shared_ptr<Program> progTexture = make_shared<Program>();
  progTexture->setShaderNames(RESOURCE_DIR + "bling_phong_vert_texture.glsl",
                              RESOURCE_DIR + "bling_phong_frag_texture.glsl");
  progTexture->setVerbose(true);
  progTexture->init();
  progTexture->addAttribute("aPos");
  progTexture->addAttribute("aNor");
  progTexture->addAttribute("aTex");
  progTexture->addUniform("P");
  progTexture->addUniform("MV");
  progTexture->addUniform("T");
  progTexture->addUniform("texture0");
  progTexture->addUniform("texture1");
  progTexture->addUniform("texture2");
  progTexture->addUniform("useCloudTexture");
  progTexture->addUniform("lightPos");
  programs.push_back(progTexture);
}

// Help from ChatGPT for reasoning
void createMaterials(std::vector<std::shared_ptr<Material>> &materials) {
  // increase s factor for shininess, kd determines color of bunny, ks
  // determines color of shine (specular highlights)
  // Args: ka, kd, ks , s
  shared_ptr<Material> bling_phong =
      make_shared<Material>(vec3(0.2f, 0.2f, 0.2f), vec3(0.8f, 0.7f, 0.7f),
                            vec3(1.0f, 0.9f, 0.8f), 200.0f);
  shared_ptr<Material> blue_shiny =
      make_shared<Material>(vec3(0.0f, 0.0f, 0.1f), vec3(0.1f, 0.2f, 0.8f),
                            vec3(0.0f, 1.0f, 0.2f), 300.0f);
  shared_ptr<Material> gray_matte =
      make_shared<Material>(vec3(0.2f, 0.2f, 0.2f), vec3(0.5f, 0.5f, 0.5f),
                            vec3(0.2f, 0.2f, 0.2f), 20.0f);

  materials.push_back(bling_phong);
  materials.push_back(blue_shiny);
  materials.push_back(gray_matte);
}

// Apply transforms to camera/view
void centerCam(std::shared_ptr<MatrixStack> &MV) {
  MV->translate(0.0f, -1.0f, 0.0f);
}

// Free-look world
void createSceneObjects(std::vector<std::shared_ptr<Object>> &objects,
                        std::string RESOURCE_DIR) {

  // Load meshes once
  shared_ptr<Shape> bunny = make_shared<Shape>();
  bunny->loadMesh(RESOURCE_DIR + "bunny.obj");
  bunny->init();

  shared_ptr<Shape> teapot = make_shared<Shape>();
  teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
  teapot->init();

  // Floor is 25 x 25 units
  float topLeft = -12.5f;
  for (int i = 0; i < 10; i++) {
    float z = i * 2.5f;
    for (int j = 0; j < 10; j++) {
      float x = j * 2.5f;
      // Random number for variation
      std::random_device rd;
      std::mt19937 gen(rd());

      // Define a uniform distribution in the range [1, 100].
      std::uniform_int_distribution<> dis(1, 100);

      // Generate a random number.
      float randVal = dis(gen) / 100.0f; // Make it 0 to 1
      float scaleFactor = 0.8f + (randVal * 0.75f);
      if (scaleFactor > 1.3f) {
        scaleFactor = 1.3f;
      }

      shared_ptr<Shape> shape;
      if (randVal < 0.8f) {
        shape = bunny;
      } else {
        shape = teapot;
      }

      // Apply small offset so that it is not straight on grid
      shared_ptr<Object> obj = make_shared<Object>(
          shape,
          vec3(topLeft + (x + randVal * 0.85f), 0.0f,
               topLeft + z + (randVal * 0.85f)),
          0.0f, glm::quat(), vec3(randVal, randVal, randVal), 0.0f);
      obj->setFactor(scaleFactor);

      objects.push_back(obj);
    }
  }
}

void drawSceneObjects(std::vector<std::shared_ptr<Object>> &objects,
                      std::shared_ptr<MatrixStack> &P,
                      std::shared_ptr<MatrixStack> &MV,
                      std::shared_ptr<Program> &activeProgram,
                      std::shared_ptr<Material> &activeMaterial, double t) {
  for (size_t i = 0; i < objects.size(); i++) {

    shared_ptr<Object> obj = objects[i];
    // Load obj's material here (color)
    shared_ptr<Material> material = obj->getMaterial();
    glUniform3f(activeProgram->getUniform("kd"), material->getMaterialKD().x,
                material->getMaterialKD().y, material->getMaterialKD().z);

    // Use t to interpolate values from 0.5 to 1 (oscillating)
    float scaleSet = obj->getFactor() + 0.25f * sin(t + i);

    obj->setScale(vec3(scaleSet, scaleSet, scaleSet));
    obj->drawObject(0, P, MV, activeProgram, activeMaterial);
  }
}

void texturesBind(std::shared_ptr<Program> &prog,
                  std::shared_ptr<Texture> &texture0,
                  std::shared_ptr<Texture> &texture1,
                  std::shared_ptr<Texture> &texture2) {

  texture0->bind(prog->getUniform("texture0"));
  texture1->bind(prog->getUniform("texture1"));
  texture2->bind(prog->getUniform("texture2"));
}

void texturesUnbind(std::shared_ptr<Program> &prog,
                    std::shared_ptr<Texture> &texture0,
                    std::shared_ptr<Texture> &texture1,
                    std::shared_ptr<Texture> &texture2) {

  texture0->unbind();
  texture1->unbind();
  texture2->unbind();
}
