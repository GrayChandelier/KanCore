#pragma once
#include "../include/Image.hpp"
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
namespace KanCore::Graphics::ImageDetails
{
	int getChannelsCount(PixelFormat format)
	{
		switch (format)
		{
		case  PixelFormat::RGB8:  return 3;
		case  PixelFormat::BGR8:  return 3;
		case  PixelFormat::RGBA8: return 4;
		case  PixelFormat::BGRA8: return 4;
		case  PixelFormat::Mono8: return 1;
		default:
			throw std::invalid_argument("Unknown PixelFormat");
		}
	}

	void fillImage(Image& target, Size2Di size, PixelFormat format, std::span<uint8_t> pixels)
	{
		size_t expected = size.width * size.height * getChannelsCount(format);
		if (pixels.size() != expected)
			throw std::runtime_error("Bad image data size");

		target.size = size;
		target.format = format;

		target.pixels.clear();
		target.pixels.resize(pixels.size());

		std::memcpy(target.pixels.data(), pixels.data(), pixels.size());

	}

}

namespace KanCore::Graphics::ImageLoaders
{
	void loadFromFile(Image& image, const std::string& path, uint8_t desiredChannels)
	{
		if (path.empty())
			throw std::invalid_argument("Failed to load image: empty file path");

		int width, height, channels;
	
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, desiredChannels);

		if (!data)
		{
			const char* reason = stbi_failure_reason();
			throw std::runtime_error(
				"Failed to load image '" + path + "': " +
				(reason ? std::string(reason) : "Unknown error")
			);
		}

		PixelFormat format;
		switch (desiredChannels)
		{
			case 1: format = PixelFormat::Mono8; break;
			case 3: format = PixelFormat::RGB8; break;
			case 4: format = PixelFormat::RGBA8; break;
			default: throw std::runtime_error("Bad image desired channels (choose 1, 3 or 4 for Mono, RGB or RGBA)");
		}

		Graphics::ImageDetails::fillImage(image, 
										  Size2Di{ static_cast<size_t>(width), static_cast<size_t>(height) }, 
										  format, 
			                              { data, static_cast<size_t>(width) * static_cast<size_t>(height) * desiredChannels });

		stbi_image_free(data);


	}
	Image loadFromFile(const std::string& path, uint8_t desiredChannels = 4)
	{
		Image image;
		loadFromFile(image, path, desiredChannels);


		return image;
	}
}
