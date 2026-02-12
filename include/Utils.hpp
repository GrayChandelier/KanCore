#pragma once
#include <vector>
#include <span>
#include <stdexcept>
namespace KanCore::Utils
{
	inline std::vector<uint8_t> BGRA8_to_RGBA8(std::span<uint8_t>& source)
	{
		if (source.size() % 4 != 0)
			throw std::invalid_argument("Bad container size");

		std::vector<uint8_t> output(source.size());

		for (size_t it = 0; it < source.size(); it += 4)
		{
			output[it] = source[it + 2];
			output[it + 1] = source[it + 1];
			output[it + 2] = source[it + 0]; 
			output[it + 3] = source[it + 3]; 
		}

		return output;
	}

	inline std::vector<uint8_t> Mono8_to_RGBA8(std::span<uint8_t>& source)
	{
		std::vector<uint8_t> output(source.size() * 4);

		for (size_t i = 0; i < source.size(); ++i)
		{
			output[4 * i + 0] = source[i];
			output[4 * i + 1] = source[i];
			output[4 * i + 2] = source[i];
			output[4 * i + 3] = 255;
		}

		return output;
	}

	inline std::vector<uint8_t> RGBA8_to_Mono8(std::span<uint8_t>& source)
	{
		if (source.size() % 4 != 0)
			throw std::invalid_argument("Bad container size");

		std::vector<uint8_t> output(source.size() / 4);

		for (size_t it = 0; it < output.size(); it += 1)
		{
			float r = source[4 * it + 0];
			float g = source[4 * it + 1];
			float b = source[4 * it + 2];

			output[it] = static_cast<uint8_t>(0.2126f * r + 0.7152f * g + 0.0722f * b);
		}
		return output;
	}

}
