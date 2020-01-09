#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm.hpp"
#include "gtx/hash.hpp"

/**
 * A struct representing a vertex with 3 attributes: position (vec3), color (vec3),
 * and UV coordinates (vec2).
 */
struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texture_coordinate;

    bool operator==(const Vertex& other) const
    {
        return position == other.position && color == other.color && texture_coordinate == other.texture_coordinate;
    }
};

namespace std
{
    template<>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texture_coordinate) << 1);
        }
    };
}
