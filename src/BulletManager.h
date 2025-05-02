#pragma once

#include "Structure.h"
#include <algorithm>
enum class BulletType { RICOCHET, PIERCING };

struct Bullet {
  glm::vec3 position;
  glm::vec3 velocity;
  BulletType type = BulletType::RICOCHET;
  bool alive = false;
};

class BulletManager {
private:
  std::shared_ptr<Shape> sphereMesh;
  std::vector<glm::mat4> modelMatsStatic;
  std::vector<Bullet> bullets;

  GLuint instanceVBO = 0;

public:
  BulletManager(std::shared_ptr<Shape> sphereMesh) : sphereMesh(sphereMesh) {
    assert(sphereMesh->getType() == ShapeType::SPHERE);

    // generate one VBO for our instance‐mats
    glGenBuffers(1, &instanceVBO);
  };
  ~BulletManager() {
    if (instanceVBO)
      glDeleteBuffers(1, &instanceVBO);
  };

  // use when updating bullets
  void uploadInstanceBuffer() {
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, modelMatsStatic.size() * sizeof(glm::mat4),
                 modelMatsStatic.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  // POVPosition to mean spawn bullet in front of player in the direction they
  // are looking
  void spawnBullet(glm::vec3 playerPOVPosition = glm::vec3(0.0f),
                   glm::vec3 velocity = glm::vec3(3.0f),
                   BulletType type = BulletType::PIERCING) {
    bullets.push_back({playerPOVPosition, velocity, type, true});
  };

  void update(float dt, Structure &wall) {
    modelMatsStatic.clear();
    for (auto it = bullets.begin(); it != bullets.end();) {
      // 1) advance
      it->position += it->velocity * dt;

      // TODO: implement collision test
      // 2) collision test against your wall
      //    //    e.g. collisionSphere returns indices of hit cubes
      //    float radius = 1.0f;
      //    // auto hit = wall.collisionSphere(makeSphereShapeAt(b.position,
      //    radius)); auto hit = std::vector<int>(); if (!hit.empty()) {
      //      if (b.type == BulletType::PIERCING) {
      //        // fracture all the hit bricks
      //        for (int idx : hit)
      //          wall.fracturedCube(idx);
      //        // bullet keeps going (or you could kill it after N hits)
      //      } else {
      //        // ricochet: reverse velocity when you hit
      //        // b.velolity = glm::reflect(b.velocity, hitSurfaceNormal(...));
      //      }
      //    }

      // 3) lifetime / bounds check
      if (glm::length(it->position) > 100.0f /* or time‑to‑live */) {
        it = bullets.erase(it);
      } else {
        // still alive
        modelMatsStatic.push_back(
            glm::translate(glm::mat4(1.0f), it->position));
        ++it;
      }
    }

    // 4) cull dead bullets
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                 [](auto const &b) { return !b.alive; }),
                  bullets.end());
    uploadInstanceBuffer();
  }

  void renderBullets(std::shared_ptr<Program> prog) {
    if (modelMatsStatic.empty()) {
      return;
    }

    glBindVertexArray(sphereMesh->getVAO());

    // Bind vetex attribs (aPos and aNor)
    // aPos
    int posLoc = prog->getAttribute("aPos");
    glEnableVertexAttribArray(posLoc);
    glBindBuffer(GL_ARRAY_BUFFER, sphereMesh->getPosBufID());
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    // aNor
    int norLoc = prog->getAttribute("aNor");
    if (norLoc != -1) {
      glEnableVertexAttribArray(norLoc);
      glBindBuffer(GL_ARRAY_BUFFER, sphereMesh->getNorBufID());
      glVertexAttribPointer(norLoc, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    }

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
    glDrawArraysInstanced(GL_TRIANGLES, 0, sphereMesh->getVertexCount(),
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
    // cleanup
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
};
