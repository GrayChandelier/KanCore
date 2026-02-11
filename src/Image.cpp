#pragma once
#include "../include/Image.hpp"
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
namespace KanCore::Graphics::ImageDetails
{
	void fillImage(Image& target, Size2Di size, uint8_t channels, std::span<uint8_t> pixels)
	{
		target.size = size;
		target.channels = channels;

		target.pixels.reserve(pixels.size());
		target.pixels.insert(target.pixels.end(), pixels.begin(), pixels.end());
	}

}

namespace KanCore::Graphics::ImageLoaders
{
	void loadFromFile(Image& image, const std::string& path, uint8_t desiredChannels)
	{
		if (path.empty())
			throw std::invalid_argument("Failed to load image: empty file path");

		int width, height, channels;

		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, desiredChannels);

		if (!data)
		{
			const char* reason = stbi_failure_reason();
			throw std::runtime_error(
				"Failed to load image '" + path + "': " +
				(reason ? std::string(reason) : "Unknown error")
			);
		}

		Graphics::ImageDetails::fillImage(image, Size2Di{ static_cast<size_t>(width), 
			static_cast<size_t>(height) }, 
			desiredChannels, 
			{ data, static_cast<size_t>(width) * static_cast<size_t>(height) * desiredChannels }
		);
		stbi_image_free(data);


		std::cout << width << "x" << height << "\n\n";
	}
	Image loadFromFile(const std::string& path, uint8_t desiredChannels = 4)
	{
		Image image;
		loadFromFile(image, path, desiredChannels);


		return image;
	}
}
