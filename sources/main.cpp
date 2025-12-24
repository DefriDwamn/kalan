#include "Editor.hpp"
#include "Player.hpp"
#include "entt/entity/fwd.hpp"
#include "entt/entt.hpp"
#include "raylib-cpp.hpp"
#include "resources/AssetManager.hpp"
#include "resources/ParallelLoader.hpp"
#include "rendering/PBRMaterial.hpp"
#include "rendering/Lighting.hpp"
#include <chrono>

// Loading screen с анимацией
void DrawLoadingScreen(raylib::Window& window, const char* text, float progress = -1.0f) {
    BeginDrawing();
    window.ClearBackground(Color{20, 20, 25, 255});
    
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    // Текст
    const char* loadingText = text;
    int textW = MeasureText(loadingText, 30);
    DrawText(loadingText, (screenW - textW) / 2, screenH / 2 - 50, 30, WHITE);
    
    // Анимированные точки
    static int dots = 0;
    static double lastTime = 0;
    if (GetTime() - lastTime > 0.3) {
        dots = (dots + 1) % 4;
        lastTime = GetTime();
    }
    char dotsStr[5] = "....";
    dotsStr[dots] = '\0';
    DrawText(dotsStr, (screenW + textW) / 2 + 5, screenH / 2 - 50, 30, GRAY);
    
    // Прогресс бар (если есть)
    if (progress >= 0.0f) {
        int barW = 400;
        int barH = 20;
        int barX = (screenW - barW) / 2;
        int barY = screenH / 2 + 20;
        
        DrawRectangle(barX, barY, barW, barH, DARKGRAY);
        DrawRectangle(barX, barY, (int)(barW * progress), barH, Color{100, 180, 255, 255});
        DrawRectangleLines(barX, barY, barW, barH, GRAY);
        
        // Процент
        char percentText[16];
        snprintf(percentText, sizeof(percentText), "%d%%", (int)(progress * 100));
        int percentW = MeasureText(percentText, 20);
        DrawText(percentText, (screenW - percentW) / 2, barY + barH + 10, 20, WHITE);
    }
    
    EndDrawing();
}

int main() {
  raylib::Window window(1920, 1080, "Kalan");
  SetTraceLogLevel(LOG_INFO); // Временно для отладки
  DrawLoadingScreen(window, "Initializing");

  // Инициализация PBR системы
  kalan::PBRMaterial::initShader();
  kalan::LightingSystem::instance().init();
  
  kalan::LightingSystem::instance().addLight({
      .enabled = true,
      .type = kalan::LightType::Directional,
      .direction = {-1.0f, -1.0f, -1.0f},
      .color = WHITE,
      .intensity = 2.0f
  });

  entt::registry registry;

  raylib::Camera camera({0.2f, 0.4f, 0.2f}, {0.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f}, 45.0f);

  // Loading screen для модели с прогрессом
  DrawLoadingScreen(window, "Loading model", 0.0f);
  
  auto loadStart = std::chrono::high_resolution_clock::now();
  
  // Используем параллельный загрузчик с GPU-ускоренными mipmaps
  auto handsModel = kalan::ParallelModelLoader::instance().loadModel(
      "assets/models/nerf/nerf_retaliator.glb",
      [&window](const kalan::ParallelModelLoader::LoadProgress& progress) {
          DrawLoadingScreen(window, "Loading textures", progress.getUploadProgress());
      }
  );
  
  auto loadEnd = std::chrono::high_resolution_clock::now();
  auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();
  TraceLog(LOG_WARNING, "Model loaded in %lld ms", loadTime);

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
