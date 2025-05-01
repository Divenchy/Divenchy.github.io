#include "Wall.h"

using std::vector, std::shared_ptr, std::make_shared, glm::vec3, glm::vec4,
    glm::mat3, glm::mat4;

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
    }
  }
  uploadInstanceBuffer();
};
