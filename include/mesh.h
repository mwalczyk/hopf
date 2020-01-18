#pragma once

#include <vector>

#include "glad/glad.h"
#include "glm.hpp"

#include "vertex.h"

namespace graphics
{

    struct DrawCommand
    {
        uint32_t mode;              // Probably GL_TRIANGLES
        uint32_t count;             // Number of elements to be rendered
        uint32_t type;              // Probably GL_UNSIGNED_BYTE
        uint32_t indices;           // A pointer to the location where the indices are stored
        uint32_t base_vertex;       // A constant that should be added to each element of `indices` when choosing elements from the enabled vertex arrays
        uint32_t base_instance;     // The base instance for use in fetching instanced vertex attributes
    };

    using MeshData = std::pair<std::vector<Vertex>, std::vector<uint32_t>>;

    class Mesh
    {

    public:

        static MeshData from_sphere(float radius, const glm::vec3& center, size_t u_divisions, size_t v_divisions)
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            // Calculate vertex positions
            for (int i = 0; i <= v_divisions; ++i)
            {
                float v = i / static_cast<float>(v_divisions);		// Fraction along the v-axis, 0..1
                float phi = v * glm::pi<float>();					// Vertical angle, 0..pi

                for (int j = 0; j <= u_divisions; ++j)
                {
                    float u = j / static_cast<float>(u_divisions);	// Fraction along the u-axis, 0..1
                    float theta = u * (glm::pi<float>() * 2);		// Rotational angle, 0..2 * pi

                    // Spherical to Cartesian coordinates
                    float x = cosf(theta) * sinf(phi);
                    float y = cosf(phi);
                    float z = sinf(theta) * sinf(phi);
                    auto position = glm::vec3(x, y, z) * radius + center;

                    Vertex vertex;
                    vertex.position = position;
                    vertex.color = glm::vec3{ 1.0f, 1.0f, 1.0f };
                    vertex.texture_coordinate = glm::vec2{ 0.0f, 0.0f }; // TODO
                    vertices.push_back(vertex);

                    // TODO:
                    // Normal would be: `glm::normalize(vertex)`
                }
            }

            // Calculate indices
            for (int i = 0; i < u_divisions * v_divisions + u_divisions; ++i)
            {
                indices.push_back(i);
                indices.push_back(i + u_divisions + 1);
                indices.push_back(i + u_divisions);

                indices.push_back(i + u_divisions + 1);
                indices.push_back(i);
                indices.push_back(i + 1);
            }

            return { vertices, indices };
        }

        static MeshData from_grid(float width, float height, const glm::vec3& center = glm::vec3{ 0.0f }, size_t u_subdivisions = 10, size_t v_subdivisions = 10)
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            auto map = [](float v, float in_min, float in_max, float out_min, float out_max)
            {
                return (v - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
            };

            for (size_t row = 0; row < v_subdivisions; ++row)
            {
                for (size_t col = 0; col < u_subdivisions; ++col)
                {
                    float u = static_cast<float>(col) / (u_subdivisions - 1);
                    float v = static_cast<float>(row) / (v_subdivisions - 1);

                    glm::vec3 position = { map(u, 0.0f, 1.0f, -1.0f, 1.0f),
                                           0.0f,
                                           map(v, 0.0f, 1.0f, -1.0f, 1.0f) };
                    position.x *= width;
                    position.z *= height;
                    position += center;

                    Vertex vertex;

                    vertex.position = position;
                    vertex.color = glm::vec3{ 1.0f };
                    vertex.texture_coordinate = glm::vec2{ u, v };

                    vertices.push_back(vertex);

                    // If `u_divisions` is set to 4, we have:
                    // 
                    // 0 -- 1 -- 2 -- 3
                    // | \  | \  |  \ |
                    // 4 -- 5 -- 6 -- 7
                    // . . . 
                    // .
                    // .
                    // Note: we assume a counter-clockwise winding pattern.

                    // We don't need to form any triangles for the last row
                    size_t cell = col + u_subdivisions * row;
                    if ((row + 1) % v_subdivisions != 0)
                    {
                        // Form the first triangle (i.e. 0 -> 4 -> 5...).
                        if ((col + 1) % u_subdivisions != 0)
                        {
                            indices.push_back(cell);

                            indices.push_back(cell + u_subdivisions);
                            indices.push_back(cell + u_subdivisions + 1);
                        }

                        // Only form this triangle if we aren't on the first (0-th) column.
                        if (col % u_subdivisions != 0)
                        {
                            indices.push_back(cell);
                            indices.push_back(cell - 1);
                            indices.push_back(cell + u_subdivisions);

                        }
                    }
                }
            }

            return { vertices, indices };
        }

        static MeshData from_coordinate_frame(float size, const glm::vec3& center = glm::vec3{ 0.0f })
        {
            std::vector<Vertex> vertices = {
                // X-axis
                Vertex {
                    glm::vec3{0.0f} +center,
                    glm::vec3{1.0f, 0.0f, 0.0f}
                },
                Vertex {
                    glm::vec3{1.0f, 0.0f, 0.0f} *size + center,
                    glm::vec3{1.0f, 0.0f, 0.0f}
                },

                // Y-axis
                Vertex {
                    glm::vec3{0.0f} +center,
                    glm::vec3{0.0f, 1.0f, 0.0f}
                },
                Vertex {
                    glm::vec3{0.0f, 1.0f, 0.0f} *size + center,
                    glm::vec3{0.0f, 1.0f, 0.0f}
                },

                // Z-axis
                Vertex {
                    glm::vec3{0.0f} +center,
                    glm::vec3{0.0f, 0.0f, 1.0f}
                },
                Vertex {
                    glm::vec3{0.0f, 0.0f, 1.0f} *size + center,
                    glm::vec3{0.0f, 0.0f, 1.0f}
                },
            };

            std::vector<uint32_t> indices = {
                0, 1, 2, 3, 4, 5, 6
            };

            return { vertices, indices };
        }

        Mesh() = default;

        Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices) :
            vertices{ vertices },
            indices{ indices }
        {
            setup();
        }

        ~Mesh()
        {
            // RAII: clean-up OpenGL objects
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ibo);
        }

        Mesh& operator=(Mesh&& other) noexcept
        {
            // Grab the other mesh's OpenGL handles
            std::swap(vao, other.vao);
            std::swap(vbo, other.vbo);
            std::swap(ibo, other.ibo);

            vertices = std::move(other.vertices);
            indices = std::move(other.indices);

            return *this;
        }

        Mesh& operator=(const Mesh& other) = delete;

        void draw(uint32_t mode = GL_TRIANGLES) const
        {
            glBindVertexArray(vao);

            if (!indices.empty())
            {
                glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, 0);
            }
            else
            {
                glDrawArrays(mode, 0, vertices.size());
            }

            glBindVertexArray(0);
        }

        void set_vertices(const std::vector<Vertex>& updated_vertices)
        {
            // Re-allocate the buffer if more space is needed: otherwise, we can simply copy in the new data because
            // we already have enough storage
            if (vertices.size() < updated_vertices.size())
            {
                // In DSA, if you need to re-allocate buffer memory, you basically have to reinitialize the 
                // entire buffer, per: https://www.reddit.com/r/opengl/comments/aifvjl/glnamedbufferstorage_vs_glbufferdata/
                vertices = updated_vertices;
                glDeleteVertexArrays(1, &vao);
                glDeleteBuffers(1, &vbo);
                glDeleteBuffers(1, &ibo);
                setup();
            }
            else
            {
                glNamedBufferSubData(vbo, 0, sizeof(Vertex) * updated_vertices.size(), updated_vertices.data());
                vertices = updated_vertices;
            }
        }

        void set_indices(const std::vector<uint32_t>& updated_indices)
        {
            if (indices.size() < updated_indices.size())
            {
                indices = updated_indices;
                glDeleteVertexArrays(1, &vao);
                glDeleteBuffers(1, &vbo);
                glDeleteBuffers(1, &ibo);
                setup();
            }
            else
            {
                glNamedBufferSubData(vbo, 0, sizeof(Vertex) * updated_indices.size(), updated_indices.data());
                indices = updated_indices;
            }
        }

        size_t get_vertex_count() const
        {
            return vertices.size();
        }

        size_t get_index_count() const
        {
            return indices.size();
        }

        const std::vector<Vertex>& get_vertices() const
        {
            return vertices;
        }

        const std::vector<uint32_t>& get_indices() const
        {
            return indices;
        }

    private:

        uint32_t vao;
        uint32_t vbo;
        uint32_t ibo;

        // We shouldn't need to hold onto these CPU-side, but for convenience, we keep them here for now
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        void setup()
        {
            // Load data into the vertex buffer
            if (vertices.empty())
            {
                // This should never happen
                vbo = 0;
            }
            else
            {
                glCreateBuffers(1, &vbo);
                glNamedBufferStorage(vbo, sizeof(Vertex) * vertices.size(), &vertices[0], GL_DYNAMIC_STORAGE_BIT);
            }

            // Load data into the index buffer
            if (indices.empty())
            {
                ibo = 0;
            }
            else
            {
                glCreateBuffers(1, &ibo);
                glNamedBufferStorage(ibo, sizeof(uint32_t) * indices.size(), &indices[0], GL_DYNAMIC_STORAGE_BIT);
            }

            // Set up the VAO and attributes
            glCreateVertexArrays(1, &vao);

            if (vbo)
            {
                // All vertex attributes will be sourced from a single buffer
                glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));

                glEnableVertexArrayAttrib(vao, 0);
                glEnableVertexArrayAttrib(vao, 1);
                glEnableVertexArrayAttrib(vao, 2);

                glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
                glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
                glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texture_coordinate));

                glVertexArrayAttribBinding(vao, 0, 0);
                glVertexArrayAttribBinding(vao, 1, 0);
                glVertexArrayAttribBinding(vao, 2, 0);
            }
            if (ibo)
            {
                glVertexArrayElementBuffer(vao, ibo);
            }

            // TODO: build draw command
            // ...
        }
    };

}