#include "Material.h"

using glm::vec3, glm::vec4, glm::mat4;

Material::Material(const vec3 &ke, const vec3 &kd, const vec3 &ks, float s) {
  this->ke = ke;
  this->kd = kd;
  this->ks = ks;
  this->s = s;
}

vec3 Material::getMaterialKE() { return this->ke; }
vec3 Material::getMaterialKD() { return this->kd; }
vec3 Material::getMaterialKS() { return this->ks; }
float Material::getMaterialS() { return this->s; }
