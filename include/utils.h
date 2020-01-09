#pragma once

#include <vector>

std::vector<float> linear_spacing(float lower, float upper, size_t steps)
{
	std::vector<float> data;
	for (size_t i = 0; i < steps; i++)
	{
		data.push_back(lower + static_cast<float>(i)* (upper - lower) / static_cast<float>(steps - 1));
	}

	return data;
}