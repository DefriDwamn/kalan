#include "raylib-cpp.hpp"

int main() {
  raylib::Color textColor = raylib::Color::LightGray();
  raylib::Window window(1920, 1080, "raylib [core] example - basic window");

  while (!window.ShouldClose()) {
    // Update

    // Draw
    BeginDrawing();
    {
      window.ClearBackground(RAYWHITE);
      textColor.DrawText("Congrats! You created your first window!", 190, 200,
                         20);
    }
    EndDrawing();
  }

  return 0;
}
