#include "Armament.h"
#include "Camera.h"
#include "GLFW/glfw3.h"

static constexpr float COLLISION_RADIUS_XZ = 0.3f;
static constexpr float COLLISION_FOOT_Y = 0.0f;
static constexpr float COLLISION_HEAD_Y = 1.8f; // eye level

class Player {
private:
  float speed = 12.0f;
  float vertVel = 0.0f;
  bool grounded = true;
  int ARMAMENT_MODE = 0;
  static constexpr float JUMP_SPEED = 8.0f;
  std::shared_ptr<Camera> playerPOV;
  std::shared_ptr<Armament> armament;
  std::shared_ptr<BulletManager> bulletManager;

public:
  // Default
  Player() : speed(15.0f) {
    this->playerPOV = std::make_shared<Camera>();
    this->armament = std::make_shared<Armament>();
  };

  Player(std::shared_ptr<Camera> cam,
         std::shared_ptr<BulletManager> bulletManger)
      : speed(15.0f), playerPOV(cam), bulletManager(bulletManger) {
    this->armament = std::make_shared<Armament>();
  };

  std::shared_ptr<Armament> getArmament() { return this->armament; };

  void shoot() {
    glm::vec3 bulletPos =
        playerPOV->getPosition() + playerPOV->getForward() * 1.0f;
    bulletPos.y += 1.0f;
    glm::vec3 bulletVelVec = playerPOV->getForward() * 35.0f;
    auto req = armament->fireArmament(
        bulletPos, bulletVelVec,
        this->ARMAMENT_MODE); // 1 for piercing, 0 for ricochet
    if (req) {
      this->bulletManager->spawnBullet(req->pos, req->vel, req->type);
    };
  };

  void setArmamentMode(int mode) { this->ARMAMENT_MODE = mode; };
  void setWeapon(std::shared_ptr<Armament> weapon) { this->armament = weapon; };

  bool wouldCollide(const glm::vec3 &candidate, const Structure &wall) const {
    glm::vec3 pMin{candidate.x - COLLISION_RADIUS_XZ, candidate.y,
                   candidate.z - COLLISION_RADIUS_XZ};
    glm::vec3 pMax{candidate.x + COLLISION_RADIUS_XZ,
                   candidate.y + (COLLISION_HEAD_Y - COLLISION_FOOT_Y),
                   candidate.z + COLLISION_RADIUS_XZ};
    return wall.collidesAABB(pMin, pMax);
  }

  void move(GLFWwindow *window, float dt,
            const std::vector<std::shared_ptr<Structure>> &structures) {
    // 0) get current position
    glm::vec3 pos = playerPOV->getPosition();
    const float r = COLLISION_RADIUS_XZ;

    // —— VERTICAL MOVEMENT ——
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && grounded) {
      vertVel = JUMP_SPEED;
      grounded = false;
    }
    vertVel += GRAV_VEC.y() * dt;
    float nextY = pos.y + vertVel * dt;

    // land on any cube directly under your XZ footprint
    if (nextY <= COLLISION_FOOT_Y) {
      pos.y = COLLISION_FOOT_Y; // lock at zero
      vertVel = 0.0f;
      grounded = true;
    } else {
      bool hitGround = false;
      float bestY = -1e9f;
      for (auto &s : structures) {
        for (auto &M : s->getModelMatsStatic()) {
          glm::vec3 c = glm::vec3(M[3]);
          if (pos.x + r > c.x - 0.5f && pos.x - r < c.x + 0.5f &&
              pos.z + r > c.z - 0.5f && pos.z - r < c.z + 0.5f) {
            float topY = c.y + 0.5f;
            if (nextY <= topY && pos.y >= topY) {
              hitGround = true;
              bestY = std::max(bestY, topY);
            }
          }
        }
      }
      if (hitGround) {
        pos.y = bestY;
        vertVel = 0.0f;
        grounded = true;
      } else {
        pos.y = nextY;
        grounded = false;
      }
    }

    // —— HORIZONTAL MOVEMENT ——
    // build a flat‐only candidate
    glm::vec3 forward = playerPOV->getForward();
    forward.y = 0;
    forward = glm::normalize(forward);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 dir(0);
    if (glfwGetKey(window, GLFW_KEY_W))
      dir += forward;
    if (glfwGetKey(window, GLFW_KEY_S))
      dir -= forward;
    if (glfwGetKey(window, GLFW_KEY_D))
      dir += right;
    if (glfwGetKey(window, GLFW_KEY_A))
      dir -= right;
    glm::vec3 horizVel =
        glm::length(dir) > 1e-4f ? glm::normalize(dir) * speed : glm::vec3(0);

    glm::vec3 candidate = pos + glm::vec3(horizVel.x, 0, horizVel.z) * dt;

    // full‐body slab at new Y
    glm::vec3 pMin{candidate.x - r, pos.y + COLLISION_FOOT_Y, candidate.z - r};
    glm::vec3 pMax{candidate.x + r,
                   pos.y + (COLLISION_HEAD_Y - COLLISION_FOOT_Y),
                   candidate.z + r};

    // **only** collide against cubes whose *top* is *above* your foot‐level
    bool blocked = false;
    for (auto &s : structures) {
      for (auto &M : s->getModelMatsStatic()) {
        glm::vec3 c = glm::vec3(M[3]);
        float botY = c.y - 0.5f;
        float topY = c.y + 0.5f;

        // skip the platform you’re standing on:
        if (fabs(topY - pos.y) < 1e-2f)
          continue;

        // AABB vs your candidate box:
        if (pMin.x < c.x + 0.5f && pMax.x > c.x - 0.5f && pMin.y < topY &&
            pMax.y > botY && pMin.z < c.z + 0.5f && pMax.z > c.z - 0.5f) {
          blocked = true;
          break;
        }
      }
      if (blocked)
        break;
    }

    if (!blocked) {
      // no wall in the way
      pos.x = candidate.x;
      pos.z = candidate.z;
    } else {
      // try a little step‐up
      const float maxStep = 0.5f;
      float bestTopY = -1e9f;
      for (auto &s : structures) {
        for (auto &M : s->getModelMatsStatic()) {
          glm::vec3 c = glm::vec3(M[3]);
          float topY = c.y + 0.5f;
          if (candidate.x > c.x - 0.5f && candidate.x < c.x + 0.5f &&
              candidate.z > c.z - 0.5f && candidate.z < c.z + 0.5f &&
              topY > pos.y && topY <= pos.y + maxStep) {
            bestTopY = std::max(bestTopY, topY);
          }
        }
      }
      if (bestTopY > -1e8f) {
        pos.x = candidate.x;
        pos.z = candidate.z;
        pos.y = bestTopY;
        vertVel = 0.0f;
        grounded = true;
      }
      // else stay in place
    }

    playerPOV->setPosition(pos);
  }

  void setPlayerPos(glm::vec3 pos) { playerPOV->setPosition(pos); };
  glm::vec3 getPlayerPos() { return playerPOV->getPosition(); };
};
