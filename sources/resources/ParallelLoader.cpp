#include "ParallelLoader.hpp"
#include "../rendering/PBRMaterial.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace kalan {

// ============ ImageThreadPool ============

ImageThreadPool::ImageThreadPool(size_t threads) {
    if (threads == 0) {
        threads = std::max(1u, std::thread::hardware_concurrency());
    }
    
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back(&ImageThreadPool::workerLoop, this);
    }
}

ImageThreadPool::~ImageThreadPool() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

void ImageThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            
            if (stop_ && tasks_.empty()) return;
            
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        
        task();
        --pendingCount_;
    }
}

std::future<PreloadedImage> ImageThreadPool::decodeAsync(const std::string& path) {
    auto promise = std::make_shared<std::promise<PreloadedImage>>();
    auto future = promise->get_future();
    
    {
        std::lock_guard lock(mutex_);
        ++pendingCount_;
        
        tasks_.push([promise, path]() {
            PreloadedImage result;
            result.path = path;
            
            // Декодирование в рабочем потоке (без OpenGL!)
            result.image = LoadImage(path.c_str());
            result.valid = (result.image.data != nullptr);
            
            promise->set_value(std::move(result));
        });
    }
    
    cv_.notify_one();
    return future;
}

std::future<PreloadedImage> ImageThreadPool::decodeFromMemoryAsync(
    std::shared_ptr<std::vector<unsigned char>> data,
    const std::string& hint) 
{
    auto promise = std::make_shared<std::promise<PreloadedImage>>();
    auto future = promise->get_future();
    
    {
        std::lock_guard lock(mutex_);
        ++pendingCount_;
        
        tasks_.push([promise, data, hint]() {
            PreloadedImage result;
            result.path = hint;
            
            // Определяем формат по hint или пробуем PNG/JPG
            const char* ext = ".png";
            if (hint.find(".jpg") != std::string::npos || 
                hint.find(".jpeg") != std::string::npos) {
                ext = ".jpg";
            } else if (hint.find(".png") != std::string::npos) {
                ext = ".png";
            } else if (hint.find(".tga") != std::string::npos) {
                ext = ".tga";
            } else if (hint.find(".bmp") != std::string::npos) {
                ext = ".bmp";
            }
            
            result.image = LoadImageFromMemory(ext, data->data(), 
                                                static_cast<int>(data->size()));
            result.valid = (result.image.data != nullptr);
            
            promise->set_value(std::move(result));
        });
    }
    
    cv_.notify_one();
    return future;
}

void ImageThreadPool::waitAll() {
    while (pendingCount_.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ============ GPU Utilities ============

namespace gpu {

Texture2D uploadTextureGPU(Image image, bool genMipmaps) {
    if (!image.data) return {0};
    
    // Загрузить текстуру в GPU
    Texture2D texture = LoadTextureFromImage(image);
    
    if (genMipmaps && texture.id != 0) {
        // GPU-ускоренная генерация mipmaps через raylib
        GenTextureMipmaps(&texture);
        
        // Установить фильтрацию для mipmaps
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    }
    
    return texture;
}

std::vector<Texture2D> uploadTexturesBatchGPU(std::vector<Image>& images, bool genMipmaps) {
    std::vector<Texture2D> textures;
    textures.reserve(images.size());
    
    for (auto& img : images) {
        textures.push_back(uploadTextureGPU(img, genMipmaps));
    }
    
    return textures;
}

} // namespace gpu

// ============ Assimp to Raylib conversion helpers ============

namespace {

// Конвертация aiMatrix4x4 в raylib Matrix
Matrix ConvertAssimpMatrix(const aiMatrix4x4& m) {
    // Assimp использует row-major, raylib использует column-major
    return Matrix{
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    };
}

// Трансформировать вершины меша по матрице
void TransformMeshVertices(Mesh& mesh, const Matrix& transform) {
    // Вычисляем нормальную матрицу (для нормалей)
    Matrix normalMatrix = MatrixTranspose(MatrixInvert(transform));
    
    for (int i = 0; i < mesh.vertexCount; ++i) {
        // Трансформируем позицию
        Vector3 pos = {
            mesh.vertices[i*3 + 0],
            mesh.vertices[i*3 + 1],
            mesh.vertices[i*3 + 2]
        };
        pos = Vector3Transform(pos, transform);
        mesh.vertices[i*3 + 0] = pos.x;
        mesh.vertices[i*3 + 1] = pos.y;
        mesh.vertices[i*3 + 2] = pos.z;
        
        // Трансформируем нормали
        if (mesh.normals) {
            Vector3 normal = {
                mesh.normals[i*3 + 0],
                mesh.normals[i*3 + 1],
                mesh.normals[i*3 + 2]
            };
            normal = Vector3Normalize(Vector3Transform(normal, normalMatrix));
            mesh.normals[i*3 + 0] = normal.x;
            mesh.normals[i*3 + 1] = normal.y;
            mesh.normals[i*3 + 2] = normal.z;
        }
        
        // Трансформируем тангенты
        if (mesh.tangents) {
            Vector3 tangent = {
                mesh.tangents[i*4 + 0],
                mesh.tangents[i*4 + 1],
                mesh.tangents[i*4 + 2]
            };
            tangent = Vector3Normalize(Vector3Transform(tangent, normalMatrix));
            mesh.tangents[i*4 + 0] = tangent.x;
            mesh.tangents[i*4 + 1] = tangent.y;
            mesh.tangents[i*4 + 2] = tangent.z;
        }
    }
}

// Конвертация aiMesh в raylib Mesh (без upload)
Mesh ConvertAssimpMesh(const aiMesh* aiM) {
    Mesh mesh = {0};
    
    mesh.vertexCount = aiM->mNumVertices;
    mesh.triangleCount = aiM->mNumFaces;
    
    // Vertices
    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    for (unsigned int i = 0; i < aiM->mNumVertices; i++) {
        mesh.vertices[i*3 + 0] = aiM->mVertices[i].x;
        mesh.vertices[i*3 + 1] = aiM->mVertices[i].y;
        mesh.vertices[i*3 + 2] = aiM->mVertices[i].z;
    }
    
    // Normals
    if (aiM->HasNormals()) {
        mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
        for (unsigned int i = 0; i < aiM->mNumVertices; i++) {
            mesh.normals[i*3 + 0] = aiM->mNormals[i].x;
            mesh.normals[i*3 + 1] = aiM->mNormals[i].y;
            mesh.normals[i*3 + 2] = aiM->mNormals[i].z;
        }
    }
    
    // Tangents
    if (aiM->HasTangentsAndBitangents()) {
        mesh.tangents = (float*)MemAlloc(mesh.vertexCount * 4 * sizeof(float));
        for (unsigned int i = 0; i < aiM->mNumVertices; i++) {
            mesh.tangents[i*4 + 0] = aiM->mTangents[i].x;
            mesh.tangents[i*4 + 1] = aiM->mTangents[i].y;
            mesh.tangents[i*4 + 2] = aiM->mTangents[i].z;
            mesh.tangents[i*4 + 3] = 1.0f; // w component
        }
    }
    
    // Texture coordinates
    if (aiM->HasTextureCoords(0)) {
        mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
        for (unsigned int i = 0; i < aiM->mNumVertices; i++) {
            mesh.texcoords[i*2 + 0] = aiM->mTextureCoords[0][i].x;
            mesh.texcoords[i*2 + 1] = aiM->mTextureCoords[0][i].y;
        }
    }
    
    // Vertex colors
    if (aiM->HasVertexColors(0)) {
        mesh.colors = (unsigned char*)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));
        for (unsigned int i = 0; i < aiM->mNumVertices; i++) {
            mesh.colors[i*4 + 0] = (unsigned char)(aiM->mColors[0][i].r * 255);
            mesh.colors[i*4 + 1] = (unsigned char)(aiM->mColors[0][i].g * 255);
            mesh.colors[i*4 + 2] = (unsigned char)(aiM->mColors[0][i].b * 255);
            mesh.colors[i*4 + 3] = (unsigned char)(aiM->mColors[0][i].a * 255);
        }
    }
    
    // Indices
    mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));
    for (unsigned int i = 0; i < aiM->mNumFaces; i++) {
        const aiFace& face = aiM->mFaces[i];
        if (face.mNumIndices == 3) {
            mesh.indices[i*3 + 0] = (unsigned short)face.mIndices[0];
            mesh.indices[i*3 + 1] = (unsigned short)face.mIndices[1];
            mesh.indices[i*3 + 2] = (unsigned short)face.mIndices[2];
        }
    }
    
    // НЕ делаем UploadMesh здесь - сначала применим трансформации
    return mesh;
}

// Информация о меше с его трансформацией
struct MeshInstance {
    unsigned int meshIndex;
    Matrix transform;
};

// Рекурсивный обход дерева узлов для сбора мешей с трансформациями
void CollectMeshInstances(
    const aiNode* node, 
    const Matrix& parentTransform,
    std::vector<MeshInstance>& instances) 
{
    // Комбинируем трансформацию родителя с локальной
    Matrix localTransform = ConvertAssimpMatrix(node->mTransformation);
    Matrix globalTransform = MatrixMultiply(localTransform, parentTransform);
    
    // Добавляем все меши этого узла
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        instances.push_back({node->mMeshes[i], globalTransform});
    }
    
    // Рекурсивно обходим детей
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        CollectMeshInstances(node->mChildren[i], globalTransform, instances);
    }
}

// Получить путь к текстуре из aiMaterial
std::string GetTexturePath(const aiMaterial* mat, aiTextureType type, const fs::path& modelDir) {
    if (mat->GetTextureCount(type) > 0) {
        aiString texPath;
        if (mat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
            std::string path = texPath.C_Str();
            // Если путь относительный, добавляем директорию модели
            if (!fs::path(path).is_absolute()) {
                return (modelDir / path).string();
            }
            return path;
        }
    }
    return "";
}

// Информация о текстуре для загрузки
struct TextureLoadInfo {
    std::string path;
    const aiTexture* embedded = nullptr; // Для embedded текстур
    int materialIndex;
    int mapType; // MATERIAL_MAP_* enum
};

// Маппинг aiTextureType -> raylib MATERIAL_MAP_*
int AssimpToRaylibMapType(aiTextureType type) {
    switch (type) {
        case aiTextureType_DIFFUSE:
        case aiTextureType_BASE_COLOR:
            return MATERIAL_MAP_ALBEDO;
        case aiTextureType_NORMALS:
        case aiTextureType_NORMAL_CAMERA:
            return MATERIAL_MAP_NORMAL;
        case aiTextureType_METALNESS:
            return MATERIAL_MAP_METALNESS;
        case aiTextureType_DIFFUSE_ROUGHNESS:
            return MATERIAL_MAP_ROUGHNESS;
        case aiTextureType_AMBIENT_OCCLUSION:
        case aiTextureType_LIGHTMAP:
            return MATERIAL_MAP_OCCLUSION;
        case aiTextureType_EMISSIVE:
        case aiTextureType_EMISSION_COLOR:
            return MATERIAL_MAP_EMISSION;
        default:
            return -1;
    }
}

} // anonymous namespace

// ============ ParallelModelLoader ============

ParallelModelLoader& ParallelModelLoader::instance() {
    static ParallelModelLoader inst;
    return inst;
}

ParallelModelLoader::ParallelModelLoader() 
    : threadCount_(std::max(1u, std::thread::hardware_concurrency())) {}

void ParallelModelLoader::setThreadCount(size_t count) {
    threadCount_ = std::max(size_t(1), count);
    threadPool_.reset();
}

std::shared_ptr<raylib::Model> ParallelModelLoader::loadModel(
    const fs::path& modelPath,
    std::function<void(const LoadProgress&)> progressCallback) 
{
    LoadProgress progress;
    
    // Создаём thread pool если нужно
    if (!threadPool_) {
        threadPool_ = std::make_unique<ImageThreadPool>(threadCount_);
    }
    
    fs::path modelDir = modelPath.parent_path();
    
    // ========== ШАГ 1: Загрузка модели через Assimp ==========
    Assimp::Importer importer;
    
    unsigned int flags = 
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_FlipUVs |
        aiProcess_OptimizeMeshes |
        aiProcess_PreTransformVertices; // Применяет все трансформации к вершинам
    
    const aiScene* scene = importer.ReadFile(modelPath.string(), flags);
    
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "ParallelModelLoader: Failed to load " << modelPath 
                  << ": " << importer.GetErrorString() << "\n";
        return nullptr;
    }
    
    // ========== ШАГ 2: Собираем информацию о текстурах для параллельной загрузки ==========
    std::vector<TextureLoadInfo> texturesToLoad;
    
    // Отслеживаем какие слоты материалов уже заполнены
    std::unordered_set<uint64_t> loadedSlots;
    auto makeSlotKey = [](int matIdx, int mapType) -> uint64_t {
        return (static_cast<uint64_t>(matIdx) << 32) | static_cast<uint64_t>(mapType);
    };
    
    // Текстуры, которые нужно загрузить для каждого типа
    // Приоритет: PBR типы (BASE_COLOR) перед legacy (DIFFUSE)
    const std::vector<aiTextureType> textureTypes = {
        aiTextureType_BASE_COLOR,      // PBR albedo (приоритет)
        aiTextureType_DIFFUSE,         // Legacy diffuse
        aiTextureType_NORMALS,
        aiTextureType_NORMAL_CAMERA,
        aiTextureType_METALNESS,
        aiTextureType_DIFFUSE_ROUGHNESS,
        aiTextureType_AMBIENT_OCCLUSION,
        aiTextureType_LIGHTMAP,
        aiTextureType_EMISSIVE,
        aiTextureType_EMISSION_COLOR
    };
    
    TraceLog(LOG_INFO, "ParallelModelLoader: %d materials, %d embedded textures", 
             scene->mNumMaterials, scene->mNumTextures);
    
    for (unsigned int matIdx = 0; matIdx < scene->mNumMaterials; ++matIdx) {
        const aiMaterial* aiMat = scene->mMaterials[matIdx];
        
        for (aiTextureType type : textureTypes) {
            int raylibMap = AssimpToRaylibMapType(type);
            if (raylibMap < 0) continue;
            
            // Проверяем, не загружена ли уже текстура для этого слота
            uint64_t slotKey = makeSlotKey(matIdx, raylibMap);
            if (loadedSlots.count(slotKey) > 0) continue;
            
            if (aiMat->GetTextureCount(type) > 0) {
                aiString texPath;
                if (aiMat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
                    TextureLoadInfo info;
                    info.materialIndex = matIdx;
                    info.mapType = raylibMap;
                    
                    std::string pathStr = texPath.C_Str();
                    
                    // Проверяем, embedded ли текстура
                    const aiTexture* embTex = scene->GetEmbeddedTexture(texPath.C_Str());
                    if (embTex) {
                        info.embedded = embTex;
                        info.path = pathStr;
                        TraceLog(LOG_INFO, "  Material %d, map %d: embedded (%dx%d, %s)", 
                                 matIdx, raylibMap, embTex->mWidth, embTex->mHeight, embTex->achFormatHint);
                    } else {
                        info.path = (modelDir / pathStr).string();
                        TraceLog(LOG_INFO, "  Material %d, map %d: %s", matIdx, raylibMap, info.path.c_str());
                    }
                    
                    texturesToLoad.push_back(info);
                    loadedSlots.insert(slotKey);
                }
            }
        }
    }
    
    TraceLog(LOG_INFO, "ParallelModelLoader: %zu textures to load", texturesToLoad.size());
    if (progressCallback) progressCallback(progress);
    
    // ========== ШАГ 3: Параллельное декодирование текстур ==========
    struct TextureFuture {
        std::future<PreloadedImage> future;
        int materialIndex;
        int mapType;
    };
    std::vector<TextureFuture> futures;
    
    for (const auto& texInfo : texturesToLoad) {
        TextureFuture tf;
        tf.materialIndex = texInfo.materialIndex;
        tf.mapType = texInfo.mapType;
        
        if (texInfo.embedded) {
            // Embedded текстура
            const aiTexture* embTex = texInfo.embedded;
            
            if (embTex->mHeight == 0) {
                // Сжатый формат (PNG/JPG и т.д.)
                auto data = std::make_shared<std::vector<unsigned char>>(
                    reinterpret_cast<const unsigned char*>(embTex->pcData),
                    reinterpret_cast<const unsigned char*>(embTex->pcData) + embTex->mWidth
                );
                // achFormatHint это "jpg", "png" и т.д. без точки
                std::string hint = ".";
                hint += embTex->achFormatHint;
                tf.future = threadPool_->decodeFromMemoryAsync(data, hint);
            } else {
                // Raw RGBA данные - создаём Image напрямую
                auto data = std::make_shared<std::vector<unsigned char>>(
                    embTex->mWidth * embTex->mHeight * 4
                );
                const aiTexel* texels = embTex->pcData;
                for (unsigned int i = 0; i < embTex->mWidth * embTex->mHeight; ++i) {
                    // aiTexel хранит BGRA, конвертируем в RGBA
                    (*data)[i*4 + 0] = texels[i].r;
                    (*data)[i*4 + 1] = texels[i].g;
                    (*data)[i*4 + 2] = texels[i].b;
                    (*data)[i*4 + 3] = texels[i].a;
                }
                // Для raw данных создаём promise напрямую
                auto promise = std::make_shared<std::promise<PreloadedImage>>();
                tf.future = promise->get_future();
                
                PreloadedImage result;
                result.path = "<raw>";
                result.image.data = MemAlloc(data->size());
                memcpy(result.image.data, data->data(), data->size());
                result.image.width = embTex->mWidth;
                result.image.height = embTex->mHeight;
                result.image.mipmaps = 1;
                result.image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
                result.valid = true;
                
                promise->set_value(std::move(result));
            }
        } else {
            // Внешний файл
            tf.future = threadPool_->decodeAsync(texInfo.path);
        }
        
        futures.push_back(std::move(tf));
    }
    
    // ========== ШАГ 4: Создаём raylib Model со всеми mesh ==========
    Model model = {0};
    model.transform = MatrixIdentity();
    
    // Meshes
    model.meshCount = scene->mNumMeshes;
    model.meshes = (Mesh*)MemAlloc(model.meshCount * sizeof(Mesh));
    model.meshMaterial = (int*)MemAlloc(model.meshCount * sizeof(int));
    
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        model.meshes[i] = ConvertAssimpMesh(scene->mMeshes[i]);
        model.meshMaterial[i] = scene->mMeshes[i]->mMaterialIndex;
        
        // Upload to GPU после конвертации
        UploadMesh(&model.meshes[i], false);
    }
    
    // Materials - создаём дефолтные
    model.materialCount = scene->mNumMaterials;
    model.materials = (Material*)MemAlloc(model.materialCount * sizeof(Material));
    
    for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i] = LoadMaterialDefault();
        
        // Читаем базовые свойства из aiMaterial
        const aiMaterial* aiMat = scene->mMaterials[i];
        
        aiColor4D diffuse;
        if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS) {
            model.materials[i].maps[MATERIAL_MAP_ALBEDO].color = {
                (unsigned char)(diffuse.r * 255),
                (unsigned char)(diffuse.g * 255),
                (unsigned char)(diffuse.b * 255),
                (unsigned char)(diffuse.a * 255)
            };
        }
        
        float metallic = 0.0f;
        aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &metallic);
        model.materials[i].maps[MATERIAL_MAP_METALNESS].value = metallic;
        
        float roughness = 1.0f;
        aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness);
        model.materials[i].maps[MATERIAL_MAP_ROUGHNESS].value = roughness;
    }
    
    // ========== ШАГ 5: Собираем декодированные текстуры и загружаем в GPU ==========
    int successCount = 0;
    int failCount = 0;
    
    for (auto& tf : futures) {
        PreloadedImage img = tf.future.get();
        ++progress.imagesDecoded;
        
        if (img.valid && img.image.data) {
            // Загружаем в GPU с mipmaps
            Texture2D tex = gpu::uploadTextureGPU(img.image, true);
            
            if (tex.id != 0 && tex.id != 1 && tf.materialIndex < model.materialCount) {
                // Выгружаем старую текстуру если была (но не дефолтную)
                Texture2D& oldTex = model.materials[tf.materialIndex].maps[tf.mapType].texture;
                if (oldTex.id > 1) {
                    UnloadTexture(oldTex);
                }
                model.materials[tf.materialIndex].maps[tf.mapType].texture = tex;
                
                // Для albedo сбрасываем цвет на белый, чтобы текстура отображалась корректно
                if (tf.mapType == MATERIAL_MAP_ALBEDO) {
                    model.materials[tf.materialIndex].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
                }
                
                ++successCount;
                TraceLog(LOG_DEBUG, "  Texture mat %d map %d: %dx%d -> ID %d", 
                         tf.materialIndex, tf.mapType, img.image.width, img.image.height, tex.id);
            } else {
                ++failCount;
                TraceLog(LOG_WARNING, "  FAIL upload mat %d map %d: tex.id=%d", 
                         tf.materialIndex, tf.mapType, tex.id);
            }
        } else {
            ++failCount;
            TraceLog(LOG_WARNING, "  FAIL decode mat %d map %d: path=%s", 
                     tf.materialIndex, tf.mapType, img.path.c_str());
        }
        
        ++progress.texturesUploaded;
        if (progressCallback) progressCallback(progress);
    }
    
    TraceLog(LOG_INFO, "ParallelModelLoader: %d textures loaded, %d failed", successCount, failCount);
    
    // ========== ШАГ 6: Применяем PBR шейдер ==========
    if (PBRMaterial::isShaderLoaded()) {
        for (int i = 0; i < model.materialCount; ++i) {
            model.materials[i].shader = PBRMaterial::getShader();
        }
    }
    
    progress.complete = true;
    if (progressCallback) progressCallback(progress);
    
    // Оборачиваем в shared_ptr с кастомным deleter
    auto result = std::shared_ptr<raylib::Model>(
        new raylib::Model(model),
        [](raylib::Model* m) {
            // raylib::Model деструктор сам выгрузит ресурсы
            delete m;
        }
    );
    
    return result;
}

} // namespace kalan
