#pragma once

#include <algorithm>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm.hpp"
#include "gtc/constants.hpp"
#include "gtx/norm.hpp"
#include "gtx/string_cast.hpp"

#include "mesh.h"
#include "utils.h"

std::vector<glm::vec3> build_tube(const std::vector<glm::vec3>& path)
{
	std::vector<glm::vec3> tube_vertices;
	const size_t number_of_segments = 20;
	const float radius = 0.2f;

	auto v_prev = glm::vec3{ 0.0f, 0.0f, 0.0f };

	// Loop over all of the indices plus the last one to form a closed loop
	for (size_t i = 0; i < path.size(); i++)
	{	
		// Wrap neighbor indices around
		size_t center_index = i;
		size_t neighbor_l_index = i == 0 ? path.size() - 1 : center_index - 1;
		size_t neighbor_r_index = i == path.size() - 1 ? 0 : center_index + 1;

		// Grab the current vertex plus its two neighbors
		auto center = path[center_index];
		auto neighbor_l = path[neighbor_l_index];
		auto neighbor_r = path[neighbor_r_index];

		auto towards_l = glm::normalize(neighbor_l - center); // Vector that points towards the left neighbor
		auto towards_r = glm::normalize(neighbor_r - center); // Vector that points towards the right neighbor

		glm::vec3 t;

		if (glm::length2(towards_r - towards_l) > 0.0f)
		{
			t = glm::normalize(towards_r - towards_l);
		} 
		else
		{
			t = -towards_l;
		}
		
		// Calculate the next `u` basis vector
		glm::vec3 u;

		if (i == 0)
		{
			// Find an arbitrary vector perpendicular to the first tangent vector
			const auto z_axis = glm::vec3{ 0.0f, 0.0f, 1.0f };
			u = glm::normalize(glm::cross(z_axis, t));
		}
		else
		{
			u = glm::normalize(glm::cross(t, v_prev));
		}

		// Calculate the next `v` basis vector
		auto v = glm::normalize(glm::cross(u, t));
		
		//std::cout << "u: " << glm::to_string(u) << "\n";
		//std::cout << "v: " << glm::to_string(v) << "\n";
		std::cout << glm::to_string(center + t) << "\n";// << glm::to_string(t) << "\n";

		for (size_t j = 0; j < number_of_segments; ++j)
		{
			float theta = glm::two_pi<float>() * (j / number_of_segments);
			float x = radius * cosf(theta);
			float y = radius * sinf(theta);
			tube_vertices.push_back(u * x + v * y + center);
		}

		// Set the previous `v` vector to the current `v` vector (parallel transport)
		v_prev = v;
	}


	// Generate the final array of vertices, which are the triangles that enclose the
	// tube extrusion: for now, we don't use indexed rendering
	std::vector<glm::vec3> triangles;

	// The number of "rings" (i.e. circular cross-sections) that form the "skeleton" of the tube
	size_t number_of_rings = tube_vertices.size() / number_of_segments;

	for (size_t ring_index = 0; ring_index < (number_of_rings - 1); ring_index++)
	{
		auto next_ring_index = (ring_index + 1) % number_of_rings;

		for (size_t local_index = 0; local_index < number_of_segments; local_index++)
		{
			// Vertices are laid out in "rings" of `number_of_segments` vertices like
			// so (for `number_of_segments = 6`):
			//
			// 6  7  8  9  ...
			//
			// 0  1  2  3  4  5
			auto next_local_index = (local_index + 1) % number_of_segments;

			// First triangle: 0 -> 6 -> 7
			triangles.push_back(tube_vertices[ring_index * number_of_segments + local_index]);
			triangles.push_back(tube_vertices[next_ring_index * number_of_segments + local_index]); // The next ring
			triangles.push_back(tube_vertices[next_ring_index * number_of_segments + next_local_index]); // The next ring

			// Second triangle: 0 -> 7 -> 1
			triangles.push_back(tube_vertices[ring_index * number_of_segments + local_index]);
			triangles.push_back(tube_vertices[next_ring_index * number_of_segments + next_local_index]); // The next ring
			triangles.push_back(tube_vertices[ring_index * number_of_segments + next_local_index]);
		}

	}

	return tube_vertices;
}

class Hopf
{
public:
	
	Hopf(const std::vector<Vertex>& base_points) :
		base_points{ base_points }
	{
		generate_fibration();
	}

	void generate_fibration(size_t iterations_per_fiber = 130)
	{
		auto phis = linear_spacing(0.0f, glm::two_pi<float>(), iterations_per_fiber);
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		
		for (size_t i = 0; i < base_points.size(); ++i)
		{
			const auto point = base_points[i];
			float a = point.position.x;
			float b = point.position.y;
			float c = point.position.z;
			float coeff = 1.0f / sqrtf(2.0f * (1.0f + c));

			// Every `steps` points (in 4-space) form a single fiber of the Hopf fibration
			for (size_t j = 0; j < iterations_per_fiber; j++)
			{
				float phi = phis[j];

				// Points in 4-space: a rotation by the quaternion <x, y, z, w> would send the
				// point <0, 0, 1> on S2 to the point <a, b, c> - thus, each base point sweeps
				// out a great circle ("fiber") on S2
				const float theta = atan2f(-a, b) - phi;
				const float alpha = sqrtf((1.0f + c) / 2.0f);
				const float beta = sqrtf((1.0f - c) / 2.0f);

				const float	w = alpha * cosf(theta);
				const float	x = alpha * sinf(theta);
				const float	y = beta * cosf(phi);
				const float	z = beta * sinf(phi);

				// Modified stereographic projection onto the unit ball in 3-space from:
				// https://nilesjohnson.net/hopf-production.html
				const float r = acosf(w) / glm::pi<float>();
				const float projection = r / sqrtf(1.0f - w * w);

				Vertex vertex;
				vertex.position = glm::vec3{
					projection * x,
					projection * y,
					projection * z
				};
				vertex.color = glm::vec3{
					a * 0.5f + 0.5f,
					b * 0.5f + 0.5f,
					c * 0.5f + 0.5f
				};
				vertex.texture_coordinate = glm::vec2{
					0.0f, // Unused, at the moment
					0.0f 
				};

				vertices.push_back(vertex);
				indices.push_back(j + iterations_per_fiber * i);
			}

			// Primitive restart
			indices.push_back(65535);
		}

		mesh = Mesh{ vertices, indices };
	}

	void draw() const
	{
		mesh.draw(GL_LINE_LOOP);
	}

	const Mesh& get_mesh() const
	{
		return mesh;
	}

	size_t get_base_point_count() const
	{
		return base_points.size();
	}

private:

	Mesh mesh;
	
	uint32_t vao;
	uint32_t vbo;

	std::vector<Vertex> base_points;
};
