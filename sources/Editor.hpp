#pragma once

#include "Player.hpp"
#include "imgui.h"
#include "raylib-cpp.hpp"
#include "rlImGui.h"

class Editor {
 public:
  Editor(Player* player)
      : player(player),
        handsOffset(player->GetHandsOffset()),
        handsRotation(player->GetHandsRotation()),
        smoothFactor(player->GetSmoothFactor()) {}

  void Draw();

 private:
  Player* player;
  raylib::Vector3 handsOffset = {0};
  raylib::Vector3 handsRotation = {0};
  float smoothFactor = 0.5f;
};

void Editor::Draw() {
  rlImGuiBegin();
  if (ImGui::Begin("Player Settings")) {
    ImGui::Text("Hands Offset:");
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    if (ImGui::InputFloat("X##handsOffset", &handsOffset.x, 0.1f, 1.0f)) {
      player->SetHandsOffset(handsOffset);
    }
    ImGui::SameLine();
    if (ImGui::InputFloat("Y##handsOffset", &handsOffset.y, 0.1f, 1.0f)) {
      player->SetHandsOffset(handsOffset);
    }
    ImGui::SameLine();
    if (ImGui::InputFloat("Z##handsOffset", &handsOffset.z, 0.1f, 1.0f)) {
      player->SetHandsOffset(handsOffset);
    }
    ImGui::PopItemWidth();

    ImGui::Text("Hands Rotation:");
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    if (ImGui::InputFloat("X##handsRotation", &handsRotation.x, 0.1f, 1.0f)) {
      player->SetHandsRotation(handsRotation);
    }
    ImGui::SameLine();
    if (ImGui::InputFloat("Y##handsRotation", &handsRotation.y, 0.1f, 1.0f)) {
      player->SetHandsRotation(handsRotation);
    }
    ImGui::SameLine();
    if (ImGui::InputFloat("Z##handsRotation", &handsRotation.z, 0.1f, 1.0f)) {
      player->SetHandsRotation(handsRotation);
    }
    ImGui::PopItemWidth();

    ImGui::Text("Smooth Factor:");
    ImGui::SameLine();
    if (ImGui::SliderFloat("##SmoothFactor", &smoothFactor, 0.0f, 50.0f)) {
      player->SetSmoothFactor(smoothFactor);
    }
  }
  ImGui::End();
  rlImGuiEnd();
}
