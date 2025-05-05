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
  GLuint debrisVBO = 0;
  // AKA origin of structure
  glm::vec3 center;
  std::vector<std::shared_ptr<Particle>> particles;
  std::vector<std::shared_ptr<Spring>> springs;
  bool fracturable = true;

  // For transforms
  glm::mat4 worldXform = glm::mat4(1.0f);

public:
  Structure(std::shared_ptr<Shape> cubeMesh) : cubeMesh(cubeMesh) {
    // Ensure shape passed is indeed a cube, if not reject
    assert(cubeMesh->getType() == ShapeType::CUBE);
    // If is cube, generate instanceVBO
    glGenBuffers(1, &instanceVBO);
    glGenBuffers(1, &debrisVBO);
  };
  ~Structure() {
    if (instanceVBO)
      glDeleteBuffers(1, &instanceVBO);
    if (debrisVBO)
      glDeleteBuffers(1, &debrisVBO);
  }

  virtual void createStructure(std::shared_ptr<Shape> cubeMesh, int width,
                               int height, glm::vec3 center) = 0;

  /// Apply a world‐space rotation to _every_ cube, particle & freeCube.
  void rotate(float angle, const glm::vec3 &axis) {
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), angle, axis);

    // 1) rotate all the instance matrices
    for (auto &M : modelMatsStatic) {
      M = R * M;
    }

    // 2) rotate any “free” cubes
    for (auto &fc : freeCubes) {
      glm::vec4 p(fc.position.x(), fc.position.y(), fc.position.z(), 1.0f);
      p = R * p;
      fc.position = glmVec3ToEigen(glm::vec3(p));

      glm::vec4 v(fc.velocity.x(), fc.velocity.y(), fc.velocity.z(), 0.0f);
      v = R * v;
      fc.velocity = glmVec3ToEigen(glm::vec3(v));
    }

    // 3) rotate your constraint‐solver particles too
    for (auto &p : particles) {
      glm::vec4 x(p->x(0), p->x(1), p->x(2), 1.0f);
      x = R * x;
      p->x = glmVec3ToEigen(glm::vec3(x));
      // if you care about their velocities:
      glm::vec4 vel(p->v(0), p->v(1), p->v(2), 0.0f);
      vel = R * vel;
      p->v = glmVec3ToEigen(glm::vec3(vel));
    }

    // 4) re‐upload your instance VBO to reflect the new modelMatsStatic
    uploadInstanceBuffer();
  };

  void setModelMatAtIdx(int idx, glm::mat4 &M) {
    this->modelMatsStatic[idx] = M;
  };
  std::vector<glm::mat4> getModelMatsStatic() { return this->modelMatsStatic; };
  std::shared_ptr<Shape> getMesh() { return this->cubeMesh; };

  std::vector<FreeCube> &getFreeCubes() { return this->freeCubes; };

  bool getFracturable() { return this->fracturable; };
  void setFracturable(bool isFrac) { this->fracturable = isFrac; };

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

  void updateDebris(float dt) {
    for (auto &d : freeCubes) {
      // unpack
      glm::vec3 vel{(float)d.velocity.x(), (float)d.velocity.y(),
                    (float)d.velocity.z()};
      glm::vec3 pos{(float)d.position.x(), (float)d.position.y(),
                    (float)d.position.z()};

      // gravity
      vel += glm::vec3(0.0f, -9.8f, 0.0f) * dt;
      pos += vel * dt;

      // simple ground bounce
      if (pos.y < 0.0f) {
        pos.y = 0.0f;
        vel.y *= -0.4f;
      }

      // pack back
      d.velocity = Eigen::Vector3d(vel.x, vel.y, vel.z);
      d.position = Eigen::Vector3d(pos.x, pos.y, pos.z);
    }
  }

  void renderDebris(const std::shared_ptr<Program> &prog) {
    if (freeCubes.empty())
      return;

    // 1) build a temporary array of glm::mat4's from your FreeCube data:
    std::vector<glm::mat4> debrisMats;
    debrisMats.reserve(freeCubes.size());
    for (auto &fc : freeCubes) {
      glm::mat4 M = glm::translate(glm::mat4(1.0f),
                                   glm::vec3(fc.position.x(), fc.position.y(),
                                             fc.position.z())) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(fc.size));
      debrisMats.push_back(M);
    }

    // 2) bind your cube VAO and per‐vertex attribs exactly like
    // renderStructure:
    glBindVertexArray(cubeMesh->getVAO());
    int posLoc = prog->getAttribute("aPos");
    glEnableVertexAttribArray(posLoc);
    glBindBuffer(GL_ARRAY_BUFFER, cubeMesh->getPosBufID());
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    int norLoc = prog->getAttribute("aNor");
    if (norLoc >= 0) {
      glEnableVertexAttribArray(norLoc);
      glBindBuffer(GL_ARRAY_BUFFER, cubeMesh->getNorBufID());
      glVertexAttribPointer(norLoc, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    }

    int texLoc = prog->getAttribute("aTex");
    if (texLoc >= 0 && cubeMesh->getTexBufID() != 0) {
      glEnableVertexAttribArray(texLoc);
      glBindBuffer(GL_ARRAY_BUFFER, cubeMesh->getTexBufID());
      glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    }

    // 3) upload debrisMats into debrisVBO
    glBindBuffer(GL_ARRAY_BUFFER, debrisVBO);
    glBufferData(GL_ARRAY_BUFFER, debrisMats.size() * sizeof(glm::mat4),
                 debrisMats.data(), GL_DYNAMIC_DRAW);

    // 4) hook it to aInstMat0..3 with divisor=1:
    std::size_t vec4Size = sizeof(glm::vec4);
    int matLoc[4] = {
        prog->getAttribute("aInstMat0"), prog->getAttribute("aInstMat1"),
        prog->getAttribute("aInstMat2"), prog->getAttribute("aInstMat3")};
    for (int i = 0; i < 4; ++i) {
      if (matLoc[i] < 0)
        continue;
      glEnableVertexAttribArray(matLoc[i]);
      glVertexAttribPointer(matLoc[i], 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                            (void *)(i * vec4Size));
      glVertexAttribDivisor(matLoc[i], 1);
    }

    // 5) finally draw instanced
    GLsizei instanceCount = (GLsizei)debrisMats.size();
    glDrawArraysInstanced(GL_TRIANGLES, 0, cubeMesh->getVertexCount(),
                          instanceCount);

    // 6) cleanup
    for (int i = 0; i < 4; ++i) {
      if (matLoc[i] >= 0) {
        glDisableVertexAttribArray(matLoc[i]);
        glVertexAttribDivisor(matLoc[i], 0);
      }
    }
    if (texLoc >= 0)
      glDisableVertexAttribArray(texLoc);
    glDisableVertexAttribArray(posLoc);
    if (norLoc >= 0)
      glDisableVertexAttribArray(norLoc);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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
    // aTex
    int texLoc = prog->getAttribute("aTex");
    if (texLoc >= 0 && cubeMesh->getTexBufID() != 0) {
      glEnableVertexAttribArray(texLoc);
      glBindBuffer(GL_ARRAY_BUFFER, cubeMesh->getTexBufID());
      glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
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
      if (texLoc >= 0) {
        glDisableVertexAttribArray(texLoc);
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
  void fracturedCube(int k, const glm::vec3 &impactPoint,
                     const glm::vec3 &bulletVelocity) {
    glm::vec3 cubePos = glm::vec3(modelMatsStatic[k][3]);

    // mark the particle free
    particles[k]->fixed = false;
    // remove all springs attached to that particle
    springs.erase(remove_if(begin(springs), end(springs),
                            [&](auto &s) {
                              return s->p0 == particles[k] ||
                                     s->p1 == particles[k];
                            }),
                  end(springs));

    // move its model matrix into freeCubes for separate physics, erase from
    // instance
    modelMatsStatic.erase(modelMatsStatic.begin() + k);
    uploadInstanceBuffer();

    // compute radial blast direction
    glm::vec3 dir = cubePos - impactPoint;
    if (glm::length(dir) < 1e-4f) {
      dir = glm::vec3(1, 0, 0);
    }
    dir = glm::normalize(dir);

    float blastStrength = 15.0f;
    FreeCube cc;
    cc.position = glmVec3ToEigen(cubePos);
    cc.velocity = glmVec3ToEigen(dir * blastStrength + bulletVelocity * 0.5f);
    cc.size = 1.0f;
    freeCubes.push_back(cc);
    std::cout << "[fracture] freeCubes now = " << freeCubes.size()
              << " (spawned at " << cubePos.x << "," << cubePos.y << ","
              << cubePos.z << ")\n";
  };

  // GETTERS and SETTERS
  GLuint getInstanceVBO() { return this->instanceVBO; };
  // Append to modelMatsStatic
  void pushBackModelMat(glm::mat4 mat) { modelMatsStatic.push_back(mat); };

  bool collidesAABB(glm::vec3 pMin, glm::vec3 pMax) const {
    const float half = 0.5f;
    for (auto &M : modelMatsStatic) {
      glm::vec3 c = glm::vec3(M[3]);
      glm::vec3 cMin = c - glm::vec3(half);
      glm::vec3 cMax = c + glm::vec3(half);

      // 1) if your slab is completely below this cube, skip it
      if (pMax.y <= cMin.y)
        continue;
      // 2) if your slab is completely above this cube, skip it (optional)
      if (pMin.y > cMax.y)
        continue;

      // 3) now do the XZ overlap test
      if (pMin.x < cMax.x && pMax.x > cMin.x && pMin.z < cMax.z &&
          pMax.z > cMin.z) {
        return true;
      }
    }
    return false;
  }
};
