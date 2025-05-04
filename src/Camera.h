#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <memory>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

class MatrixStack;

class Camera {
public:
  enum { ROTATE = 0, TRANSLATE, SCALE };

  Camera();
  virtual ~Camera();
  void setInitDistance(float z) { translations.z = -std::abs(z); }
  void setAspect(float a) { aspect = a; };
  void setRotationFactor(float f) { rfactor = f; };
  void setTranslationFactor(float f) { tfactor = f; };
  void setScaleFactor(float f) { sfactor = f; };
  void mouseClicked(float x, float y, bool shift, bool ctrl, bool alt);
  void mouseMoved(float x, float y);
  void applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const;
  // Edited
  void applyViewMatrix(std::shared_ptr<MatrixStack> MV) const;
  void applyViewMatrixFreeLook(std::shared_ptr<MatrixStack> MV) const;
  // New
  glm::mat4 getViewMatrixFreeLook();
  void mouseMoveFreeLook(float dx, float dy);
  void keyInput(char key, float deltaTime);
  void zoom(float factor);
  glm::vec3 getPosition() { return this->position; };
  void setPosition(glm::vec3 &pos) { this->position = pos; };
  glm::vec3 getForward() { return this->forward; };
  void setPreviousMouse(const glm::vec2 &p) { mousePrev = p; }
  void processMouseMovement(float dx, float dy);

private:
  float aspect;
  float fovy;
  float znear;
  float zfar;
  glm::vec2 rotations;
  glm::vec3 translations;
  glm::vec2 mousePrev;
  int state;
  float rfactor;
  float tfactor;
  float sfactor;

  mutable glm::mat4 viewFreeLook = glm::mat4(1.0f);

  // New members
  glm::vec3 position;
  glm::vec3 forward;
  float yaw, pitch; // In rads
};

#endif
