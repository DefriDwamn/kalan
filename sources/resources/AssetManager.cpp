#include "resources/AssetManager.hpp"
#include "rendering/PBRMaterial.hpp"

#include <iostream>

namespace kalan {

AssetManager& AssetManager::instance() noexcept {
    static AssetManager inst;
    return inst;
}

void AssetManager::setAssetsRoot(fs::path root) noexcept {
    std::lock_guard lock(mutex_);
    assetsRoot_ = std::move(root);
}

fs::path AssetManager::getAssetsRoot() const noexcept {
    std::lock_guard lock(mutex_);
    return assetsRoot_;
}

void AssetManager::setPostLoadModelHook(std::function<void(raylib::Model&, const fs::path&)> hook) noexcept {
    std::lock_guard lock(mutex_);
    postLoadModelHook_ = std::move(hook);
}

void AssetManager::enableAutoPBR(bool enable) noexcept {
    std::lock_guard lock(mutex_);
    autoPBR_ = enable;
}

bool AssetManager::isAutoPBREnabled() const noexcept {
    std::lock_guard lock(mutex_);
    return autoPBR_;
}

static std::optional<fs::path> findWithExts(const fs::path& base, const std::vector<const char*>& exts) {
    if (base.has_extension() && fs::exists(base)) return fs::weakly_canonical(base);
    for (auto e : exts) {
        fs::path p = base;
        p += e;
        if (fs::exists(p)) return fs::weakly_canonical(p);
    }
    return std::nullopt;
}

std::optional<fs::path> AssetManager::resolveModelPath(const fs::path& pathOrName) const {
    if (pathOrName.is_absolute()) {
        if (fs::exists(pathOrName)) return fs::weakly_canonical(pathOrName);
        return std::nullopt;
    }
    fs::path base = assetsRoot_ / "models" / pathOrName;
    return findWithExts(base, modelExts_);
}

std::optional<fs::path> AssetManager::resolveTexturePath(const fs::path& pathOrName) const {
    if (pathOrName.is_absolute()) {
        if (fs::exists(pathOrName)) return fs::weakly_canonical(pathOrName);
        return std::nullopt;
    }
    fs::path base = assetsRoot_ / "textures" / pathOrName;
    return findWithExts(base, textureExts_);
}

std::optional<fs::path> AssetManager::resolveSoundPath(const fs::path& pathOrName) const {
    if (pathOrName.is_absolute()) {
        if (fs::exists(pathOrName)) return fs::weakly_canonical(pathOrName);
        return std::nullopt;
    }
    fs::path base = assetsRoot_ / "sounds" / pathOrName;
    return findWithExts(base, soundExts_);
}

template<typename T>
std::shared_ptr<T> AssetManager::loadCached(std::unordered_map<std::string, std::weak_ptr<T>>& cache, const fs::path& absolutePath) {
    auto key = fs::weakly_canonical(absolutePath).string();

    if (auto it = cache.find(key); it != cache.end()) {
        if (auto sp = it->second.lock()) return sp;
    }

    try {
        auto sp = std::make_shared<T>(key);
        cache[key] = sp;
        return sp;
    } catch (const std::exception& e) {
        std::cerr << "AssetManager: failed to load " << key << ": " << e.what() << "\n";
        return nullptr;
    } catch (...) {
        std::cerr << "AssetManager: unknown error loading " << key << "\n";
        return nullptr;
    }
}

std::shared_ptr<raylib::Model> AssetManager::getModel(fs::path pathOrName) {
    auto resolved = resolveModelPath(pathOrName);
    if (!resolved) return nullptr;

    std::lock_guard lock(mutex_);
    auto key = resolved->string();

    if (auto it = modelCache_.find(key); it != modelCache_.end()) {
        if (auto sp = it->second.lock()) return sp;
    }

    auto model = loadCached<raylib::Model>(modelCache_, *resolved);
    if (model) {
        // Применить PBR текстуры если включено
        if (autoPBR_) {
            applyPBRToModel(*model, *resolved);
        }
        // Вызвать пользовательский хук
        if (postLoadModelHook_) {
            postLoadModelHook_(*model, *resolved);
        }
    }
    return model;
}

void AssetManager::applyPBRToModel(raylib::Model& model, const fs::path& modelPath) {
    // Применить PBR шейдер к модели, сохранив её оригинальные текстуры
    PBRMaterial::applyShaderToModel(model);
}

std::shared_ptr<raylib::Texture> AssetManager::getTexture(fs::path pathOrName) {
    auto resolved = resolveTexturePath(pathOrName);
    if (!resolved) return nullptr;

    std::lock_guard lock(mutex_);
    return loadCached<raylib::Texture>(textureCache_, *resolved);
}

std::shared_ptr<raylib::Sound> AssetManager::getSound(fs::path pathOrName) {
    auto resolved = resolveSoundPath(pathOrName);
    if (!resolved) return nullptr;

    std::lock_guard lock(mutex_);
    return loadCached<raylib::Sound>(soundCache_, *resolved);
}

void AssetManager::clearCache() noexcept {
    std::lock_guard lock(mutex_);
    modelCache_.clear();
    textureCache_.clear();
    soundCache_.clear();
}

// Explicit template instantiations not necessary here

} // namespace kalan
