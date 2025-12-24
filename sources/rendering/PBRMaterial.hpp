#pragma once

#include "raylib-cpp.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <array>

namespace fs = std::filesystem;

namespace kalan {

// Типы PBR текстур
enum class PBRTextureType : size_t {
    Albedo = 0,
    Normal,
    Metallic,
    Roughness,
    AO,
    Count
};

// PBR материал — хранит текстуры и применяет шейдер
class PBRMaterial {
public:
    static constexpr size_t TextureCount = static_cast<size_t>(PBRTextureType::Count);
    
    // Инициализация глобального PBR шейдера (вызвать один раз после InitWindow)
    static void initShader(const fs::path& vsPath, const fs::path& fsPath);
    static void initShader(); // использует дефолтные пути assets/shaders/pbr.vs, pbr.fs
    static Shader& getShader() noexcept;
    static bool isShaderLoaded() noexcept;
    
    // Дефолтные текстуры (1x1 пиксель)
    static void initDefaults();
    static Texture2D getDefaultAlbedo();
    static Texture2D getDefaultNormal();
    static Texture2D getDefaultMetallic();
    static Texture2D getDefaultRoughness();
    static Texture2D getDefaultAO();
    
    PBRMaterial() = default;
    
    // Устанавливает текстуру определённого типа
    void setTexture(PBRTextureType type, Texture2D tex);
    void setTexture(PBRTextureType type, std::shared_ptr<raylib::Texture> tex);
    
    // Получить текстуру
    [[nodiscard]] Texture2D getTexture(PBRTextureType type) const;
    
    // Применить только шейдер к модели, сохранив её оригинальные текстуры
    static void applyShaderToModel(raylib::Model& model);
    
    // Создать Material из текстур
    [[nodiscard]] Material toRaylibMaterial() const;

private:
    std::array<Texture2D, TextureCount> textures_{};
    std::array<std::shared_ptr<raylib::Texture>, TextureCount> textureOwners_{}; // shared ownership
    
    static inline Shader shader_{};
    static inline bool shaderLoaded_ = false;
    
    static inline Texture2D defaultAlbedo_{};
    static inline Texture2D defaultNormal_{};
    static inline Texture2D defaultMetallic_{};
    static inline Texture2D defaultRoughness_{};
    static inline Texture2D defaultAO_{};
    static inline bool defaultsLoaded_ = false;
};

// Автоматический загрузчик PBR текстур по имени модели
// Ищет текстуры в той же папке, что и модель:
//   model_albedo.png, model_normal.png, model_metallic.png, model_roughness.png, model_ao.png
// Или в подпапке textures/
class PBRTextureLoader {
public:
    // Загружает текстуры для модели, возвращает заполненный PBRMaterial
    // modelPath — путь к .glb/.gltf/.obj файлу
    [[nodiscard]] static PBRMaterial loadForModel(const fs::path& modelPath);
    
    // Попытаться извлечь текстуры из уже загруженной модели (встроенные в glb)
    [[nodiscard]] static PBRMaterial extractFromModel(const raylib::Model& model);
    
private:
    static std::optional<Texture2D> tryLoadTexture(const fs::path& basePath, 
                                                    const std::string& suffix,
                                                    const std::vector<std::string>& extensions);
};

} // namespace kalan
