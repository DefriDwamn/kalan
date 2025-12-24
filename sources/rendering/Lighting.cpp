#include "Lighting.hpp"
#include <format>

namespace kalan {

LightingSystem& LightingSystem::instance() noexcept {
    static LightingSystem inst;
    return inst;
}

void LightingSystem::init() {
    if (!PBRMaterial::isShaderLoaded()) {
        PBRMaterial::initShader();
    }
    
    Shader& shader = PBRMaterial::getShader();
    
    locLightCount_ = GetShaderLocation(shader, "lightCount");
    locAmbientColor_ = GetShaderLocation(shader, "ambientColor");
    
    for (int i = 0; i < MaxLights; ++i) {
        std::string prefix = std::format("lights[{}].", i);
        locEnabled_[i] = GetShaderLocation(shader, (prefix + "enabled").c_str());
        locType_[i] = GetShaderLocation(shader, (prefix + "type").c_str());
        locPosition_[i] = GetShaderLocation(shader, (prefix + "position").c_str());
        locDirection_[i] = GetShaderLocation(shader, (prefix + "direction").c_str());
        locColor_[i] = GetShaderLocation(shader, (prefix + "color").c_str());
        locIntensity_[i] = GetShaderLocation(shader, (prefix + "intensity").c_str());
        locCutoff_[i] = GetShaderLocation(shader, (prefix + "cutoff").c_str());
        locOuterCutoff_[i] = GetShaderLocation(shader, (prefix + "outerCutoff").c_str());
    }
}

int LightingSystem::addLight(const Light& light) {
    if (static_cast<int>(lights_.size()) >= MaxLights) {
        return -1;
    }
    lights_.push_back(light);
    return static_cast<int>(lights_.size()) - 1;
}

Light& LightingSystem::getLight(int index) {
    return lights_.at(index);
}

const Light& LightingSystem::getLight(int index) const {
    return lights_.at(index);
}

void LightingSystem::removeLight(int index) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_.erase(lights_.begin() + index);
    }
}

void LightingSystem::clearLights() {
    lights_.clear();
}

void LightingSystem::setAmbientColor(Color color) {
    ambientColor_ = {
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f
    };
}

void LightingSystem::setAmbientColor(Vector3 color) {
    ambientColor_ = color;
}

void LightingSystem::update(const raylib::Camera& camera) {
    Shader& shader = PBRMaterial::getShader();
    
    // View position
    Vector3 viewPos = camera.GetPosition();
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &viewPos, SHADER_UNIFORM_VEC3);
    
    // Ambient
    SetShaderValue(shader, locAmbientColor_, &ambientColor_, SHADER_UNIFORM_VEC3);
    
    // Light count
    int count = static_cast<int>(lights_.size());
    SetShaderValue(shader, locLightCount_, &count, SHADER_UNIFORM_INT);
    
    // Each light
    for (int i = 0; i < count && i < MaxLights; ++i) {
        const Light& light = lights_[i];
        
        int enabled = light.enabled ? 1 : 0;
        int type = static_cast<int>(light.type);
        Vector3 color = {
            light.color.r / 255.0f,
            light.color.g / 255.0f,
            light.color.b / 255.0f
        };
        
        // Конвертировать cutoff из градусов в косинус
        float cutoffCos = cosf(light.cutoff * DEG2RAD);
        float outerCutoffCos = cosf(light.outerCutoff * DEG2RAD);
        
        SetShaderValue(shader, locEnabled_[i], &enabled, SHADER_UNIFORM_INT);
        SetShaderValue(shader, locType_[i], &type, SHADER_UNIFORM_INT);
        SetShaderValue(shader, locPosition_[i], &light.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locDirection_[i], &light.direction, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locColor_[i], &color, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locIntensity_[i], &light.intensity, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, locCutoff_[i], &cutoffCos, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, locOuterCutoff_[i], &outerCutoffCos, SHADER_UNIFORM_FLOAT);
    }
    
    // Отключить неиспользуемые слоты
    for (int i = count; i < MaxLights; ++i) {
        int disabled = 0;
        SetShaderValue(shader, locEnabled_[i], &disabled, SHADER_UNIFORM_INT);
    }
}

} // namespace kalan
