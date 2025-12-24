#include "PBRMaterial.hpp"
#include <iostream>

namespace kalan {

// ============ PBRMaterial Static Methods ============

void PBRMaterial::initShader(const fs::path& vsPath, const fs::path& fsPath) {
    if (shaderLoaded_) return;
    
    shader_ = LoadShader(vsPath.string().c_str(), fsPath.string().c_str());
    
    // Привязка локаций текстур
    shader_.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shader_, "albedoMap");
    shader_.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader_, "normalMap");
    shader_.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(shader_, "metallicMap");
    shader_.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(shader_, "roughnessMap");
    shader_.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(shader_, "aoMap");
    
    // MVP и view
    shader_.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader_, "mvp");
    shader_.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader_, "matModel");
    shader_.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader_, "viewPos");
    
    shaderLoaded_ = true;
}

void PBRMaterial::initShader() {
    initShader("assets/shaders/pbr.vs", "assets/shaders/pbr.fs");
}

Shader& PBRMaterial::getShader() noexcept {
    return shader_;
}

bool PBRMaterial::isShaderLoaded() noexcept {
    return shaderLoaded_;
}

void PBRMaterial::initDefaults() {
    if (defaultsLoaded_) return;
    
    // Albedo: белый
    Image imgAlbedo = GenImageColor(1, 1, WHITE);
    defaultAlbedo_ = LoadTextureFromImage(imgAlbedo);
    UnloadImage(imgAlbedo);
    
    // Normal: нейтральный (0.5, 0.5, 1.0)
    Image imgNormal = GenImageColor(1, 1, {128, 128, 255, 255});
    defaultNormal_ = LoadTextureFromImage(imgNormal);
    UnloadImage(imgNormal);
    
    // Metallic: 0 (неметаллический)
    Image imgMetallic = GenImageColor(1, 1, BLACK);
    defaultMetallic_ = LoadTextureFromImage(imgMetallic);
    UnloadImage(imgMetallic);
    
    // Roughness: 1 (полностью шероховатый)
    Image imgRoughness = GenImageColor(1, 1, WHITE);
    defaultRoughness_ = LoadTextureFromImage(imgRoughness);
    UnloadImage(imgRoughness);
    
    // AO: 1 (нет окклюзии)
    Image imgAO = GenImageColor(1, 1, WHITE);
    defaultAO_ = LoadTextureFromImage(imgAO);
    UnloadImage(imgAO);
    
    defaultsLoaded_ = true;
}

Texture2D PBRMaterial::getDefaultAlbedo() {
    if (!defaultsLoaded_) initDefaults();
    return defaultAlbedo_;
}

Texture2D PBRMaterial::getDefaultNormal() {
    if (!defaultsLoaded_) initDefaults();
    return defaultNormal_;
}

Texture2D PBRMaterial::getDefaultMetallic() {
    if (!defaultsLoaded_) initDefaults();
    return defaultMetallic_;
}

Texture2D PBRMaterial::getDefaultRoughness() {
    if (!defaultsLoaded_) initDefaults();
    return defaultRoughness_;
}

Texture2D PBRMaterial::getDefaultAO() {
    if (!defaultsLoaded_) initDefaults();
    return defaultAO_;
}

// ============ PBRMaterial Instance Methods ============

void PBRMaterial::setTexture(PBRTextureType type, Texture2D tex) {
    textures_[static_cast<size_t>(type)] = tex;
}

void PBRMaterial::setTexture(PBRTextureType type, std::shared_ptr<raylib::Texture> tex) {
    if (tex) {
        textureOwners_[static_cast<size_t>(type)] = tex;
        textures_[static_cast<size_t>(type)] = *tex; // implicit conversion to Texture2D
    }
}

Texture2D PBRMaterial::getTexture(PBRTextureType type) const {
    auto tex = textures_[static_cast<size_t>(type)];
    if (tex.id == 0) {
        // Вернуть дефолт
        switch (type) {
            case PBRTextureType::Albedo:    return getDefaultAlbedo();
            case PBRTextureType::Normal:    return getDefaultNormal();
            case PBRTextureType::Metallic:  return getDefaultMetallic();
            case PBRTextureType::Roughness: return getDefaultRoughness();
            case PBRTextureType::AO:        return getDefaultAO();
            default: return getDefaultAlbedo();
        }
    }
    return tex;
}

Material PBRMaterial::toRaylibMaterial() const {
    Material mat = LoadMaterialDefault();
    mat.shader = shader_;
    
    mat.maps[MATERIAL_MAP_ALBEDO].texture = getTexture(PBRTextureType::Albedo);
    mat.maps[MATERIAL_MAP_NORMAL].texture = getTexture(PBRTextureType::Normal);
    mat.maps[MATERIAL_MAP_METALNESS].texture = getTexture(PBRTextureType::Metallic);
    mat.maps[MATERIAL_MAP_ROUGHNESS].texture = getTexture(PBRTextureType::Roughness);
    mat.maps[MATERIAL_MAP_OCCLUSION].texture = getTexture(PBRTextureType::AO);
    
    // Генерация mipmaps
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_ALBEDO].texture);
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_NORMAL].texture);
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_METALNESS].texture);
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_ROUGHNESS].texture);
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_OCCLUSION].texture);
    
    return mat;
}

void PBRMaterial::applyShaderToModel(raylib::Model& model) {
    if (!shaderLoaded_) return;
    
    for (int i = 0; i < model.GetMaterialCount(); ++i) {
        Material& mat = model.materials[i];
        
        // Применить PBR шейдер
        mat.shader = shader_;
        
        // Если текстуры отсутствуют — подставить дефолтные
        if (mat.maps[MATERIAL_MAP_ALBEDO].texture.id == 0) {
            mat.maps[MATERIAL_MAP_ALBEDO].texture = getDefaultAlbedo();
        }
        if (mat.maps[MATERIAL_MAP_NORMAL].texture.id == 0) {
            mat.maps[MATERIAL_MAP_NORMAL].texture = getDefaultNormal();
        }
        if (mat.maps[MATERIAL_MAP_METALNESS].texture.id == 0) {
            mat.maps[MATERIAL_MAP_METALNESS].texture = getDefaultMetallic();
        }
        if (mat.maps[MATERIAL_MAP_ROUGHNESS].texture.id == 0) {
            mat.maps[MATERIAL_MAP_ROUGHNESS].texture = getDefaultRoughness();
        }
        if (mat.maps[MATERIAL_MAP_OCCLUSION].texture.id == 0) {
            mat.maps[MATERIAL_MAP_OCCLUSION].texture = getDefaultAO();
        }
        
        // Генерация mipmaps для существующих текстур
        GenTextureMipmaps(&mat.maps[MATERIAL_MAP_ALBEDO].texture);
    }
}

// ============ PBRTextureLoader ============

std::optional<Texture2D> PBRTextureLoader::tryLoadTexture(
    const fs::path& basePath,
    const std::string& suffix,
    const std::vector<std::string>& extensions) 
{
    fs::path dir = basePath.parent_path();
    std::string stem = basePath.stem().string();
    
    // Попробовать найти: stem_suffix.ext, stem-suffix.ext, suffix.ext
    std::vector<std::string> prefixes = {
        stem + "_" + suffix,
        stem + "-" + suffix,
        suffix
    };
    
    for (const auto& prefix : prefixes) {
        for (const auto& ext : extensions) {
            fs::path candidate = dir / (prefix + ext);
            if (fs::exists(candidate)) {
                Texture2D tex = LoadTexture(candidate.string().c_str());
                if (tex.id != 0) return tex;
            }
        }
    }
    
    // Проверить подпапку textures/
    fs::path texDir = dir / "textures";
    if (fs::exists(texDir)) {
        for (const auto& prefix : prefixes) {
            for (const auto& ext : extensions) {
                fs::path candidate = texDir / (prefix + ext);
                if (fs::exists(candidate)) {
                    Texture2D tex = LoadTexture(candidate.string().c_str());
                    if (tex.id != 0) return tex;
                }
            }
        }
    }
    
    return std::nullopt;
}

PBRMaterial PBRTextureLoader::loadForModel(const fs::path& modelPath) {
    PBRMaterial mat;
    
    static const std::vector<std::string> exts = {".png", ".jpg", ".jpeg", ".tga", ".bmp"};
    
    // Albedo / Diffuse / BaseColor
    static const std::vector<std::string> albedoSuffixes = {"albedo", "diffuse", "basecolor", "base_color", "color"};
    for (const auto& suffix : albedoSuffixes) {
        if (auto tex = tryLoadTexture(modelPath, suffix, exts)) {
            mat.setTexture(PBRTextureType::Albedo, *tex);
            break;
        }
    }
    
    // Normal
    static const std::vector<std::string> normalSuffixes = {"normal", "norm", "nrm", "normalmap"};
    for (const auto& suffix : normalSuffixes) {
        if (auto tex = tryLoadTexture(modelPath, suffix, exts)) {
            mat.setTexture(PBRTextureType::Normal, *tex);
            break;
        }
    }
    
    // Metallic / Metalness
    static const std::vector<std::string> metallicSuffixes = {"metallic", "metal", "metalness"};
    for (const auto& suffix : metallicSuffixes) {
        if (auto tex = tryLoadTexture(modelPath, suffix, exts)) {
            mat.setTexture(PBRTextureType::Metallic, *tex);
            break;
        }
    }
    
    // Roughness
    static const std::vector<std::string> roughnessSuffixes = {"roughness", "rough"};
    for (const auto& suffix : roughnessSuffixes) {
        if (auto tex = tryLoadTexture(modelPath, suffix, exts)) {
            mat.setTexture(PBRTextureType::Roughness, *tex);
            break;
        }
    }
    
    // AO (Ambient Occlusion)
    static const std::vector<std::string> aoSuffixes = {"ao", "occlusion", "ambient_occlusion", "ambientocclusion"};
    for (const auto& suffix : aoSuffixes) {
        if (auto tex = tryLoadTexture(modelPath, suffix, exts)) {
            mat.setTexture(PBRTextureType::AO, *tex);
            break;
        }
    }
    
    return mat;
}

PBRMaterial PBRTextureLoader::extractFromModel(const raylib::Model& model) {
    PBRMaterial mat;
    
    if (model.GetMaterialCount() > 0) {
        const Material& srcMat = model.materials[0];
        
        // Извлекаем текстуры из первого материала модели (обычно встроены в glb)
        if (srcMat.maps[MATERIAL_MAP_ALBEDO].texture.id != 0) {
            mat.setTexture(PBRTextureType::Albedo, srcMat.maps[MATERIAL_MAP_ALBEDO].texture);
        }
        if (srcMat.maps[MATERIAL_MAP_NORMAL].texture.id != 0) {
            mat.setTexture(PBRTextureType::Normal, srcMat.maps[MATERIAL_MAP_NORMAL].texture);
        }
        if (srcMat.maps[MATERIAL_MAP_METALNESS].texture.id != 0) {
            mat.setTexture(PBRTextureType::Metallic, srcMat.maps[MATERIAL_MAP_METALNESS].texture);
        }
        if (srcMat.maps[MATERIAL_MAP_ROUGHNESS].texture.id != 0) {
            mat.setTexture(PBRTextureType::Roughness, srcMat.maps[MATERIAL_MAP_ROUGHNESS].texture);
        }
        if (srcMat.maps[MATERIAL_MAP_OCCLUSION].texture.id != 0) {
            mat.setTexture(PBRTextureType::AO, srcMat.maps[MATERIAL_MAP_OCCLUSION].texture);
        }
    }
    
    return mat;
}

} // namespace kalan
