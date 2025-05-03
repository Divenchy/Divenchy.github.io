#pragma once

#include <Eigen/Dense>

#define ALPHA 0e-1
#define STEPS_H 1e-3
#define PARTICLE_MASS 0.1
#define DAMP 1e-5
#define PARTICLE_RAD 0.01
#define GRAV_VEC Eigen::Vector3d(0.0, -9.8, 0.0)

// GLM common vectors
#define GLM_AXIS_X glm::vec3(1.0f, 0.0f, 0.0f)
#define GLM_AXIS_Y glm::vec3(0.0f, 1.0f, 0.0f)
#define GLM_AXIS_Z glm::vec3(0.0f, 0.0f, 1.0f)
