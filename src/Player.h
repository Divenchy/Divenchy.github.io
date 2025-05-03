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
    glm::vec3 pMin{candidate.x - COLLISION_RADIUS_XZ, candidate.y,
                   candidate.z - COLLISION_RADIUS_XZ};
    glm::vec3 pMax{candidate.x + COLLISION_RADIUS_XZ,
                   candidate.y + (COLLISION_HEAD_Y - COLLISION_FOOT_Y),
                   candidate.z + COLLISION_RADIUS_XZ};
    return wall.collidesAABB(pMin, pMax);
  }

  void move(GLFWwindow *window, float dt,
            const std::vector<std::shared_ptr<Structure>> &structures) {
    // 0) grab your current 3D position
    glm::vec3 pos = playerPOV->getPosition();

    // 1) -- handle jumping & gravity
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && grounded) {
      vertVel = JUMP_SPEED;
      grounded = false;
    }
    vertVel += GRAV_VEC.y() * dt; // apply gravity
    pos.y += vertVel * dt;        // move vertically

    // floor / ground collision
    if (pos.y <= COLLISION_FOOT_Y) {
      pos.y = COLLISION_FOOT_Y;
      vertVel = 0.0f;
      grounded = true;
    }

    // 2) build your input‐directed horizontal velocity
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

    // 3) a single horiz sweep at your _new_ Y
    glm::vec3 horizCandidate = pos + glm::vec3(horizVel.x, 0, horizVel.z) * dt;
    // build a *short* AABB (just around your feet)
    float stepHeight = 0.5f;
    glm::vec3 footMin{horizCandidate.x - COLLISION_RADIUS_XZ, pos.y,
                      horizCandidate.z - COLLISION_RADIUS_XZ};
    glm::vec3 footMax{horizCandidate.x + COLLISION_RADIUS_XZ,
                      pos.y + stepHeight,
                      horizCandidate.z + COLLISION_RADIUS_XZ};
    bool blocked = false;
    for (auto &s : structures) {
      // pass the candidate foot‐location (x,z) and your _new_ pos.y
      if (s->collidesAABB(footMin, footMax)) {
        blocked = true;
        break;
      }
    }
    if (!blocked) {
      pos.x = horizCandidate.x;
      pos.z = horizCandidate.z;
    }

    // 4) finally commit
    playerPOV->setPosition(pos);
  }
};
