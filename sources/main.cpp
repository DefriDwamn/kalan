#include "Player.hpp"
#include "raylib-cpp.hpp"

int main() {
  raylib::Window window(1920, 1080, "raylib test");
  Player player;

  while (!window.ShouldClose()) {
    // Update

    // Draw
    BeginDrawing();
    {
      window.ClearBackground(RAYWHITE);
      player.Update();
      player.Draw();
    }
    EndDrawing();
  }

  return 0;
}
