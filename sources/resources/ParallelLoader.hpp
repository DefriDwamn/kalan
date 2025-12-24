#pragma once

#include "raylib-cpp.hpp"
#include <filesystem>
#include <vector>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <unordered_map>

namespace fs = std::filesystem;

namespace kalan {

// Предзагруженное изображение (в RAM, без GPU)
struct PreloadedImage {
    Image image{};
    std::string path;
    bool valid = false;
    
    PreloadedImage() = default;
    PreloadedImage(PreloadedImage&& other) noexcept 
        : image(other.image), path(std::move(other.path)), valid(other.valid) {
        other.image = {};
        other.valid = false;
    }
    PreloadedImage& operator=(PreloadedImage&& other) noexcept {
        if (this != &other) {
            if (valid && image.data) UnloadImage(image);
            image = other.image;
            path = std::move(other.path);
            valid = other.valid;
            other.image = {};
            other.valid = false;
        }
        return *this;
    }
    ~PreloadedImage() {
        if (valid && image.data) UnloadImage(image);
    }
    
    // Запрет копирования
    PreloadedImage(const PreloadedImage&) = delete;
    PreloadedImage& operator=(const PreloadedImage&) = delete;
};

// Thread pool для параллельного декодирования
class ImageThreadPool {
public:
    explicit ImageThreadPool(size_t threads = 0);
    ~ImageThreadPool();
    
    // Добавить задачу декодирования
    std::future<PreloadedImage> decodeAsync(const std::string& path);
    std::future<PreloadedImage> decodeFromMemoryAsync(
        std::shared_ptr<std::vector<unsigned char>> data, 
        const std::string& hint);
    
    // Ожидать завершения всех задач
    void waitAll();
    
    size_t getThreadCount() const { return workers_.size(); }
    size_t getPendingCount() const { return pendingCount_.load(); }

private:
    void workerLoop();
    
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> pendingCount_{0};
};

// Информация о декодированной текстуре
struct DecodedTexture {
    Image image{};
    int materialIndex = -1;
    int mapType = -1; // MATERIAL_MAP_* enum
    bool valid = false;
};

// Параллельный загрузчик моделей (использует Assimp)
class ParallelModelLoader {
public:
    struct LoadProgress {
        std::atomic<int> imagesDecoded{0};
        std::atomic<int> totalImages{0};
        std::atomic<int> texturesUploaded{0};
        std::atomic<bool> complete{false};
        
        float getDecodeProgress() const {
            int t = totalImages.load();
            return t > 0 ? static_cast<float>(imagesDecoded.load()) / t : 0.0f;
        }
        float getUploadProgress() const {
            int t = totalImages.load();
            return t > 0 ? static_cast<float>(texturesUploaded.load()) / t : 0.0f;
        }
    };

    static ParallelModelLoader& instance();
    
    // Загрузить модель с параллельным декодированием текстур через Assimp
    std::shared_ptr<raylib::Model> loadModel(
        const fs::path& modelPath,
        std::function<void(const LoadProgress&)> progressCallback = nullptr
    );
    
    // Установить количество потоков (по умолчанию = CPU cores)
    void setThreadCount(size_t count);

private:
    ParallelModelLoader();
    
    std::unique_ptr<ImageThreadPool> threadPool_;
    size_t threadCount_ = 0;
};

// Утилиты для GPU-ускоренных операций
namespace gpu {
    // Загрузить текстуру из Image с GPU-генерацией mipmaps
    Texture2D uploadTextureGPU(Image image, bool genMipmaps = true);
    
    // Batch загрузка нескольких текстур
    std::vector<Texture2D> uploadTexturesBatchGPU(std::vector<Image>& images, bool genMipmaps = true);
}

} // namespace kalan
