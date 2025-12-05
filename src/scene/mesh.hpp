#pragma once
#define TINYOBJLOADER_IMPLEMENTATION

#include <vector>
#include <webgpu/webgpu_cpp.h>
#include "tiny_obj_loader.h"
#include <ranges>


namespace scene {

struct Vertex {
    float position[3];
    float color[3];
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

        std::vector<Vertex> vertices;
        vertices.resize(vertices_raw.size()/3);

        for(int i = 0; i < vertices_raw.size(); i+=3){
            
            vertices[i/3] = {{vertices_raw[i], vertices_raw[i+1], vertices_raw[i+2]},
                           {colors_raw[i]  , colors_raw[i+1]  , colors_raw[i+2]}};
        }

        std::vector<uint16_t> indices;
        for (const auto& idx : shapes[0].mesh.indices) {
            indices.push_back(static_cast<uint16_t>(idx.vertex_index));
        }



        Mesh mesh;
        mesh.upload(device, queue, vertices, indices);
        return mesh;
    }

    // Factory method for a colored cube
    static Mesh createCube(wgpu::Device device, wgpu::Queue queue) {
        // Each face has its own vertices for distinct face colors
        std::vector<Vertex> vertices = {
            // Front face (red) - z = 0.5
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.3f, 0.3f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.3f, 0.3f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.3f, 0.3f}},
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.3f, 0.3f}},

            // Back face (cyan) - z = -0.5
            {{ 0.5f, -0.5f, -0.5f}, {0.3f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.3f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 1.0f}},

            // Top face (green) - y = 0.5
            {{-0.5f,  0.5f,  0.5f}, {0.3f, 1.0f, 0.3f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.3f, 1.0f, 0.3f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 0.3f}},
            {{-0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 0.3f}},

            // Bottom face (magenta) - y = -0.5
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.3f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.3f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.3f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.3f, 1.0f}},

            // Right face (yellow) - x = 0.5
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.3f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.3f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.3f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.3f}},

            // Left face (blue) - x = -0.5
            {{-0.5f, -0.5f, -0.5f}, {0.3f, 0.3f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.3f, 0.3f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.3f, 0.3f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.3f, 0.3f, 1.0f}},
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
};

} // namespace scene
