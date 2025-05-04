#pragma once

#include "Particle.h"
#include "Spring.h"
#include "Structure.h"

class Platform : public Structure {
private:
  int width; // in meters
  int length;

public:
  Platform(std::shared_ptr<Shape> cubeMesh, int width, int length,
           glm::vec3 center)
      : Structure(cubeMesh), width(width), length(length) {
    createStructure(cubeMesh, width, length, center);
  };
  ~Platform() = default;
  virtual void createStructure(std::shared_ptr<Shape> cubeMesh, int width,
                               int height, glm::vec3 center) override;
};
