#include <memory>
#define _USE_MATH_DEFINES
#include "Camera.h"
#include "MatrixStack.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using glm::vec3, glm::vec4, glm::mat4;

Camera::Camera()
    : aspect(1.0f), fovy((float)(45.0 * M_PI / 180.0)), znear(0.1f),
      zfar(1000.0f), rotations(0.0, 0.0), translations(0.0f, 0.0f, -5.0f),
      rfactor(0.01f), tfactor(0.001f), sfactor(0.005f),
      position(0.0f, 0.0f, 5.0f), yaw(glm::radians(90.0f)),
      pitch(glm::radians(0.0f)) {
  this->forward = glm::normalize(
      glm::vec3(cos(pitch) * sin(yaw),
                sin(pitch), // Note: For translation, we ignore pitch
                            // so that movement stays on the ground.
                cos(pitch) * cos(yaw)));

} // Yaw at 90 to face -Z

Camera::~Camera() {}

void Camera::mouseClicked(float x, float y, bool shift, bool ctrl, bool alt) {
  mousePrev.x = x;
  mousePrev.y = y;
  if (shift) {
    state = Camera::TRANSLATE;
  } else if (ctrl) {
    state = Camera::SCALE;
  } else {
    state = Camera::ROTATE;
  }
}

void Camera::mouseMoved(float x, float y) {
  glm::vec2 mouseCurr(x, y);
  glm::vec2 dv = mouseCurr - mousePrev;
  switch (state) {
  case Camera::ROTATE:
    rotations += rfactor * dv;
    break;
  case Camera::TRANSLATE:
    translations.x -= translations.z * tfactor * dv.x;
    translations.y += translations.z * tfactor * dv.y;
    break;
  case Camera::SCALE:
    translations.z *= (1.0f - sfactor * dv.y);
    break;
  }
  mousePrev = mouseCurr;
}

void Camera::applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const {
  // Modify provided MatrixStack
  P->multMatrix(glm::perspective(fovy, aspect, znear, zfar));
}

void Camera::applyViewMatrix(std::shared_ptr<MatrixStack> MV) const {
  MV->translate(translations);
  MV->rotate(rotations.y, glm::vec3(1.0f, 0.0f, 0.0f));
  MV->rotate(rotations.x, glm::vec3(0.0f, 1.0f, 0.0f));
}

// Help with ChatGPT
void Camera::applyViewMatrixFreeLook(std::shared_ptr<MatrixStack> MV) const {
  vec3 forward = glm::normalize(
      glm::vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw)));
  vec3 up = glm::vec3(0, 1, 0);
  mat4 view = glm::lookAt(position, position + forward, up);
  viewFreeLook = view;
  MV->multMatrix(view);
}

// Make getter for view matrix
mat4 Camera::getViewMatrixFreeLook() { return viewFreeLook; }

void Camera::mouseMoveFreeLook(float x, float y) {
  glm::vec2 mouseCurr(x, y);
  float dx = mouseCurr.x - mousePrev.x;
  float dy = mouseCurr.y - mousePrev.y;
  float sensitivity = 0.005f;
  yaw += dx * sensitivity;
  pitch += dy * sensitivity;

  // Clamp pitch to avoid wonky wonk
  float pitchLimit = glm::radians(60.0f);
  pitch = std::max(-pitchLimit, std::min(pitch, pitchLimit));
  this->forward = glm::normalize(
      glm::vec3(cos(pitch) * sin(yaw),
                sin(pitch), // Note: For translation, we ignore pitch
                            // so that movement stays on the ground.
                cos(pitch) * cos(yaw)));
  mousePrev = mouseCurr;
}

void Camera::keyInput(char key, float deltaTime) {
  float speed = 0.3f; // Adjust speed as needed.
  // Compute the forward direction from yaw and pitch (for viewing)
  glm::vec3 forward =
      glm::normalize(glm::vec3(cos(pitch) * sin(yaw),
                               0.0f, // Note: For translation, we ignore pitch
                                     // so that movement stays on the ground.
                               cos(pitch) * cos(yaw)));
  // Compute right direction.
  glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

  if (key == 'w' || key == 'W') {
    position += forward * speed;
  }
  if (key == 's' || key == 'S') {
    position -= forward * speed;
  }
  if (key == 'a' || key == 'A') {
    position -= right * speed;
  }
  if (key == 'd' || key == 'D') {
    position += right * speed;
  }
}

// Help with ChatGPT
void Camera::zoom(float degree) {
  std::cout << "FOV" << fovy << " degrees." << std::endl;
  fovy += glm::radians(degree);
  // Clamp fovy between roughly 4° and 114°.
  float minFovy = glm::radians(4.0f);
  float maxFovy = glm::radians(114.0f);
  if (fovy < minFovy)
    fovy = minFovy;
  if (fovy > maxFovy)
    fovy = maxFovy;
}
