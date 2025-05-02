#include "Armament.h"
#include "Camera.h"

class Player {
private:
  int health;
  float speed;
  std::shared_ptr<Camera> playerPOV;
  std::shared_ptr<Armament> armament;
  std::shared_ptr<BulletManager> bulletManager;

public:
  // Default
  Player() : health(100), speed(1.0f) {
    this->playerPOV = std::make_shared<Camera>();
    this->armament = std::make_shared<Armament>();
  };
  Player(std::shared_ptr<Camera> cam,
         std::shared_ptr<BulletManager> bulletManger)
      : health(100), speed(1.0f), playerPOV(cam), bulletManager(bulletManger) {
    this->armament = std::make_shared<Armament>();
  };
  void shoot() {
    glm::vec3 bulletPos =
        playerPOV->getPosition() + playerPOV->getForward() * 1.0f;
    glm::vec3 bulletVelVec = playerPOV->getForward() * 4.0f;
    auto req = armament->fireArmament(bulletPos, bulletVelVec,
                                      1); // 1 for piercing, 0 for ricochet
    if (req) {
      this->bulletManager->spawnBullet(req->pos, req->vel, req->type);
    };
  };
  void setWeapon(std::shared_ptr<Armament> weapon) { this->armament = weapon; };
};
