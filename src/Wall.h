#pragma once

#include "Structure.h"
class Wall : public Structure {
private:
  int width; // in meters
  int height;

public:
  Wall(std::shared_ptr<Shape> cubeMesh, int width, int height, glm::vec3 center)
      : Structure(cubeMesh) {
    createStructure(cubeMesh, width, height, center);
  };
  ~Wall() = default;
  virtual void createStructure(std::shared_ptr<Shape> cubeMesh, int width,
                               int height, glm::vec3 center) override;
};
