#include <memory>

#include "Matrix.hpp"
#include "Player.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include "raylib-cpp.hpp"
#include "raylib.h"

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

  raylib::Vector3 handsPos = raylib::Vector3(camera->position) + direction;

  raylib::Matrix lookToMatrix =
      raylib::Matrix::LookAt(handsPos, camera->position, {0.0f, 1.0f, 0.0f})
          .Invert();

  targetHandsRot = raylib::Vector4::FromMatrix(lookToMatrix);
  currHandsRot =
      currHandsRot.Slerp(targetHandsRot, smoothFactor * GetFrameTime());

  lookToMatrix = currHandsRot.ToMatrix();
  lookToMatrix.m12 = handsPos.x;
  lookToMatrix.m13 = handsPos.y;
  lookToMatrix.m14 = handsPos.z;

  raylib::Matrix rotationMatrix =
      raylib::Matrix::RotateZ(handsRotation.z * DEG2RAD) *
      raylib::Matrix::RotateY(handsRotation.y * DEG2RAD) *
      raylib::Matrix::RotateX(handsRotation.x * DEG2RAD);

  // Сначала умножаем матрицу трансформации на матрицу передвижения c оффсетом -
  // и только потом задаем вращение, чтобы модель рук вращалсь вокруг своих осей
  handsModel->transform =
      raylib::Matrix::Translate(handsOffset.x, handsOffset.y, handsOffset.z) *
      lookToMatrix;
  handsModel->transform =
      rotationMatrix * raylib::Matrix(handsModel->transform);
}

void Player::Update() {
  if (!camera) return;
  UpdateHandsTransform();
}

void Player::Draw() {
  if (!camera || !handsModel) return;
  handsModel->Draw({0}, 1.0f, RED);
}
