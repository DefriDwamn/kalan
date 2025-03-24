#include <memory>

#include "Player.hpp"
#include "raylib-cpp.hpp"

Player::Player(std::unique_ptr<raylib::Model> handsModel,
               raylib::Camera3D* camera = nullptr,
               raylib::Vector3 position = {0.}, int speed = 10)
    : handsModel(std::move(handsModel)),
      camera(camera),
      position(position),
      speed(speed) {}

void Player::SetCamera(raylib::Camera3D* camera) { this->camera = camera; }

void Player::SetHandsOffset(const raylib::Vector3& offset) {
  handsOffset = offset;
}

void Player::SetHandsRotation(const raylib::Vector3& rotation) {
  handsRotation = rotation;
}

void Player::SetSmoothFactor(float smooth) { smoothFactor = smooth; }

raylib::Vector3 Player::GetHandsOffset() const { return handsOffset; }

raylib::Vector3 Player::GetHandsRotation() const { return handsRotation; }

float Player::GetSmoothFactor() const { return smoothFactor; }

void Player::UpdateHandsTransform() {
  if (!camera || !handsModel) return;

  raylib::Vector3 direction =
      (raylib::Vector3(camera->target) - camera->position).Normalize();

  targetHandsPos = raylib::Vector3(camera->position) + direction;
  currHandsPos =
      currHandsPos.Lerp(targetHandsPos, smoothFactor * GetFrameTime());

  raylib::Matrix lookAtMatrix =
      raylib::Matrix::LookAt(currHandsPos, camera->position, {0.0f, 1.0f, 0.0f})
          .Invert();

  raylib::Matrix rotationMatrix =
      raylib::Matrix::RotateZ(handsRotation.z * DEG2RAD) *
      raylib::Matrix::RotateY(handsRotation.y * DEG2RAD) *
      raylib::Matrix::RotateX(handsRotation.x * DEG2RAD);

  handsModel->transform = rotationMatrix * lookAtMatrix;
  handsModel->transform =
      raylib::Matrix::Translate(handsOffset.x, handsOffset.y, handsOffset.z) *
      handsModel->transform;
}

void Player::Update() {
  if (!camera) return;
  UpdateHandsTransform();
}

void Player::Draw() {
  if (!camera || !handsModel) return;
  handsModel->Draw({0}, 1.0f, RED);
}
