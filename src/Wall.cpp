#include "Wall.h"
#include "Eigen/src/Core/Matrix.h"
#include "GLM_EIGEN_COMPATIBILITY_LAYER.h"

using std::vector, std::shared_ptr, std::make_shared, glm::vec3, glm::vec4,
    glm::mat3, glm::mat4;

static glm::mat4 rotationAboutPoint(const glm::vec3 &center, float angle) {
  glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -center);
  glm::mat4 R = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
  glm::mat4 T2 = glm::translate(glm::mat4(1.0f), center);
  return T2 * R * T1;
}

void Wall::createStructure(std::shared_ptr<Shape> cubeMesh, int width,
                           int height, glm::vec3 center) {
  float bottom = cubeMesh->getMinY();
  float top = -cubeMesh->getMinY();
  float realHeight = top - bottom;
  float liftY = -bottom;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      glm::vec3 pos = center + glm::vec3(x * 1.0f, y * 1.0f + liftY, 0.0f);
      glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
      pushBackModelMat(model);
      auto p = make_shared<Particle>(cubeMesh);
      p->x = glmVec3ToEigen(glm::vec3(model[3]));
      p->fixed = true; // initially locked
      pushToParticles(p);
    }
  }

  // Connect the cubes with springs

  // horizontal neighbors
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width - 1; ++x) {
      int idx0 = y * width + x;
      int idx1 = idx0 + 1;
      pushToSprings(make_shared<Spring>(getParticleAtIdx(idx0),
                                        getParticleAtIdx(idx1), ALPHA));
    }
  }
  // vertical neighbors
  for (int y = 0; y < height - 1; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx0 = y * width + x;
      int idx1 = idx0 + width;
      pushToSprings(make_shared<Spring>(getParticleAtIdx(idx0),
                                        getParticleAtIdx(idx1), ALPHA));
    }
  }
  uploadInstanceBuffer();
};

void Wall::createStructure(std::shared_ptr<Shape> cubeMesh, int width,
                           int height, glm::vec3 center, float angle) {
  float bottom = cubeMesh->getMinY();
  float top = -cubeMesh->getMinY();
  float realHeight = top - bottom;
  float liftY = -bottom;
  glm::mat4 rot = rotationAboutPoint(center, glm::radians(angle));
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      glm::vec3 pos = center + glm::vec3(x * 1.0f, y * 1.0f + liftY, 0.0f);
      glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
      model = rot * model;
      pushBackModelMat(model);
      auto p = make_shared<Particle>(cubeMesh);
      p->x = glmVec3ToEigen(glm::vec3(model[3]));
      p->fixed = true; // initially locked
      pushToParticles(p);
    }
  }

  // Connect the cubes with springs

  // horizontal neighbors
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width - 1; ++x) {
      int idx0 = y * width + x;
      int idx1 = idx0 + 1;
      pushToSprings(make_shared<Spring>(getParticleAtIdx(idx0),
                                        getParticleAtIdx(idx1), ALPHA));
    }
  }
  // vertical neighbors
  for (int y = 0; y < height - 1; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx0 = y * width + x;
      int idx1 = idx0 + width;
      pushToSprings(make_shared<Spring>(getParticleAtIdx(idx0),
                                        getParticleAtIdx(idx1), ALPHA));
    }
  }
  uploadInstanceBuffer();
};
