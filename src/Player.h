#include "Armament.h"
#include "Camera.h"
#include "GLFW/glfw3.h"

static constexpr float COLLISION_RADIUS_XZ = 0.3f;
static constexpr float COLLISION_FOOT_Y = 0.0f;
static constexpr float COLLISION_HEAD_Y = 1.8f; // eye level

class Player {
private:
  int health;
  float speed = 15.0f;
  float vertVel = 0.0f;
  bool grounded = true;
  static constexpr float JUMP_SPEED = 8.0f;
  std::shared_ptr<Camera> playerPOV;
  std::shared_ptr<Armament> armament;
  std::shared_ptr<BulletManager> bulletManager;

public:
  // Default
  Player() : health(100), speed(15.0f) {
    this->playerPOV = std::make_shared<Camera>();
    this->armament = std::make_shared<Armament>();
  };

  Player(std::shared_ptr<Camera> cam,
         std::shared_ptr<BulletManager> bulletManger)
      : health(100), speed(15.0f), playerPOV(cam), bulletManager(bulletManger) {
    this->armament = std::make_shared<Armament>();
  };

  void shoot() {
    glm::vec3 bulletPos =
        playerPOV->getPosition() + playerPOV->getForward() * 1.0f;
    glm::vec3 bulletVelVec = playerPOV->getForward() * 10.0f;
    auto req = armament->fireArmament(bulletPos, bulletVelVec,
                                      1); // 1 for piercing, 0 for ricochet
    if (req) {
      this->bulletManager->spawnBullet(req->pos, req->vel, req->type);
    };
  };

  void setWeapon(std::shared_ptr<Armament> weapon) { this->armament = weapon; };

  bool wouldCollide(const glm::vec3 &candidate, const Structure &wall) const {
    glm::vec3 pMin{candidate.x - COLLISION_RADIUS_XZ, COLLISION_FOOT_Y,
                   candidate.z - COLLISION_RADIUS_XZ};
    glm::vec3 pMax{candidate.x + COLLISION_RADIUS_XZ, COLLISION_HEAD_Y,
                   candidate.z + COLLISION_RADIUS_XZ};
    // ask the structure: does any cube’s AABB overlap [pMin,pMax]?
    return wall.collidesAABB(pMin, pMax);
  }

  void move(GLFWwindow *window, float dt,
            const std::vector<std::shared_ptr<Structure>> &structures) {
    // 1) figure out your input direction
    glm::vec3 forward = playerPOV->getForward();
    forward.y = 0;
    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

    glm::vec3 dir(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      dir += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      dir -= forward;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      dir += right;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      dir -= right;

    glm::vec3 horizVel(0.0f);
    if (glm::length(dir) > 1e-4f) {
      dir = glm::normalize(dir);
      horizVel = dir * speed;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && grounded) {
      vertVel = JUMP_SPEED;
      grounded = false;
    }

    vertVel += (float)GRAV_VEC.y() * dt;

    // calc new position
    glm::vec3 pos = playerPOV->getPosition();
    glm::vec3 candidate = pos + horizVel * dt;
    candidate.y += vertVel * dt;

    // ground collision
    if (candidate.y <= COLLISION_FOOT_Y) {
      candidate.y = COLLISION_FOOT_Y;
      vertVel = 0.0f;
      grounded = true;
    }

    // 4) AABB‐test against every structure
    for (auto &s : structures) {
      if (wouldCollide(candidate, *s)) {
        // cancel horiz motions, but keep vert
        candidate.x = pos.x;
        candidate.z = pos.z;
        break;
      }
    }

    // Can move
    playerPOV->setPosition(candidate);
  }
};
