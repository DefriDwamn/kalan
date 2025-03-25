#include "Editor.hpp"
#include "Keyboard.hpp"
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

  SetExitKey(0);

  while (!window.ShouldClose()) {
    // Updating
    if (raylib::Keyboard::IsKeyPressed(KEY_F11)) window.ToggleFullscreen();
    if (raylib::Keyboard::IsKeyPressed(KEY_F10)) {
      editor.ToggleShow();
      if (editor.IsVisible())
        EnableCursor();
      else
        DisableCursor();
    }

    if (!editor.IsVisible()) camera.Update(CameraMode::CAMERA_FIRST_PERSON);
    player.Update();

    // Drawing
    BeginDrawing();
    {
      window.ClearBackground(RAYWHITE);

      // 3D Drawing 
      camera.BeginMode();
      {
        player.Draw();
        DrawGrid(100, 1);
        DrawCube({0}, 2.0f, 2.0f, 2.0f, YELLOW);
      }

      // 2D Drawing
      camera.EndMode();
      window.DrawFPS(0, 0);
      editor.Draw();
    }
    EndDrawing();
  }

  rlImGuiShutdown();
  return 0;
}
