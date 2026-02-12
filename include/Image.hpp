#pragma once
#include <utility>
#include <vector>
#include <span>
#include <string>

#include "Details.hpp"


namespace KanCore::Graphics
{
	class Image;

	namespace ImageDetails
	{
		int getChannelsCount(PixelFormat format);
		void fillImage(Image& target, Size2Di size, PixelFormat format, std::span<uint8_t> pixels);
	}


	namespace ImageLoaders
	{
		void loadFromFile(Image& image, const std::string& path, uint8_t desiredChannels = 4);
	}

	
	class Image
	{
		friend void ImageDetails::fillImage(Image& target, Size2Di size, PixelFormat format, std::span<uint8_t> pixels);
	private:
		Size2Di size{0,0};
		PixelFormat format;
		std::vector<uint8_t> pixels;
	public:
		Image(){}
		Image(Image&) = delete;
		Image(Image&& other) noexcept
			: pixels(std::move(other.pixels)),
			  format(other.format),
			  size(other.size)
		{}

		Image& operator=(Image&& other) noexcept
		{
			this->size = other.size;
			this->format = other.format;
			this->pixels = std::move(other.pixels);
			other.size = { 0,0 };
			return *this;
		}

		Size2Di getSize() const noexcept
		{
			return size;
		}
		PixelFormat getPixelFormat() const noexcept
		{
			return format;
		}
		

		std::span<const uint8_t> getData() const noexcept
		{
			return pixels;
		}
		inline void loadFromFile(const std::string& path, uint8_t desiredChannels = 4)
		{
			ImageLoaders::loadFromFile(*this, path, desiredChannels);
		}
	};

	
}