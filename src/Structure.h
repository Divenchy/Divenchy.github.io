#pragma once

#include "Eigen/src/Core/Matrix.h"
#include "GLM_EIGEN_COMPATIBILITY_LAYER.h"
#include "Program.h"
#include "Shape.h"
#include <cassert>
struct FreeCube {
  Eigen::Vector3d position;
  Eigen::Vector3d velocity;
  float size;
};

class Structure {
private:
  std::shared_ptr<Shape> cubeMesh;
  std::vector<glm::mat4> modelMatsStatic;
  std::vector<FreeCube> freeCubes; // Cubes that are now fractured
  GLuint instanceVBO;              // buffer for instance mats
  // AKA origin of structure
  glm::vec3 center;
  std::vector<std::shared_ptr<Particle>> particles;
  std::vector<std::shared_ptr<Spring>> springs;

public:
  Structure(std::shared_ptr<Shape> cubeMesh) : cubeMesh(cubeMesh) {
    // Ensure shape passed is indeed a cube, if not reject
    assert(cubeMesh->getType() == ShapeType::CUBE);
    // If is cube, generate instanceVBO
    glGenBuffers(1, &instanceVBO);
  };
  virtual ~Structure() = default;

  virtual void createStructure(std::shared_ptr<Shape> cubeMesh, int width,
                               int height, glm::vec3 center) = 0;

  std::shared_ptr<Shape> getMesh() { return this->cubeMesh; };

  std::vector<FreeCube> getFreeCubes() { return this->freeCubes; };

  std::vector<std::shared_ptr<Particle>> getParticleArray() {
    return this->particles;
  };

  std::vector<std::shared_ptr<Spring>> getSpringsArray() {
    return this->springs;
  };

  void pushToParticles(std::shared_ptr<Particle> p) {
    this->particles.push_back(p);
  };
  void pushToSprings(std::shared_ptr<Spring> s) { this->springs.push_back(s); };

  std::shared_ptr<Particle> getParticleAtIdx(int idx) {
    return this->particles.at(idx);
  }

  // Once done filling modeMatsStatic, upload to instace buffer, and everytime
  // structure changes
  void uploadInstanceBuffer() {
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, modelMatsStatic.size() * sizeof(glm::mat4),
                 modelMatsStatic.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  void renderStructure(const std::shared_ptr<Program> prog) {
    glBindVertexArray(cubeMesh->getVAO());

    // Bind vetex attribs (aPos and aNor)
    // aPos
    int posLoc = prog->getAttribute("aPos");
    glEnableVertexAttribArray(posLoc);
    glBindBuffer(GL_ARRAY_BUFFER, cubeMesh->getPosBufID());
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    // aNor
    int norLoc = prog->getAttribute("aNor");
    if (norLoc != -1) {
      glEnableVertexAttribArray(norLoc);
      glBindBuffer(GL_ARRAY_BUFFER, cubeMesh->getNorBufID());
      glVertexAttribPointer(norLoc, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    }

    // Bind and re-upload instance data (if it’s changed)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    modelMatsStatic.size() * sizeof(glm::mat4),
                    modelMatsStatic.data());

    // Tell OpenGL how to interpret that buffer as 4 vec4 attributes:
    //    suppose you reserve locations 4,5,6,7 for your mat4
    std::size_t vec4Size = sizeof(glm::vec4);
    int matLoc[4];
    matLoc[0] = prog->getAttribute("aInstMat0");
    matLoc[1] = prog->getAttribute("aInstMat1");
    matLoc[2] = prog->getAttribute("aInstMat2");
    matLoc[3] = prog->getAttribute("aInstMat3");
    for (int i = 0; i < 4; ++i) {
      if (matLoc[i] < 0)
        continue; // attribute wasn’t actually active
      glEnableVertexAttribArray(matLoc[i]);
      glVertexAttribPointer(matLoc[i], // use the real location
                            4, GL_FLOAT, GL_FALSE,
                            sizeof(glm::mat4),     // 64-byte stride
                            (void *)(i * vec4Size) // offset 0,16,32,48 bytes
      );
      glVertexAttribDivisor(matLoc[i], 1);
    }

    // Finally draw instanced
    GLsizei instanceCount = (GLsizei)modelMatsStatic.size();
    glDrawArraysInstanced(GL_TRIANGLES, 0, cubeMesh->getVertexCount(),
                          instanceCount);

    // Cleanup
    for (int i = 0; i < 4; ++i) {
      if (matLoc[i] >= 0) {
        glDisableVertexAttribArray(matLoc[i]);
      }
    }
    glDisableVertexAttribArray(posLoc);
    if (norLoc >= 0) {
      glDisableVertexAttribArray(norLoc);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  };

  std::vector<int> collisionSphere(const glm::vec3 &center, float radius) {

    std::vector<int> hits;
    for (size_t k = 0; k < modelMatsStatic.size(); ++k) {
      glm::vec3 cubePos = glm::vec3(modelMatsStatic[k][3]);
      float halfSize = 0.5f; // or whatever your cube’s “radius” is
      if (glm::length(center - cubePos) < (radius + halfSize)) {
        hits.push_back((int)k);
      }
    }
    return hits;
  };

  // Fracture cube, add to freeCubes
  void fracturedCube(int k) {
    // mark the particle free
    particles[k]->fixed = false;
    // remove all springs attached to that particle
    springs.erase(remove_if(begin(springs), end(springs),
                            [&](auto &s) {
                              return s->p0 == particles[k] ||
                                     s->p1 == particles[k];
                            }),
                  end(springs));
    // move its model matrix into freeCubes for separate physics
    FreeCube cc;
    cc.position = glmVec3ToEigen(glm::vec3(modelMatsStatic[k][3]));
    cc.velocity = glmVec3ToEigen(glm::vec3(0));
    cc.size = 1.0f;
    freeCubes.push_back(cc);
    // erase from instanced list
    modelMatsStatic.erase(modelMatsStatic.begin() + k);
    uploadInstanceBuffer();
  };

  // GETTERS and SETTERS

  GLuint getInstanceVBO() { return this->instanceVBO; };
  // Append to modelMatsStatic
  void pushBackModelMat(glm::mat4 mat) { modelMatsStatic.push_back(mat); };
};
