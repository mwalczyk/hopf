#pragma once

#include <vector>

#include "glad/glad.h"
#include "glm.hpp"

#include "vertex.h"


class Mesh 
{

public:

    static Mesh from_sphere(float radius, const glm::vec3& center, size_t u_divisions, size_t v_divisions)
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
    
        return Mesh{ vertices, indices };
    }

    static Mesh from_grid(float width, float height, const glm::vec3& center = glm::vec3{ 0.0f }, size_t u_subdivisions = 10, size_t v_subdivisions = 10)
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

        return Mesh{ vertices, indices };
    }

    Mesh()
    {
        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        glCreateBuffers(1, &ibo);
    }

    Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices) :
        vertices{ vertices },
        indices{ indices },
        has_indices{ indices.size() > 0 }
    {
        setup();
    }

    ~Mesh() 
    {
        // Clean-up OpenGL object

        // TODO: for some reason, this causes objects not to be drawn?
        //glDeleteVertexArrays(1, &vao);
        //glDeleteBuffers(1, &vbo);
        //glDeleteBuffers(1, &ibo);
    }

    void draw(uint32_t mode = GL_TRIANGLES) const
    {
        glBindVertexArray(vao);

        if (has_indices)
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

    size_t get_vertex_count() const
    {
        return vertices.size();
    }

    size_t get_index_count() const
    {
        return indices.size();
    }

private:

    bool has_indices = false;

    uint32_t vao;
    uint32_t vbo;
    uint32_t ibo;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void setup()
    {
        // Create OpenGL objects
        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        glCreateBuffers(1, &ibo);

        {
            // Load data into the vertex buffer
            glNamedBufferStorage(vbo, sizeof(Vertex) * vertices.size(), &vertices[0], GL_DYNAMIC_STORAGE_BIT);

            if (has_indices)
            {
                // Load data into the index buffer
                glNamedBufferStorage(ibo, sizeof(uint32_t) * indices.size(), &indices[0], GL_DYNAMIC_STORAGE_BIT);
            }
        }

        {
            // Associate VBO and IBO with the mesh's VAO
            glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));

            if (has_indices)
            {
                glVertexArrayElementBuffer(vao, ibo);
            }
        }

        {
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
    }
};