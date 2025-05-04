#include "Platform.h"
#include "Eigen/src/Core/Matrix.h"
#include "GLM_EIGEN_COMPATIBILITY_LAYER.h"

using std::shared_ptr, std::make_shared, glm::vec3, glm::vec4, glm::mat3,
    glm::mat4;

void Platform::createStructure(std::shared_ptr<Shape> cubeMesh, int width,
                               int height, glm::vec3 center) {

  // compute vertical offset so cubes sit on top of center.y
  float bottom = cubeMesh->getMinY();
  float liftY = -bottom;

  // build a width × length grid at y = liftY
  for (int z = 0; z < length; ++z) {
    for (int x = 0; x < width; ++x) {
      vec3 pos = center + vec3(x * 1.0f, // assuming getSize()==1.0f
                               liftY, z * 1.0f);
      mat4 model = glm::translate(mat4(1.0f), pos);

      // record instance matrix
      pushBackModelMat(model);

      // make a locked particle at that position
      auto p = make_shared<Particle>(cubeMesh);
      p->x = glmVec3ToEigen(vec3(model[3]));
      p->fixed = true;
      pushToParticles(p);
    }
  }

  // connect springs along +X
  for (int z = 0; z < length; ++z) {
    for (int x = 0; x < width - 1; ++x) {
      int idx0 = z * width + x;
      int idx1 = idx0 + 1;
      pushToSprings(make_shared<Spring>(getParticleAtIdx(idx0),
                                        getParticleAtIdx(idx1), ALPHA));
    }
  }
  // connect springs along +Z
  for (int z = 0; z < length - 1; ++z) {
    for (int x = 0; x < width; ++x) {
      int idx0 = z * width + x;
      int idx1 = idx0 + width;
      pushToSprings(make_shared<Spring>(getParticleAtIdx(idx0),
                                        getParticleAtIdx(idx1), ALPHA));
    }
  }

  // finally upload all the instance matrices in one go
  uploadInstanceBuffer();
};
