#include "stb_image.h"
#include <iostream>
#include <vk_loader.h>

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine,
    std::filesystem::path filePath)
{
    // 1. Opening mesh using gLTF
    std::cout << "Loading gLTF mesh: " << filePath << std::endl;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser {};

    auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);

    if (load)
    {
        gltf = std::move(load.get());
    }
    else
    {
        fmt::print("Failed to load gLTF: {} \n", fastgltf::to_underlying(load.error()));
        return {};
    }

    // 2. Load mesh data from binary file
    std::vector<std::shared_ptr<MeshAsset>> meshes;

    // Use the same vectors for all meshes so that the memory doesn't reallocate as often.
    // This is a temporary storage that is used as a parser in the end and not the final storage.
    std::vector<uint32_t> indices {};
    std::vector<Vertex> vertices {};

    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
        MeshAsset newMesh;

        newMesh.name = mesh.name;

        // Clear before each mesh processing to avoid merging mesh data together.
        // It's one at a time in the end.
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives)
        {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t) indices.size();
            newSurface.count = (uint32_t) gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vertex_count = vertices.size();

            // Load indices
            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexAccessor.count);

                // Lambda function for iterating vertex iteration and getting indices
                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor,
                    [&](std::uint32_t idx)
                    {
                        indices.push_back(idx + initial_vertex_count);
                    });
            }

            // Load vertex positions
            {
                fastgltf::Accessor& positionAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + positionAccessor.count);

                // Lambda function for iterating vertex data and getting their info
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, positionAccessor,
                    [&](glm::vec3 v, size_t index)
                    {
                        Vertex newvtx;
                        newvtx.position = v;
                        newvtx.normal = { 1, 0, 0 };
                        newvtx.color = glm::vec4 { 1.f };
                        newvtx.uv_x = 0;
                        newvtx.uv_y = 0;
                        vertices[initial_vertex_count + index] = newvtx;
                    });
            }

            // Load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index)
                    {
                        vertices[initial_vertex_count + index].normal = v;
                    });
            }

            // Load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index)
                    {
                        vertices[initial_vertex_count + index].uv_x = v.x;
                        vertices[initial_vertex_count + index].uv_y = v.y;
                    });
            }

            // Load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index)
                    {
                        vertices[initial_vertex_count + index].color = v;
                    });
            }
            newMesh.surfaces.push_back(newSurface);

            // Display the vertex normals
            // Compile-time flag that to override vertex colors and normals.
            // Used best for debugging.
            constexpr bool OverrideColors = true;
            if (OverrideColors)
            {
                for (Vertex& vtx : vertices)
                {
                    vtx.color = glm::vec4(vtx.normal, 1.f);
                }
            }
            // 3. Upload mesh into a mesh buffere
            newMesh.meshBuffers = engine->upload_mesh(indices, vertices);

            // 4. Store newly read mesh data into temporary storage
            meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
        }
    }

    return meshes;
}
