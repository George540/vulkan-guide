#pragma once

#include <vk_types.h>
#include <unordered_map>
#include <filesystem>

#include "vk_descriptors.h"

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

struct LoadedGLTF : public IRenderable
{
    // Storage for all the data of a given glTF file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    // Nodes that don't have a parent, for iterating through the file in tree order
    std::vector<std::shared_ptr<Node>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;

    VulkanEngine* creator;

    ~LoadedGLTF() { clearAll(); }

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:
    void clearAll();
};

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine, std::string_view filePath);