#pragma once

#include "raylib-cpp.hpp"
#include "PBRMaterial.hpp"
#include <vector>
#include <array>

namespace kalan {

enum class LightType : int {
    Point = 0,
    Directional = 1,
    Spot = 2
};

struct Light {
    bool enabled = true;
    LightType type = LightType::Point;
    Vector3 position{0, 0, 0};
    Vector3 direction{0, -1, 0};
    Color color = WHITE;
    float intensity = 1.0f;
    float cutoff = 45.0f;        // для Spot (в градусах)
    float outerCutoff = 60.0f;   // для Spot (в градусах)
};

class LightingSystem {
public:
    static constexpr int MaxLights = 16;
    
    static LightingSystem& instance() noexcept;
    
    // Инициализация (вызвать после PBRMaterial::initShader)
    void init();
    
    // Добавить источник света, возвращает индекс
    int addLight(const Light& light);
    
    // Получить/изменить свет по индексу
    Light& getLight(int index);
    const Light& getLight(int index) const;
    
    // Удалить свет
    void removeLight(int index);
    
    // Очистить все источники
    void clearLights();
    
    // Задать ambient цвет
    void setAmbientColor(Color color);
    void setAmbientColor(Vector3 color);
    
    // Обновить uniforms в шейдере (вызывать каждый кадр после BeginMode3D)
    void update(const raylib::Camera& camera);
    
    [[nodiscard]] int getLightCount() const noexcept { return static_cast<int>(lights_.size()); }

private:
    LightingSystem() = default;
    
    std::vector<Light> lights_;
    Vector3 ambientColor_{0.03f, 0.03f, 0.03f};
    
    // Shader locations
    int locLightCount_ = -1;
    int locAmbientColor_ = -1;
    std::array<int, MaxLights> locEnabled_{};
    std::array<int, MaxLights> locType_{};
    std::array<int, MaxLights> locPosition_{};
    std::array<int, MaxLights> locDirection_{};
    std::array<int, MaxLights> locColor_{};
    std::array<int, MaxLights> locIntensity_{};
    std::array<int, MaxLights> locCutoff_{};
    std::array<int, MaxLights> locOuterCutoff_{};
};

} // namespace kalan
