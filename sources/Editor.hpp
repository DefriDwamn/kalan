#pragma once

#include "Player.hpp"
#include "imgui.h"
#include "raylib-cpp.hpp"
#include "raylib.h"
#include "rlImGui.h"

class Editor {
 public:
  Editor(Player* player)
      : player(player),
        handsOffset(player->GetHandsOffset()),
        handsRotation(player->GetHandsRotation()),
        smoothFactor(player->GetSmoothFactor()),
        show(true) {}

  void Draw();
  void ToggleShow();

 private:
  float fastStep = 10.f;
  float step = 1.f;
  Player* player;
  raylib::Vector3 handsOffset;
  raylib::Vector3 handsRotation;
  float smoothFactor;
  bool show;
};

void Editor::ToggleShow() { show = !show; }

void Editor::Draw() {
  if (!show) return;

  rlImGuiBegin();
  if (ImGui::Begin("Player Settings")) {
    ImGui::Text("Hands Offset:");
    ImGui::PushItemWidth(100);
    if (ImGui::InputFloat("X##handsOffset", &handsOffset.x, step, fastStep)) {
      player->SetHandsOffset(handsOffset);
    }
    ImGui::SameLine();
    if (ImGui::InputFloat("Y##handsOffset", &handsOffset.y, step, fastStep)) {
      player->SetHandsOffset(handsOffset);
    }
    ImGui::SameLine();
    if (ImGui::InputFloat("Z##handsOffset", &handsOffset.z, step, fastStep)) {
      player->SetHandsOffset(handsOffset);
    }
    ImGui::PopItemWidth();

    ImGui::Text("Hands Rotation:");
    ImGui::PushItemWidth(100);
    if (ImGui::InputFloat("X##handsRotation", &handsRotation.x, step,
                          fastStep)) {
      player->SetHandsRotation(handsRotation);
    }
    ImGui::SameLine();
    if (ImGui::InputFloat("Y##handsRotation", &handsRotation.y, step,
                          fastStep)) {
      player->SetHandsRotation(handsRotation);
    }
    ImGui::SameLine();
    if (ImGui::InputFloat("Z##handsRotation", &handsRotation.z, step,
                          fastStep)) {
      player->SetHandsRotation(handsRotation);
    }
    ImGui::PopItemWidth();

    ImGui::Text("Smooth Factor:");
    if (ImGui::SliderFloat("##SmoothFactor", &smoothFactor, 0.0f, 200.0f)) {
      player->SetSmoothFactor(smoothFactor);
    }
  }
  ImGui::End();
  rlImGuiEnd();
}
