#include "GLFW/glfw3.h"
#include "TextRenderer.h"
#include "glm/matrix.hpp"
#include "pch.h"
#include <iterator>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <SFML/Audio.hpp>

// clang-format off
#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Routines.h"
#include "Structure.h"
#include "Wall.h"
#include "BulletManager.h"
#include "Player.h"
// clang-format on

using namespace std;

int width, height;
GLFWwindow *window;         // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
int TASK = 1;
bool OFFLINE = false;

// For shear
glm::mat4 S(1.0f);
// clang-format off
glm::mat4 T(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), 
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), 
            glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
            glm::vec4(10.0f, 10.0f, 0.0f, 1.0f));
// clang-format on

// AUDIO / UI
sf::Music music;
bool isPlaying;
TextRenderer text;
glm::mat4 ortho;
int NUM_BUNNIES;
static double frozenTime = 0.0;
bool updateTime = true;

shared_ptr<Player> player;
shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Shape> shape;
shared_ptr<Shape> frustrum;

// Shapes
shared_ptr<Shape> cubeMesh;
shared_ptr<Shape> sphereMesh;
shared_ptr<Shape> bunny;
shared_ptr<BulletManager> bulletManager;
std::vector<shared_ptr<Structure>> structures;
std::vector<shared_ptr<Bunny>> bunnies;

// Textures
shared_ptr<Texture> wallTex;
std::vector<shared_ptr<Texture>> textures;

// HUD
shared_ptr<Shape> hudBunny;
shared_ptr<Shape> hudTeapot;

// Objects vector
std::vector<std::shared_ptr<Object>> objects;

// Lights, and Material stacks
std::vector<shared_ptr<Program>> programs;
std::vector<shared_ptr<Material>> materials;
std::vector<shared_ptr<Light>> lights;

// Shaders
glm::vec3 kd;
glm::vec3 ks;
glm::vec3 ke;
glm::vec3 s;
glm::vec3 vPos;
glm::vec3 vNor;

// Might need to get these
glm::vec3 aPos;
glm::vec3 aNor;

// Used for cycling through shaders
int shaderIndex = 0;
int materialIndex = 0;
int lightsIndex = 0;

bool enableTopDown = false;

bool keyToggles[256] = {false}; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description) {
  cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
  // GAME controls
  if (key == GLFW_KEY_F && action == GLFW_PRESS) {
    // fire once when F goes down
    if (player)
      player->shoot();
  }
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int mods) {
  // Get the current mouse position.
  double xmouse, ymouse;
  glfwGetCursorPos(window, &xmouse, &ymouse);
  // Get current window size.
  int width, height;
  glfwGetWindowSize(window, &width, &height);
  if (action == GLFW_PRESS) {
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    bool alt = (mods & GLFW_MOD_ALT) != 0;
    camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
  }
  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
  if (state == GLFW_PRESS) {
    player->shoot();
  }
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow *window, double xmouse,
                                     double ymouse) {
  // For click and look
  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  if (state == GLFW_PRESS) {
    // camera->mouseMoved((float)xmouse, (float)ymouse);
    camera->mouseMoveFreeLook((float)xmouse, (float)ymouse);
  }

  // Follow cursor
  // camera->mouseMoveFreeLook((float)xmouse, (float)ymouse);
}

static void char_callback(GLFWwindow *window, unsigned int key) {
  keyToggles[key] = !keyToggles[key];
  switch (key) {
  case 'z': {
    // zooom in
    camera->zoom(-11.0f);
    break;
  }
  case 'Z': {
    // Zoom out
    camera->zoom(11.0f);
    break;
  }
  }
}

void musicToggle() {
  if (isPlaying) {
    music.stop();
    isPlaying = !isPlaying;
  } else {
    isPlaying = !isPlaying;
    music.play();
  }
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
  ortho = glm::ortho(0.0f, (float)width, 0.0f, (float)height);
}

// This function is called once to initialize the scene and OpenGL
static void init() {
  // Initialize time.
  glfwSetTime(0.0);

  // Set background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  // Enable z-buffer test.
  glEnable(GL_DEPTH_TEST);

  createShaders(RESOURCE_DIR, programs);
  createMaterials(materials);
  createSceneObjects(objects, RESOURCE_DIR);

  text.Init(RESOURCE_DIR + "JetBrainsMonoNerdFontMono-Italic.ttf", 24,
            programs[4]);
  // For text
  shaderIndex = 0;
  materialIndex = 0;

  camera = make_shared<Camera>();
  camera->setInitDistance(2.0f); // Camera's initial Z translation

  // Frustrum
  frustrum = make_shared<Shape>();
  frustrum->loadMesh(RESOURCE_DIR + "Frustrum.obj");
  frustrum->init();

  // Game Elements
  cubeMesh = make_shared<Shape>();
  cubeMesh->loadMesh(RESOURCE_DIR + "cube.obj");
  cubeMesh->init();
  cubeMesh->setType(ShapeType::CUBE);

  sphereMesh = make_shared<Shape>();
  sphereMesh->loadMesh(RESOURCE_DIR + "sphere.obj");
  sphereMesh->init();
  sphereMesh->setType(ShapeType::SPHERE);

  bunny = make_shared<Shape>();
  bunny->loadMesh(RESOURCE_DIR + "bunny.obj");
  bunny->init();
  bunny->setType(ShapeType::BUNNY);

  wallTex = make_shared<Texture>();
  wallTex->setFilename(RESOURCE_DIR + "Dungeon_brick_wall_grey.png");
  wallTex->setUnit(0); // we'll bind it to GL_TEXTURE0
  wallTex->init();     // uploads to the GPU
  textures.push_back(wallTex);

  bulletManager = make_shared<BulletManager>(sphereMesh);
  player = make_shared<Player>(camera, bulletManager);
  std::shared_ptr<Armament> pp_919 = make_shared<Armament>(100, 100);
  player->setWeapon(pp_919); // For more ammo
  player->setPlayerPos(glm::vec3(20.0f, 40.0f, 20.0f));
  // Create structures
  initOuterAndFloors(structures, cubeMesh);
  initFloorThree(structures, cubeMesh);
  initBunnies(bunnies, bunny);
  NUM_BUNNIES = bunnies.size();

  std::shared_ptr<Light> lightSourceFloorThree = std::make_shared<Light>(
      glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(1.0f, 0.9f, 0.85f));
  std::shared_ptr<Light> lightSourceOutdoor = std::make_shared<Light>(
      glm::vec3(0.0f, 40.0f, -20.0f), glm::vec3(1.0f, 1.0f, 1.0f));
  lights.push_back(lightSourceFloorThree);
  lights.push_back(lightSourceOutdoor);

  GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render() {
  static double lastTime = 0.0; // preserved between frames
  double now = glfwGetTime();
  double deltaTime = now - lastTime;
  lastTime = now;
  // Clear framebuffer.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (keyToggles[(unsigned)'c']) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
  if (keyToggles[(unsigned)'p']) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  // Text data
  char timerBuf[32];
  if (updateTime) {
    frozenTime = glfwGetTime();
  }
  double e = frozenTime; // seconds since glfwInit
  int hours = (int)e / 3600;
  int minutes = ((int)e % 3600) / 60;
  int seconds = (int)e % 60;
  int millis = (int)((e - floor(e)) * 1000.0);
  sprintf(timerBuf, "%02d:%02d:%02d.%03d", hours, minutes, seconds, millis);

  // bunnies
  char bunniesBuf[32];
  sprintf(bunniesBuf, "Bunnies left: %d", NUM_BUNNIES);

  if (NUM_BUNNIES <= 0) {
    updateTime = false;
  }

  // armament ammunition
  char armamentBuf[48];
  auto [piercingAmmo, ricochetAmmo] = player->getArmament()->getAmmoLeft();
  sprintf(armamentBuf, "[Ammo]  Piercing: %d   Ricochet: %d", piercingAmmo,
          ricochetAmmo);

  // Get current frame buffer size.
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  camera->setAspect((float)width / (float)height);

  // Game state
  bunnyCollisions(bulletManager, bunnies, NUM_BUNNIES);
  bulletManager->update(deltaTime, structures);
  player->move(window, deltaTime, structures);

  //// DRAWING
  // Matrix stacks
  auto P = make_shared<MatrixStack>();
  auto MV = make_shared<MatrixStack>();
  auto TOPDOWNP = make_shared<MatrixStack>();
  auto TOPDOWNMV = make_shared<MatrixStack>();

  // Apply camera transforms
  P->pushMatrix();
  camera->applyProjectionMatrix(P);
  MV->pushMatrix();
  camera->applyViewMatrixFreeLook(MV);

  centerCam(MV);
  shaderIndex = 1;
  shared_ptr<Program> activeProg = programs[shaderIndex];
  shared_ptr<Material> activeMaterial = materials[materialIndex];

  // Draw text Infor at topleft
  activeProg = programs[4];
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  activeProg->bind();
  glUniformMatrix4fv(
      activeProg->getUniform("projection"), 1, GL_FALSE,
      glm::value_ptr(glm::ortho(0.0f, float(width), 0.0f, float(height))));
  glUniform1i(activeProg->getUniform("text"), 0);
  text.RenderText(timerBuf, 10.0f, height - 30.0f, 1.0f,
                  glm::vec3(1.0f, 1.0f, 1.0f), activeProg);
  text.RenderText(bunniesBuf, 10.0f, height - 60.0f, 1.0f,
                  glm::vec3(1.0f, 1.0f, 1.0f), activeProg);
  text.RenderText(armamentBuf, 10.0f, height - 90.0f, 1.0f,
                  glm::vec3(1.0f, 1.0f, 1.0f), activeProg);
  activeProg->unbind();

  activeProg = programs[1];
  activeProg->bind();
  glUniformMatrix4fv(activeProg->getUniform("P"), 1, GL_FALSE,
                     glm::value_ptr(P->topMatrix()));
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(MV->topMatrix()));
  glUniformMatrix3fv(activeProg->getUniform("T"), 1, GL_FALSE,
                     glm::value_ptr(T));
  // GRIIIDS LINESSS
  drawGridLines(activeProg, P, MV, T);
  activeProg->unbind();

  // SCENE , set prog to bling phong shader
  shaderIndex = 0;
  activeProg = programs[shaderIndex];
  activeProg->bind();
  // Set light position uniform on the active program
  // CONVERT LIGHT WORLD SPACE COORDS TO EYE SPACE COORDS
  glm::mat4 viewMatrix = MV->topMatrix();
  std::vector<glm::vec3> viewLightPositions, lightColors;
  viewLightPositions.resize(lights.size());
  lightColors.resize(lights.size());
  for (size_t i = 0; i < lights.size(); i++) {
    // Transform the world-space light position into view space.
    glm::vec4 viewPos = viewMatrix * glm::vec4(lights[i]->pos, 1.0f);
    viewLightPositions[i] = glm::vec3(viewPos);
    lightColors[i] = lights[i]->color;
  }
  float bricksPerUnit = 0.1f; // i.e. 2 bricks per 1m
  GLint ts = activeProg->getUniform("tileScale");
  glUniform1f(ts, bricksPerUnit);
  drawLevel(activeProg, P, MV, T, lights, viewLightPositions, lightColors,
            activeMaterial, materials, structures, textures, width, height,
            deltaTime);
  activeProg->unbind();

  // Bullets
  // Switch to textureless bling phong rendering
  activeProg = programs[3];
  activeMaterial = materials[1];
  activeProg->bind();

  GLint uLP = activeProg->getUniform("lightsPos");
  GLint uLC = activeProg->getUniform("lightsColor");
  GLint uP = activeProg->getUniform("P");
  GLint uM = activeProg->getUniform("MV");

  // upload the same P and MV you used for your scene objects
  glUniformMatrix4fv(uP, 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
  glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));

  // assume viewLightPositions & lightColors are std::vector<glm::vec3> of size
  glUniform3fv(uLP, lights.size(), glm::value_ptr(viewLightPositions[0]));
  glUniform3fv(uLC, lights.size(), glm::value_ptr(lightColors[0]));
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
  drawBullets(activeProg, P, MV, deltaTime, bulletManager, structures);

  activeProg->unbind();

  activeProg = programs[5];
  activeMaterial = materials[1];
  activeProg->bind();
  glUniformMatrix4fv(activeProg->getUniform("P"), 1, GL_FALSE,
                     glm::value_ptr(P->topMatrix()));
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(MV->topMatrix()));
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
  glUniform3fv(activeProg->getUniform("lightsPos"), lights.size(),
               glm::value_ptr(viewLightPositions[0]));
  glUniform3fv(activeProg->getUniform("lightsColor"), lights.size(),
               glm::value_ptr(lightColors[0]));
  drawBunnies(activeProg, P, MV, lights, viewLightPositions, lightColors,
              activeMaterial, materials, bunnies, width, height);
  activeProg->unbind();

  MV->popMatrix();
  P->popMatrix();

  drawReticle(width, height);

  GLSL::checkError(GET_FILE_LINE);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    cout << "Usage: TargetPractice RESOURCE_DIR" << endl;
    return 0;
  }
  RESOURCE_DIR = argv[1] + string("/");

  // Set error callback.
  glfwSetErrorCallback(error_callback);
  // Initialize the library.
  if (!glfwInit()) {
    return -1;
  }
  // query the primary monitor and its video mode
  GLFWmonitor *primary = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(primary);

  // make your window fullscreen on that monitor
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
  window = glfwCreateWindow(mode->width, mode->height, "TARGETPRACTICE",
                            primary, // <-- fullscreen on primary monitor
                            NULL     // <-- no shared context
  );

  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwGetFramebufferSize(window, &width, &height);
  // Make the window's context current.
  glfwMakeContextCurrent(window);
  // Initialize GLEW.
  glewExperimental = true;
  if (glewInit() != GLEW_OK) {
    cerr << "Failed to initialize GLEW" << endl;
    return -1;
  }
  glGetError(); // A bug in glewInit() causes an error that we can safely
                // ignore.
  cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
  cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
  GLSL::checkVersion();
  // Set vsync.
  glfwSwapInterval(1);
  // Set keyboard callback.
  glfwSetKeyCallback(window, key_callback);
  // Set char callback.
  glfwSetCharCallback(window, char_callback);
  // Set cursor position callback.
  glfwSetCursorPosCallback(window, cursor_position_callback);
  // Set mouse button callback.
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  // Set the window resize call back.
  glfwSetFramebufferSizeCallback(window, resize_callback);
  // Initialize scene.
  init();

  // Init music buffer
  if (!music.openFromFile(RESOURCE_DIR + "WHAT.mp3")) {
    std::cerr << "Error loading music file" << std::endl;
    return -1;
  }

  music.setLoopPoints({sf::milliseconds(0), sf::seconds(180)});
  music.setVolume(50); // Set volume (0-100)
  // Loop until the user closes the window.
  while (!glfwWindowShouldClose(window)) {
    // Render scene.
    render();
    // Swap front and back buffers.
    glfwSwapBuffers(window);
    // Poll for and process events.
    glfwPollEvents();
  }
  // Quit program.
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
