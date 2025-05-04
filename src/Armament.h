#include "BulletManager.h"
#include <iostream>
#include <optional>

struct BulletRequest {
  glm::vec3 pos, vel;
  BulletType type;
};

class Armament {
private:
  int ricochetAmmo;
  int piercingAmmo;

public:
  Armament() : ricochetAmmo(5), piercingAmmo(5) {}; // Default constructor
  Armament(int ricoAmmo, int pierceAmmo)
      : ricochetAmmo(ricoAmmo), piercingAmmo(pierceAmmo) {};
  bool hasAmmo(int mode) {
    if (mode == 1) {
      if (this->piercingAmmo > 0) {
        return true;
      }
      std::cout << "[ARMAMENT] OUT OF PIERCING AMMO" << std::endl;
    } else if (mode == 0) {
      if (this->ricochetAmmo > 0) {
        return true;
      }
      std::cout << "[ARMAMENT] OUT OF RICOCHET AMMO" << std::endl;
    }

    return false;
  };

  void decrementAmmo(int mode) {
    if (mode == 1) {
      if (piercingAmmo > 0) {
        this->piercingAmmo--;
      }
    } else if (mode == 0) {
      if (this->ricochetAmmo > 0) {
        this->ricochetAmmo--;
      }
    }
  };

  std::optional<BulletRequest> fireArmament(glm::vec3 bulletPos,
                                            glm::vec3 bulletVVec, int mode) {
    if (hasAmmo(mode)) {
      decrementAmmo(mode);
      return BulletRequest{bulletPos, bulletVVec,
                           mode == 1 ? BulletType::PIERCING
                                     : BulletType::RICOCHET};
    }
    return std::nullopt; // no shot fired
  };

  std::pair<int, int> getAmmoLeft() {
    return {this->piercingAmmo, this->ricochetAmmo};
  };
};
