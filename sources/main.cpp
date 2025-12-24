#include "Editor.hpp"
#include "Player.hpp"
#include "entt/entity/fwd.hpp"
#include "entt/entt.hpp"
#include "raylib-cpp.hpp"
#include "resources/AssetManager.hpp"
#include "rendering/PBRMaterial.hpp"
#include "rendering/Lighting.hpp"

int main() {
  raylib::Window window(1920, 1080, "Kalan");

  // Инициализация PBR системы
  kalan::PBRMaterial::initShader();
  kalan::LightingSystem::instance().init();
  
  kalan::LightingSystem::instance().addLight({
      .enabled = true,
      .type = kalan::LightType::Point,
      .direction = {-1.0f, -1.0f, -1.0f},
      .color = WHITE,
      .intensity = 2.0f
  });
  
  // Теперь можно включить autoPBR
  kalan::AssetManager::instance().enableAutoPBR(true);

  entt::registry registry;

  raylib::Camera camera({0.2f, 0.4f, 0.2f}, {0.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f}, 45.0f);

  auto handsModel =
      kalan::AssetManager::instance().getModel("nerf/nerf_retaliator.glb");
  kalan::Player player(std::move(handsModel), &camera, {0.}, 10);

  kalan::Editor &editor = kalan::Editor::GetInstance(&player);
  SetExitKey(0);

  while (!window.ShouldClose()) {
    // Updating

    // entity in InputSystem should update
    if (raylib::Keyboard::IsKeyPressed(KEY_F11))
      window.ToggleFullscreen();
    if (raylib::Keyboard::IsKeyPressed(KEY_F10)) {
      editor.ToggleShow();
      if (editor.IsVisible())
        EnableCursor();
      else
        DisableCursor();
    }
    //

    // entity with TrasformComponent should update
    if (!editor.IsVisible())
      camera.Update(CameraMode::CAMERA_FIRST_PERSON);
    player.Update();
    //

    // Drawing
    BeginDrawing();
    {
      window.ClearBackground(RAYWHITE);

      // entity with DrawableComponent 3d should update
      camera.BeginMode();
      {
        // Обновить uniforms освещения
        kalan::LightingSystem::instance().update(camera);
        
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
