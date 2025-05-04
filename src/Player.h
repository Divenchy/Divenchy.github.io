#include "Armament.h"
#include "Camera.h"
#include "GLFW/glfw3.h"

static constexpr float COLLISION_RADIUS_XZ = 0.3f;
static constexpr float COLLISION_FOOT_Y = 0.0f;
static constexpr float COLLISION_HEAD_Y = 1.8f; // eye level

class Player {
private:
  int health;
  float speed = 12.0f;
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

  std::shared_ptr<Armament> getArmament() { return this->armament; };

  void shoot() {
    glm::vec3 bulletPos =
        playerPOV->getPosition() + playerPOV->getForward() * 1.0f;
    bulletPos.y += 1.0f;
    glm::vec3 bulletVelVec = playerPOV->getForward() * 35.0f;
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

    // Jumping and vertical
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && grounded) {
      vertVel = JUMP_SPEED;
      grounded = false;
    }
    vertVel += GRAV_VEC.y() * dt; // apply gravity

    pos.y += vertVel * dt;
    // floor / ground collision
    if (pos.y <= COLLISION_FOOT_Y) {
      pos.y = COLLISION_FOOT_Y;
      vertVel = 0.0f;
      grounded = true;
    }

    // Horizontal checks
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
    } else {
      horizVel = glm::vec3(0.0f);
    }

    // — first try to move at current Y —
    glm::vec3 candidate = pos + glm::vec3(horizVel.x, 0, horizVel.z) * dt;
    const float r = COLLISION_RADIUS_XZ;
    // your “feet” AABB
    auto makeslab = [&](float y0) {
      return std::make_pair(
          glm::vec3(candidate.x - r, y0, candidate.z - r),
          glm::vec3(candidate.x + r, y0 + 0.5f, candidate.z + r));
    };
    auto [footMin, footMax] = makeslab(pos.y);

    bool blocked = false;
    for (auto &s : structures) {
      if (s->collidesAABB(footMin, footMax)) {
        blocked = true;
        break;
      }
    }

    // Horiz
    if (!blocked) {
      // no obstacle: commit
      pos.x = candidate.x;
      pos.z = candidate.z;
    } else {
      // blocked — see if we can “step up” a little
      const float maxStep = 0.5f; // max step height
      float bestTopY = -1e9f;
      // for each structure, look for cubes whose AABB sits between current Y
      // and current Y+maxStep
      for (auto &s : structures) {
        for (auto &M : s->getModelMatsStatic()) {
          glm::vec3 c = glm::vec3(M[3]); // cube center
          float topY = c.y + 0.5f;       // cube’s top face
          // only consider if it’s within step range on X,Z and within maxStep
          // above you
          if (topY > pos.y && topY <= pos.y + maxStep &&
              candidate.x > c.x - 0.5f && candidate.x < c.x + 0.5f &&
              candidate.z > c.z - 0.5f && candidate.z < c.z + 0.5f) {
            bestTopY = std::max(bestTopY, topY);
          }
        }
      }

      if (bestTopY > -1e8f) {
        // climb up
        pos.x = candidate.x;
        pos.z = candidate.z;
        pos.y = bestTopY;
        grounded = true; // you’re standing on that little ledge now
        vertVel = 0.0f;
      }
      // else: still blocked, so stay in place
    }

    // finally commit
    playerPOV->setPosition(pos);
  }

  void setPlayerPos(glm::vec3 pos) { playerPOV->setPosition(pos); };
};
