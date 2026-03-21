#pragma once

#include <vk_types.h>
#include <unordered_map>
#include <filesystem>

// forward declaration
class VulkanEngine;

struct GLTFMaterial
{
    MaterialInstance data;
};

struct GeoSurface // surface info for a draw call
{
    uint32_t startIndex;
    uint32_t count;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
    std::string name;
    std::vector<GeoSurface> surfaces; // sub-meshes of this specif ic mesh on its own draw for now
    GPUMeshBuffers meshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath);