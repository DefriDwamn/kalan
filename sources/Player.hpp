#pragma once
#include "Model.hpp"
#include "Vector4.hpp"
#include "raylib-cpp.hpp"
#include <memory>

namespace kalan {

class Player {
private:
  raylib::Vector3 position;
  std::shared_ptr<raylib::Model> handsModel;
  raylib::Camera3D *camera;
  int speed;

  raylib::Vector3 handsOffset = {-0.4f, 0.05f, 0.0f};
  raylib::Vector3 handsRotation = {0.0f, 80.0f, 0.0f};
  raylib::Quaternion targetHandsRot = {0};
  raylib::Quaternion currHandsRot = {0};
  void UpdateHandsTransform();

public:
  Player(std::shared_ptr<raylib::Model> handsModel,
         raylib::Camera3D *camera = nullptr, raylib::Vector3 position = {0.},
         int speed = 10);

  void SetCamera(raylib::Camera3D *camera);

  void SetHandsOffset(const raylib::Vector3 &offset);
  void SetHandsRotation(const raylib::Vector3 &rotation);

  raylib::Vector3 GetHandsOffset() const;
  raylib::Vector3 GetHandsRotation() const;

  void Draw();
  void Update();
};

} // namespace kalan
