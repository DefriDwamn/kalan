#include "Editor.hpp"
#include "Player.hpp"
#include "entt/entity/fwd.hpp"
#include "entt/entt.hpp"
#include "raylib-cpp.hpp"

int main() {
  raylib::Window window(1920, 1080, "Kalan");

  entt::registry registry;

  raylib::Camera camera({0.2f, 0.4f, 0.2f}, {0.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f}, 45.0f);

  std::unique_ptr<raylib::Model> playerModel =
      std::make_unique<raylib::Model>(raylib::MeshUnmanaged::Cube(1., 1., 1.));
  Player player(std::move(playerModel), &camera, {0.}, 10);

  Editor& editor = Editor::GetInstance(&player);
  SetExitKey(0);

  while (!window.ShouldClose()) {
    // Updating

    // entity in InputSystem should update
    if (raylib::Keyboard::IsKeyPressed(KEY_F11)) window.ToggleFullscreen();
    if (raylib::Keyboard::IsKeyPressed(KEY_F10)) {
      editor.ToggleShow();
      if (editor.IsVisible())
        EnableCursor();
      else
        DisableCursor();
    }
    //

    // entity with TrasformComponent should update
    if (!editor.IsVisible()) camera.Update(CameraMode::CAMERA_FIRST_PERSON);
    player.Update();
    //

    // Drawing
    BeginDrawing();
    {
      window.ClearBackground(RAYWHITE);

      // entity with DrawableComponent 3d should update
      camera.BeginMode();
      {
        player.Draw();
        DrawGrid(100, 1);
        DrawCube({0}, 2.0f, 2.0f, 2.0f, YELLOW);
      }
      camera.EndMode();
      //

      // entity with DrawableComponent 2d should update
      window.DrawFPS(0, 0);
      editor.Draw();
      //
    }
    EndDrawing();
  }
  return 0;
}
