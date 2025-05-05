#pragma once

#include "Structure.h"
#include <algorithm>
enum class BulletType { RICOCHET, PIERCING };

// Thanks alot for ChatGPT for great help in developing collision detection
struct Bullet {
  glm::vec3 position;
  glm::vec3 velocity;
  int remainingBounces = 4;
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

  std::vector<Bullet> &getBullets() { return this->bullets; };

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
    bullets.push_back({playerPOVPosition, velocity, 4, type, true});
  };

  void update(float dt, std::vector<std::shared_ptr<Structure>> &structures) {
    modelMatsStatic.clear();
    for (auto it = bullets.begin(); it != bullets.end();) {
      bool bouncedThisFrame = false;
      if (!it->alive) {
        it = bullets.erase(it);
        continue;
      }
      // 1) advance
      it->position += it->velocity * dt;

      // Collison test
      bool bulletKilled = false;
      for (auto structure : structures) {
        float radius = 0.5f; // bullet radius
        auto hits = structure->collisionSphere(it->position, radius);

        if (!hits.empty()) {
          // sort descending so highest indices are procesed first
          // kill bullet if PIERCING and has collided
          std::sort(hits.begin(), hits.end(), std::greater<int>());
          if (it->type == BulletType::PIERCING) {
            if (structure->getFracturable()) {
              for (int idx : hits) {
                structure->fracturedCube(idx, it->position, it->velocity);
              }
            }
            // kill the bullet
            it = bullets.erase(it);
            // re‐upload your wall's instance buffer so that the fractured cubes
            // vanish
            structure->uploadInstanceBuffer();
            bulletKilled = true;
            break; // out of the structures loop
          } else {
            // RICOCHET: reflect the velocity around the cube normal (optional)
            for (int k : hits) {
              // // 1) Grab the model matrix for cube k and compute its inverse
              // (no scale):
              glm::mat4 M = structure->getModelMatsStatic()[k];
              glm::mat4 M_inv = glm::inverse(M);

              // 2) Transform the bullet position into cube‐local coordinates:
              glm::vec4 localPos4 = M_inv * glm::vec4(it->position, 1.0f);
              glm::vec3 localPos = glm::vec3(localPos4) - glm::vec3(0.0f);
              // since cubes are centered at the origin in local space

              // 3) Decide which face you’re closest to:
              glm::vec3 d = localPos;
              float ax = fabs(d.x), ay = fabs(d.y), az = fabs(d.z);
              glm::vec3 localNormal(0.0f);
              if (ax > ay && ax > az)
                localNormal.x = (d.x > 0.0f ? 1.0f : -1.0f);
              else if (ay > ax && ay > az)
                localNormal.y = (d.y > 0.0f ? 1.0f : -1.0f);
              else
                localNormal.z = (d.z > 0.0f ? 1.0f : -1.0f);

              // 4) Rotate that normal back into world‐space (ignore
              // translation):
              glm::vec3 worldNormal = glm::mat3(M) * localNormal;
              worldNormal = glm::normalize(worldNormal);
              // wrote — reflect velocity:
              glm::vec3 V = it->velocity;
              glm::vec3 R = V - 2.0f * glm::dot(V, worldNormal) * worldNormal;
              it->velocity = R * 0.8f;

              // nudge out:
              it->position += worldNormal * 0.01f;

              // decrement bounce count & kill if exhausted:
              it->remainingBounces--;
              if (it->remainingBounces <= 0) {
                it->alive = false;
              }
              bouncedThisFrame = true;
              break; // only handle the first hit this frame
            }
          }
        }
      }

      if (bulletKilled) {
        continue;
      }

      if (bouncedThisFrame) {
        if (it->alive) {
          // push transform into modelMatsStatic …
          glm::mat4 M =
              glm::translate(glm::mat4(1.0f), glm::vec3(it->position));
          M = M * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)); // half‑size
          modelMatsStatic.push_back(M);
          ++it;
        }
        continue;
      }

      // 3) lifetime / bounds check
      if (glm::length(it->position) > 100.0f /* or time‑to‑live */) {
        it = bullets.erase(it);
        continue;
      } else {
        // still alive
        glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(it->position));
        M = M * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)); // half‑size
        modelMatsStatic.push_back(M);
        ++it;
      }
    }

    uploadInstanceBuffer();
  }

  void renderBullets(std::shared_ptr<Program> prog) {
    if (modelMatsStatic.empty()) {
      return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    modelMatsStatic.size() * sizeof(glm::mat4),
                    modelMatsStatic.data());

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
