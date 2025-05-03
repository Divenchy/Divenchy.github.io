#pragma once
#include <Eigen/Dense>

inline Eigen::Vector3d glmVec3ToEigen(const glm::vec3 &v) {

  Eigen::Vector3d eigenVec3(v.x, v.y, v.z);
  return eigenVec3;
}
