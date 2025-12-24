#pragma once

#include "raylib-cpp.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace kalan {

class AssetManager {
public:
  static AssetManager &instance() noexcept;

  void setAssetsRoot(fs::path root) noexcept;
  [[nodiscard]] fs::path getAssetsRoot() const noexcept;

  // Установить хук после загрузки модели (например, для применения PBR
  // материала)
  void setPostLoadModelHook(
      std::function<void(raylib::Model &, const fs::path &)> hook) noexcept;

  // Включить автоматическое применение PBR материалов при загрузке моделей
  void enableAutoPBR(bool enable = true) noexcept;
  [[nodiscard]] bool isAutoPBREnabled() const noexcept;

  [[nodiscard]] std::shared_ptr<raylib::Model> getModel(fs::path pathOrName);
  [[nodiscard]] std::shared_ptr<raylib::Texture>
  getTexture(fs::path pathOrName);
  [[nodiscard]] std::shared_ptr<raylib::Sound> getSound(fs::path pathOrName);

  void clearCache() noexcept;

private:
  AssetManager() = default;

  std::optional<fs::path> resolveModelPath(const fs::path &pathOrName) const;
  std::optional<fs::path> resolveTexturePath(const fs::path &pathOrName) const;
  std::optional<fs::path> resolveSoundPath(const fs::path &pathOrName) const;

  template <typename T>
  std::shared_ptr<T>
  loadCached(std::unordered_map<std::string, std::weak_ptr<T>> &cache,
             const fs::path &absolutePath);

  void applyPBRToModel(raylib::Model &model, const fs::path &modelPath);

  mutable std::mutex mutex_;
  fs::path assetsRoot_{"./assets"};
  std::function<void(raylib::Model &, const fs::path &)> postLoadModelHook_;
  bool autoPBR_ = false;

  std::unordered_map<std::string, std::weak_ptr<raylib::Model>> modelCache_;
  std::unordered_map<std::string, std::weak_ptr<raylib::Texture>> textureCache_;
  std::unordered_map<std::string, std::weak_ptr<raylib::Sound>> soundCache_;

  const std::vector<const char *> modelExts_{".glb", ".gltf", ".obj"};
  const std::vector<const char *> textureExts_{".png", ".jpg", ".bmp", ".tga"};
  const std::vector<const char *> soundExts_{".wav", ".ogg", ".mp3"};
};

} // namespace kalan
