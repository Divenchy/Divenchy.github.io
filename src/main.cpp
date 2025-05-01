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
// clang-format on

using namespace std;

GLFWwindow *window;         // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
int TASK = 1;
bool OFFLINE = false;

// Camera vars, for potential speed increase as its held
float deltaTime;
float oldDeltaTime = 0;

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

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Shape> shape;
shared_ptr<Shape> frustrum;

// Shapes
shared_ptr<Shape> bullet;
shared_ptr<Shape> cubeMesh;
shared_ptr<Structure> wall;

// HUD
shared_ptr<Shape> hudBunny;
shared_ptr<Shape> hudTeapot;

// Objects vector
std::vector<std::shared_ptr<Object>> objects;

// Shaders, Lights, and Material stacks
std::vector<shared_ptr<Program>> programs;
std::vector<shared_ptr<Material>> materials;
std::vector<shared_ptr<Light>> lights;

std::shared_ptr<Light> lightSource = std::make_shared<Light>(
    glm::vec3(0.0f, 20.0f, -30.0f), glm::vec3(1.0f, 1.0f, 1.0f));

glm::vec3 ka;
glm::vec3 kd;
glm::vec3 ks;
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

  bullet = make_shared<Shape>();
  bullet->loadMesh(RESOURCE_DIR + "sphere.obj");
  bullet->init();

  // Create structure
  wall = make_shared<Wall>(cubeMesh, 5, 5, glm::vec3(0.0f, 0.0f, 0.0f));

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

  if (!keyToggles[(unsigned)'f']) {
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
  glUniform3fv(activeProg->getUniform("lightPos"), 1,
               glm::value_ptr(lightPosCamSpace));
  glUniform1i(activeProg->getUniform("useCloudTexture"), 0);

  drawGrid(activeProg, P, MV);

  bullet->draw(activeProg);
  activeProg->unbind();

  shaderIndex = 0;
  activeProg = programs[shaderIndex];
  activeProg->bind();
  // Back to original shader
  glUniformMatrix4fv(activeProg->getUniform("P"), 1, GL_FALSE,
                     glm::value_ptr(P->topMatrix()));
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(MV->topMatrix()));

  // Set light position uniform on the active program
  glUniform3f(activeProg->getUniform("lightPos"), lightPosCamSpace.x,
              lightPosCamSpace.y, lightPosCamSpace.z);

  // Set material uniforms from activeMaterial
  glUniform3f(activeProg->getUniform("ka"), activeMaterial->getMaterialKA().x,
              activeMaterial->getMaterialKA().y,
              activeMaterial->getMaterialKA().z);
  glUniform3f(activeProg->getUniform("kd"), activeMaterial->getMaterialKD().x,
              activeMaterial->getMaterialKD().y,
              activeMaterial->getMaterialKD().z);
  glUniform3f(activeProg->getUniform("ks"), activeMaterial->getMaterialKS().x,
              activeMaterial->getMaterialKS().y,
              activeMaterial->getMaterialKS().z);
  glUniform1f(activeProg->getUniform("s"), activeMaterial->getMaterialS());
  wall->renderStructure(activeProg);

  activeProg->unbind();

  MV->popMatrix();
  P->popMatrix();

  // HUD Rendering (Help from ChatGPT)
  HUDP->pushMatrix();
  float fovy = glm::radians(45.0f);
  HUDP->multMatrix(glm::perspective(fovy, (float)width / (float)height, 0.1f,
                                    100.0f)); // Projection
  HUDMV->pushMatrix();
  HUDMV->multMatrix(glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0),
                                glm::vec3(0, 1, 0))); // ModelView

  activeProg = programs[2];
  // --- Begin HUD Rendering ---
  activeProg->bind();
  glUniformMatrix4fv(activeProg->getUniform("P"), 1, GL_FALSE,
                     glm::value_ptr(HUDP->topMatrix()));
  glUniformMatrix4fv(activeProg->getUniform("MV"), 1, GL_FALSE,
                     glm::value_ptr(HUDMV->topMatrix()));

  // Set light position uniform on the active program
  glUniform3f(activeProg->getUniform("lightPos"), lightPosCamSpace.x,
              lightPosCamSpace.y, lightPosCamSpace.z);

  // Set material uniforms from activeMaterial
  glUniform3f(activeProg->getUniform("ka"), activeMaterial->getMaterialKA().x,
              activeMaterial->getMaterialKA().y,
              activeMaterial->getMaterialKA().z);
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
                     (&HUDMV->topMatrix()[0][0]));
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
                     (&HUDMV->topMatrix()[0][0]));
  hudTeapot->draw(activeProg);
  HUDMV->popMatrix();

  // Re-enable depth testing after HUD pass.
  glEnable(GL_DEPTH_TEST);

  HUDMV->popMatrix();
  HUDP->popMatrix();
  activeProg->unbind();
  // --- End HUD Rendering ---

  shaderIndex = 3;
  activeProg = programs[shaderIndex];
  // Top down view
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
