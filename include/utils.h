#pragma once

#include <vector>

namespace utils
{

	std::vector<float> linear_spacing(float lower, float upper, size_t steps)
	{
		std::vector<float> data;
		for (size_t i = 0; i < steps; ++i)
		{
			data.push_back(lower + static_cast<float>(i)* (upper - lower) / static_cast<float>(steps - 1));
		}

		return data;
	}

	void save_polyline_obj(const graphics::Mesh& mesh, std::string filename = "model.obj")
	{
		// See: http://paulbourke.net/dataformats/obj/

		// If the user didn't add the file extension, add it here
		if (filename.substr(filename.find_last_of(".") + 1) != "obj")
		{
			filename += ".obj";
		}

		// Open the newly created file
		std::ofstream file;
		file.open(filename);

		// Write vertices
		for (size_t i = 0; i < mesh.get_vertices().size(); ++i)
		{
			const auto vertex = mesh.get_vertices()[i];
			file << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
		}

		// Write indices
		bool start = true;
		for (size_t i = 0; i < mesh.get_indices().size(); ++i)
		{
			if (start)
			{
				file << "l ";
				start = false;
			}

			// Primitive restart (i.e. the start of a new polyline)
			if (mesh.get_indices()[i] == std::numeric_limits<uint32_t>::max())
			{
				file << "\n";
				start = true;
				continue;
			}

			// .obj files use 1-based indexing
			file << mesh.get_indices()[i] + 1 << " ";
		}
		file.close();
	}

}