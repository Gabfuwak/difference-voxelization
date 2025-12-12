#pragma once

#include <vector>
#include <webgpu/webgpu_cpp.h>
#include "tiny_obj_loader.h"
#include <ranges>

namespace scene {
struct Vertex {
    float position[3];
    float color[3];
    float uv[2];
};

class Mesh {
public:
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    uint32_t indexCount = 0;

    void upload(wgpu::Device device, wgpu::Queue queue,
                const std::vector<Vertex>& vertices,
                const std::vector<uint16_t>& indices) {

        indexCount = static_cast<uint32_t>(indices.size());

        // Create vertex buffer
        wgpu::BufferDescriptor vbDesc{
            .label = "Vertex buffer",
            .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
            .size = vertices.size() * sizeof(Vertex),
        };
        vertexBuffer = device.CreateBuffer(&vbDesc);
        queue.WriteBuffer(vertexBuffer, 0, vertices.data(), vertices.size() * sizeof(Vertex));

        // Create index buffer
        wgpu::BufferDescriptor ibDesc{
            .label = "Index buffer",
            .usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst,
            .size = indices.size() * sizeof(uint16_t),
        };
        indexBuffer = device.CreateBuffer(&ibDesc);
        queue.WriteBuffer(indexBuffer, 0, indices.data(), indices.size() * sizeof(uint16_t));
    }

    static Mesh createMesh(std::string path, wgpu::Device device, wgpu::Queue queue){
        tinyobj::ObjReaderConfig reader_config;
        tinyobj::ObjReader reader;

        reader.ParseFromFile(path, reader_config);

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        std::vector<float> vertices_raw = attrib.vertices;
        std::vector<float> colors_raw = attrib.colors;
        std::vector<float> texcoords_raw = attrib.texcoords;  // NEW: UV coordinates from OBJ

        std::vector<Vertex> vertices;
        vertices.resize(vertices_raw.size()/3);

        for(int i = 0; i < vertices_raw.size(); i+=3){

            float r = 0.5f, g = 0.5f, b = 0.5f;  // default gray
            if (!colors_raw.empty()) {
                r = colors_raw[i];
                g = colors_raw[i + 1];
                b = colors_raw[i + 2];
            }

            // Default UVs if not present in file
            float u = 0.0f, v = 0.0f;

            vertices[i / 3] = {
                {vertices_raw[i], vertices_raw[i + 1], vertices_raw[i + 2]},
                {r, g, b},
                {u, v}  // Will be overwritten below if texcoords exist
            };
        }

        // Extract indices and UVs
        std::vector<uint16_t> indices;
        for (const auto& idx : shapes[0].mesh.indices) {
            uint16_t vertex_idx = static_cast<uint16_t>(idx.vertex_index);
            indices.push_back(vertex_idx);

            // If texture coordinates exist, update the vertex
            if (idx.texcoord_index >= 0 && !texcoords_raw.empty()) {
                int uv_idx = idx.texcoord_index * 2;
                vertices[vertex_idx].uv[0] = texcoords_raw[uv_idx];
                vertices[vertex_idx].uv[1] = texcoords_raw[uv_idx + 1];
            }
        }

        Mesh mesh;
        mesh.upload(device, queue, vertices, indices);
        return mesh;
    }

    // Factory method for a colored cube
    static Mesh createCube(wgpu::Device device, wgpu::Queue queue) {
        // Each face has its own vertices for distinct face colors
        std::vector<Vertex> vertices = {
            // Front face (red) - looking at +Z
            // Position                  Color                UV
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.3f, 0.3f}, {0.0f, 0.0f}}, // bottom-left
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.3f, 0.3f}, {1.0f, 0.0f}}, // bottom-right
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.3f, 0.3f}, {1.0f, 1.0f}}, // top-right
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.3f, 0.3f}, {0.0f, 1.0f}}, // top-left

            // Back face (cyan) - looking at -Z
            {{ 0.5f, -0.5f, -0.5f}, {0.3f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-left (from back view)
            {{-0.5f, -0.5f, -0.5f}, {0.3f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right
            {{-0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-right
            {{ 0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // top-left

            // Top face (green) - looking down at +Y
            {{-0.5f,  0.5f,  0.5f}, {0.3f, 1.0f, 0.3f}, {0.0f, 0.0f}}, // bottom-left (front-left)
            {{ 0.5f,  0.5f,  0.5f}, {0.3f, 1.0f, 0.3f}, {1.0f, 0.0f}}, // bottom-right (front-right)
            {{ 0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 0.3f}, {1.0f, 1.0f}}, // top-right (back-right)
            {{-0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 0.3f}, {0.0f, 1.0f}}, // top-left (back-left)

            // Bottom face (magenta) - looking up at -Y
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.3f, 1.0f}, {0.0f, 0.0f}}, // bottom-left (back-left)
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.3f, 1.0f}, {1.0f, 0.0f}}, // bottom-right (back-right)
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.3f, 1.0f}, {1.0f, 1.0f}}, // top-right (front-right)
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.3f, 1.0f}, {0.0f, 1.0f}}, // top-left (front-left)

            // Right face (yellow) - looking at +X
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.3f}, {0.0f, 0.0f}}, // bottom-left (front)
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.3f}, {1.0f, 0.0f}}, // bottom-right (back)
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.3f}, {1.0f, 1.0f}}, // top-right (back)
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.3f}, {0.0f, 1.0f}}, // top-left (front)

            // Left face (blue) - looking at -X
            {{-0.5f, -0.5f, -0.5f}, {0.3f, 0.3f, 1.0f}, {0.0f, 0.0f}}, // bottom-left (back)
            {{-0.5f, -0.5f,  0.5f}, {0.3f, 0.3f, 1.0f}, {1.0f, 0.0f}}, // bottom-right (front)
            {{-0.5f,  0.5f,  0.5f}, {0.3f, 0.3f, 1.0f}, {1.0f, 1.0f}}, // top-right (front)
            {{-0.5f,  0.5f, -0.5f}, {0.3f, 0.3f, 1.0f}, {0.0f, 1.0f}}, // top-left (back)
        };

        std::vector<uint16_t> indices = {
            // Front
            0, 1, 2,  0, 2, 3,
            // Back
            4, 5, 6,  4, 6, 7,
            // Top
            8, 9, 10,  8, 10, 11,
            // Bottom
            12, 13, 14,  12, 14, 15,
            // Right
            16, 17, 18,  16, 18, 19,
            // Left
            20, 21, 22,  20, 22, 23,
        };

        Mesh mesh;
        mesh.upload(device, queue, vertices, indices);
        return mesh;
    }

    // Factory method for a grid plane
    static Mesh createGridPlane(wgpu::Device device, wgpu::Queue queue,
                               float size = 10.0f, int divisions = 10) {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;

        // Calculate step size between grid lines
        float step = size / divisions;
        float halfSize = size / 2.0f;

        // Generate vertices
        for (int z = 0; z <= divisions; ++z) {
            for (int x = 0; x <= divisions; ++x) {
                float xPos = -halfSize + x * step;
                float zPos = -halfSize + z * step;

                // UV coordinates: 0→1 across entire plane
                float u = (float)x / divisions;  // 0.0 at left edge, 1.0 at right edge
                float v = (float)z / divisions;  // 0.0 at front edge, 1.0 at back edge

                // Create a checkerboard color pattern (for debugging without texture)
                float colorIntensity = ((x + z) % 2 == 0) ? 0.8f : 0.6f;

                vertices.push_back({
                    {xPos, 0.0f, zPos},                          // position
                    {colorIntensity, colorIntensity, colorIntensity},  // color
                    {u, v}                                       // UV
                });
            }
        }

        // Generate indices for triangles
        for (int z = 0; z < divisions; ++z) {
            for (int x = 0; x < divisions; ++x) {
                // Calculate vertex indices for this grid cell
                uint16_t topLeft = z * (divisions + 1) + x;
                uint16_t topRight = topLeft + 1;
                uint16_t bottomLeft = (z + 1) * (divisions + 1) + x;
                uint16_t bottomRight = bottomLeft + 1;

                // First triangle (top-left, bottom-left, top-right)
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // Second triangle (top-right, bottom-left, bottom-right)
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        Mesh mesh;
        mesh.upload(device, queue, vertices, indices);
        return mesh;
    }
};

} // namespace scene
