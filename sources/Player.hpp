#pragma once
#include <memory>

#include "Vector4.hpp"
#include "raylib-cpp.hpp"

class Player {
 private:
  raylib::Vector3 position;
  std::unique_ptr<raylib::Model> handsModel;
  raylib::Camera3D* camera;
  int speed;

  raylib::Vector3 handsOffset = {-1.1f, -0.3f, 2.0f};
  raylib::Vector3 handsRotation = {-10.0f, 0.0f, 0.0f};
  raylib::Quaternion targetHandsRot = {0};
  raylib::Quaternion currHandsRot = {0};
  float smoothFactor = 50;
  void UpdateHandsTransform();

 public:
  Player(std::unique_ptr<raylib::Model> model, raylib::Camera3D* camera,
         raylib::Vector3 position, int speed);

  void SetCamera(raylib::Camera3D* camera);

  void SetHandsOffset(const raylib::Vector3& offset);
  void SetHandsRotation(const raylib::Vector3& rotation);
  void SetSmoothFactor(float smooth);

  raylib::Vector3 GetHandsOffset() const;
  raylib::Vector3 GetHandsRotation() const;
  float GetSmoothFactor() const;

  void Draw();
  void Update();
};
