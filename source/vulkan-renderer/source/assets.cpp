#include "assets.hpp"

#include <stb_image.h>

#include "engine.hpp"
#include "initializers.hpp"

#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include "helpers.h"

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(Engine* engine, std::string localPath)
{
    std::filesystem::path const assetPath{ DebugUtils::getLoadedDebugUtils().makeAbsolutePath(localPath) };

    Log(fmt::format("Loading glTF: {}", assetPath.string()));

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(assetPath);

    auto constexpr gltfOptions{
        fastgltf::Options::LoadGLBBuffers
        | fastgltf::Options::LoadExternalBuffers
    };

    fastgltf::Parser parser{};

    fastgltf::Expected<fastgltf::Asset> load{ parser.loadGltfBinary(&data, assetPath.parent_path(), gltfOptions) };
    if (!load) {
        Error(fmt::format("Failed to load glTF: {}", fastgltf::to_underlying(load.error())));
        return {};
    }
    fastgltf::Asset const gltf{ std::move(load.get()) };

    std::vector<std::shared_ptr<MeshAsset>> newMeshes{};
    for (fastgltf::Mesh const& mesh : gltf.meshes) {
        std::vector<uint32_t> indices{};
        std::vector<Vertex> vertices{};

        std::vector<GeometrySurface> surfaces{};

        // Proliferate indices and vertices
        for (auto&& primitive : mesh.primitives)
        {
            surfaces.push_back(GeometrySurface{
                .firstIndex{ static_cast<uint32_t>(indices.size()) },
                .indexCount{ static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count) },
            });

            size_t const initialVertexIndex{ vertices.size() };

            { // Indices, not optional
                fastgltf::Accessor const& indexAccessor{ gltf.accessors[primitive.indicesAccessor.value()] };
                indices.reserve(indices.size() + indexAccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor,
                    [&](std::uint32_t index) {
                        indices.push_back(index + initialVertexIndex);
                    }
                );
            }

            { // Positions, not optional
                fastgltf::Accessor const& positionAccessor{ 
                    gltf.accessors[primitive.findAttribute("POSITION")->second] 
                };

                vertices.reserve(vertices.size() + positionAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, positionAccessor,
                    [&](glm::vec3 position, size_t index) {
                        vertices.push_back(Vertex{
                            .position{ position },
                            .uv_x{ 0.0f },
                            .normal{ 1, 0, 0 },
                            .uv_y{ 0.0f },
                            .color{ glm::vec4(1.0f) },
                        });
                    }
                );
            }

            // The rest of these parameters are optional.

            { // Normals
                auto const normals{ primitive.findAttribute("NORMAL") };
                if (normals != primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                        [&](glm::vec3 normal, size_t index) {
                            vertices[initialVertexIndex + index].normal = normal;
                        }
                    );
                }
            }

            { // UVs
                auto const uvs{ primitive.findAttribute("TEXCOORD_0") };
                if (uvs != primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uvs).second],
                        [&](glm::vec2 texcoord, size_t index) {
                            vertices[initialVertexIndex + index].uv_x = texcoord.x;
                            vertices[initialVertexIndex + index].uv_y = texcoord.y;
                        }
                    );
                }
            }

            { // Colors
                auto const colors{ primitive.findAttribute("COLOR_0") };
                if (colors != primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                        [&](glm::vec4 color, size_t index) {
                            vertices[initialVertexIndex + index].color = color;
                        }
                    );
                }
            }
        }

        // Set the colors to the normals for debugging
        bool constexpr overrideColors{ true };
        if (overrideColors) {
            for (Vertex& vertex : vertices) {
                vertex.color = glm::vec4(vertex.normal, 1.0f);
            }
        }

        bool constexpr flipY{ true };
        if (flipY) {
            for (Vertex& vertex : vertices) {
                vertex.normal.y *= -1;
                vertex.position.y *= -1;
            }
        }

        newMeshes.push_back(std::make_shared<MeshAsset>(MeshAsset{
            .name{ std::string{ mesh.name } },
            .surfaces{ surfaces },
            .meshBuffers{ engine->uploadMeshToGPU(indices, vertices) },
        }));
    }

    return newMeshes;
}