#include "glm/matrix.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Armament.h"
#include "stb_image_write.h"

class Player {
private:
  int health;
  float speed;
  Armament peaShooter;

public:
  Player() : health(100), speed(1.0f) {} // Default constructor
};
