#include "Editor.hpp"
#include "Player.hpp"
#include "raylib.h"

int main() {
  raylib::Window window(1920, 1080, "Kalan");
  raylib::Camera camera({0.2f, 0.4f, 0.2f}, {0.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f}, 45.0f);

  std::unique_ptr<raylib::Model> playerModel =
      std::make_unique<raylib::Model>(raylib::MeshUnmanaged::Cube(1., 1., 1.));
  Player player(std::move(playerModel), &camera, {0.}, 10);

  Editor editor(&player);
  rlImGuiSetup(true);

  DisableCursor();
  SetExitKey(0);

  while (!window.ShouldClose()) {
    if (IsKeyPressed(KEY_F11)) {
      if (IsWindowFullscreen()) {
        ClearWindowState(FLAG_FULLSCREEN_MODE);
      } else {
        SetWindowState(FLAG_FULLSCREEN_MODE);
      }
    }

    player.Update();
    camera.Update(CameraMode::CAMERA_FIRST_PERSON);
    HideCursor();

    BeginDrawing();
    {
      window.ClearBackground(RAYWHITE);
      camera.BeginMode();
      {
        player.Draw();
        DrawGrid(100, 1);
        DrawCube({0}, 2.0f, 2.0f, 2.0f, YELLOW);
      }
      camera.EndMode();
      window.DrawFPS(0, 0);
      editor.Draw();
    }
    EndDrawing();
  }

  rlImGuiShutdown();
  return 0;
}
