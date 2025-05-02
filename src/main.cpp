#include "glm/matrix.hpp"

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

GLFWwindow *window;         // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
int TASK = 1;
bool OFFLINE = false;

float oldDeltaTime;
float oldFrameTime = 0.0f;

// For shear
glm::mat4 S(1.0f);
// clang-format off
glm::mat4 T(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), 
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), 
            glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
            glm::vec4(10.0f, 10.0f, 0.0f, 1.0f));
// clang-format on

// AUDIO
sf::Music music;
bool isPlaying;

shared_ptr<Player> player;
shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Shape> shape;
shared_ptr<Shape> frustrum;

// Shapes
shared_ptr<Shape> cubeMesh;
shared_ptr<Shape> sphereMesh;
shared_ptr<BulletManager> bulletManager;
std::vector<shared_ptr<Structure>> structures;
shared_ptr<Structure> wall;
shared_ptr<Structure> wallTwo;

// HUD
shared_ptr<Shape> hudBunny;
shared_ptr<Shape> hudTeapot;

// Objects vector
std::vector<std::shared_ptr<Object>> objects;

// Lights, and Material stacks
std::vector<shared_ptr<Program>> programs;
std::vector<shared_ptr<Material>> materials;
std::vector<shared_ptr<Light>> lights;

std::shared_ptr<Light> lightSource = std::make_shared<Light>(
    glm::vec3(0.0f, 20.0f, -30.0f), glm::vec3(1.0f, 1.0f, 1.0f));
std::shared_ptr<Light> lightSourceTwo = std::make_shared<Light>(
    glm::vec3(-15.0f, 20.0f, -15.0f), glm::vec3(1.0f, 1.0f, 1.0f));

// Shaders
glm::vec3 ka;
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
  case 't': {
    enableTopDown = !enableTopDown;
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
}

// This function is called once to initialize the scene and OpenGL
static void init() {
  // Initialize time.
  glfwSetTime(0.0);
  oldFrameTime = float(glfwGetTime());

  // Set background color.
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  // Enable z-buffer test.
  glEnable(GL_DEPTH_TEST);

  createShaders(RESOURCE_DIR, programs);
  createMaterials(materials);

  shaderIndex = 0;
  materialIndex = 0;

  camera = make_shared<Camera>();
  camera->setInitDistance(2.0f); // Camera's initial Z translation

  // Frustrum
  frustrum = make_shared<Shape>();
  frustrum->loadMesh(RESOURCE_DIR + "Frustrum.obj");
  frustrum->init();

  // Create HUD elems
  hudBunny = make_shared<Shape>();
  hudBunny->loadMesh(RESOURCE_DIR + "bunny.obj");
  hudBunny->init();

  hudTeapot = make_shared<Shape>();
  hudTeapot->loadMesh(RESOURCE_DIR + "teapot.obj");
  hudTeapot->init();

  // Game Elements
  cubeMesh = make_shared<Shape>();
  cubeMesh->loadMesh(RESOURCE_DIR + "cube.obj");
  cubeMesh->init();
  cubeMesh->setType(ShapeType::CUBE);

  sphereMesh = make_shared<Shape>();
  sphereMesh->loadMesh(RESOURCE_DIR + "sphere.obj");
  sphereMesh->init();
  sphereMesh->setType(ShapeType::SPHERE);

  bulletManager = make_shared<BulletManager>(sphereMesh);
  player = make_shared<Player>(camera, bulletManager);
  std::shared_ptr<Armament> pp_919 = make_shared<Armament>(50, 50);
  player->setWeapon(pp_919); // For more ammo
  // Create structure
  wall = make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(3.0f, 0.0f, 0.0f));
  wallTwo = make_shared<Wall>(cubeMesh, 7, 5, glm::vec3(0.0f, 0.0f, 0.0f));
  structures.push_back(wall);
  structures.push_back(wallTwo);
  lights.push_back(lightSource);
  lights.push_back(lightSourceTwo);

  GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render() {
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

  // Get current frame buffer size.
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  camera->setAspect((float)width / (float)height);

  double t = glfwGetTime();
  if (!keyToggles[(unsigned)' ']) {
    // Spacebar turns animation on/off
    t = 0.0f;
    musicToggle();
  }

  float deltaTime = (float)glfwGetTime() - oldDeltaTime;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera->keyInput('w', deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera->keyInput('s', deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera->keyInput('a', deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera->keyInput('d', deltaTime);

  // Matrix stacks
  auto P = make_shared<MatrixStack>();
  auto MV = make_shared<MatrixStack>();
  auto HUDP = make_shared<MatrixStack>();
  auto HUDMV = make_shared<MatrixStack>();
  auto TOPDOWNP = make_shared<MatrixStack>();
  auto TOPDOWNMV = make_shared<MatrixStack>();

  // Apply camera transforms
  P->pushMatrix();
  camera->applyProjectionMatrix(P);
  MV->pushMatrix();
  camera->applyViewMatrixFreeLook(MV);

  centerCam(MV);

  glm::vec4 lightPosCamSpace =
      MV->topMatrix() * glm::vec4(lightSource->pos, 1.0f);

  shaderIndex = 3;
  shared_ptr<Program> activeProg = programs[shaderIndex];
  shared_ptr<Material> activeMaterial = materials[materialIndex];

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
  drawLevel(activeProg, P, MV, lights, activeMaterial, materials, structures);
  activeProg->unbind();

  // Bullets
  activeProg->bind();
  drawBullets(activeProg, P, MV, oldFrameTime, bulletManager, structures);
  activeProg->unbind();

  MV->popMatrix();
  P->popMatrix();

  // HUD Rendering (Help from ChatGPT)
  activeProg = programs[2];
  activeProg->bind();
  drawHUD(window, width, height, activeProg, HUDP, HUDMV, lights,
          lightPosCamSpace, activeMaterial, materials, hudBunny, hudTeapot);
  activeProg->unbind();

  // Top down view
  shaderIndex = 3;
  activeProg = programs[shaderIndex];
  if (enableTopDown) {
    // Create second viewport
    activeProg->bind();
    double s = 0.5;
    glViewport(0, 0, s * width, s * height);

    // Clear a region for new view
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, s * width, s * height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    float topDownSize = 15.0f;

    // Setup projection and camera
    TOPDOWNP->pushMatrix();
    TOPDOWNP->multMatrix(glm::ortho(-topDownSize, topDownSize, -topDownSize,
                                    topDownSize, 0.1f, 100.0f));
    TOPDOWNMV->pushMatrix();
    TOPDOWNMV->multMatrix(glm::lookAt(
        glm::vec3(0, 20, 0), // camera position: 30 units above the origin
        glm::vec3(0, 0, 0),  // look at the center of the scene
        glm::vec3(0, 0,
                  -1) // up vector: chosen to orient the view appropriately
        ));

    // Load P and ModelView onto GPU
    T = glm::mat4(1.0f);
    glUniformMatrix4fv(activeProg->getUniform("P"), 1, GL_FALSE,
                       glm::value_ptr(TOPDOWNP->topMatrix()));
    glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                       glm::value_ptr(TOPDOWNMV->topMatrix()));
    glUniformMatrix3fv(activeProg->getUniform("T"), 1, GL_FALSE,
                       glm::value_ptr(T));

    // Compute the light position in top-down coordinates:
    glm::vec4 topDownLightPos =
        TOPDOWNMV->topMatrix() * glm::vec4(lightSource->pos, 1.0f);
    // glUniform3f(activeProg->getUniform("lightPos"), topDownLightPos.x,
    // topDownLightPos.y, topDownLightPos.z);
    glUniform3fv(activeProg->getUniform("lightPos"), 1,
                 glm::value_ptr(topDownLightPos));
    glUniform1i(activeProg->getUniform("useCloudTexture"), 0);

    drawGrid(activeProg, TOPDOWNP, TOPDOWNMV);
    activeProg->unbind();

    activeProg = programs[0];
    // Draw frustrum
    drawFrustrum(activeProg, camera, TOPDOWNP, TOPDOWNMV, frustrum, width,
                 height);

    TOPDOWNMV->popMatrix();
    TOPDOWNP->popMatrix();
  }

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
  // Create a windowed mode window and its OpenGL context.
  window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
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
