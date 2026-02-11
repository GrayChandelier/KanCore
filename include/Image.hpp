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
		void fillImage(Image& target, Size2Di size, uint8_t channels, std::span<uint8_t> pixels);
	}


	namespace ImageLoaders
	{
		void loadFromFile(Image& image, const std::string& path, uint8_t desiredChannels = 4);
	}

	class Image
	{
		friend void ImageDetails::fillImage(Image& target, Size2Di size, uint8_t channels, std::span<uint8_t> pixels);
	private:
		Size2Di size{0,0};
		uint8_t channels{ 0 };
		std::vector<uint8_t> pixels;
	public:
		Image(){}
		Image(Image&) = delete;
		Image(Image&& other) noexcept
			: pixels(std::move(other.pixels)),
			  channels(other.channels),
			  size(other.size)
		{}

		Image& operator=(Image&& other) noexcept
		{
			this->size = other.size;
			this->channels = other.channels;
			this->pixels = std::move(other.pixels);

			return *this;
		}

		Size2Di getSize() const noexcept
		{
			return size;
		}
		uint8_t getChannels() const noexcept
		{
			return channels;
		}
		std::span<const uint8_t> getData() const noexcept
		{
			return pixels;
		}
		inline void loadFromFile(const std::string& path, uint8_t desiredChannels = 4)
		{
			ImageLoaders::loadFromFile(*this, path);
		}
	};

	
}