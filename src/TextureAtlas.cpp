#include "../include/TextureAtlas.hpp"
#include <limits>

namespace KanCore::Graphics
{
    namespace //anonymous namespace
    {
        OpenGL::TextureDataFormat getGLPixelFormat(PixelFormat format)
        {
            switch (format)
            {
            case PixelFormat::RGB8: return OpenGL::TextureDataFormat::RGB;
            case PixelFormat::BGR8: return OpenGL::TextureDataFormat::BGR;

            case PixelFormat::RGBA8: return OpenGL::TextureDataFormat::RGBA;
            case PixelFormat::BGRA8: return OpenGL::TextureDataFormat::BGRA;

            case PixelFormat::Mono8: return OpenGL::TextureDataFormat::R;

            default: throw std::runtime_error("Bad image format");
            }
        }
    }



    namespace TextureAtlasDetails
    {
        bool MaxRectsStrategy::contains(const FreeRect& outer, const FreeRect& inner) noexcept
        {
            return inner.x >= outer.x &&
                inner.y >= outer.y &&
                inner.x + inner.width <= outer.x + outer.width &&
                inner.y + inner.height <= outer.y + outer.height;
        }

        void MaxRectsStrategy::prune()
        {
            for (auto outerIt = freeRects.begin(); outerIt != freeRects.end(); ++outerIt)
            {
                auto innerIt = std::next(outerIt);
                while (innerIt != freeRects.end())
                {
                    if (contains(*outerIt, *innerIt))
                        innerIt = freeRects.erase(innerIt);
                    else
                        ++innerIt;
                }
            }
        }

        void MaxRectsStrategy::reinitialize(uint32_t atlasWidth, uint32_t atlasHeight, uint8_t paddingPx)
        {
            padding = paddingPx;
            freeRects.clear();
            freeRects.insert({ 0, 0, atlasWidth, atlasHeight });
        }

        std::optional<Rect2Di> MaxRectsStrategy::tryAllocatePixelRect(const Size2Di& rectSize)
        {
            if (rectSize.width <= 0 || rectSize.height <= 0)
                return std::nullopt;

            uint32_t paddedW = rectSize.width + padding;
            uint32_t paddedH = rectSize.height + padding;

            FreeRect best{};
            bool found = false;
            int bestShort = std::numeric_limits<int>::max();
            int bestLong = std::numeric_limits<int>::max();

            for (const auto& rect : freeRects)
            {
                if (paddedW > rect.width || paddedH > rect.height)
                    continue;

                int dW = rect.width - paddedW;
                int dH = rect.height - paddedH;

                int currentShort = std::min(dW, dH);
                int currentLong = std::max(dW, dH);

                if (currentShort < bestShort ||
                    (currentShort == bestShort && currentLong < bestLong))
                {
                    bestShort = currentShort;
                    bestLong = currentLong;
                    best = rect;
                    found = true;
                }
            }

            if (!found)
                return std::nullopt;

            // Erase old
            freeRects.erase(best);

            // RectSize with padding
            FreeRect placedPadded = { best.x, best.y, paddedW, paddedH };

            // The first variable of splitting
            FreeRect up1{ best.x, best.y + paddedH, best.width, best.height - paddedH };
            FreeRect right1{ best.x + paddedW, best.y, best.width - paddedW, paddedH };

            uint64_t wasted1 = (up1.area() > 0 && right1.area() > 0) ?
                std::min(up1.area(), right1.area()) : 0;

            // The second variable of splitting
            FreeRect up2{ best.x, best.y + paddedH, paddedW, best.height - paddedH };
            FreeRect right2{ best.x + paddedW, best.y, best.width - paddedW, best.height };

            uint64_t wasted2 = (up2.area() > 0 && right2.area() > 0) ?
                std::min(up2.area(), right2.area()) : 0;

            if (wasted1 <= wasted2)
            {
                if (up1.area() > 0) freeRects.insert(up1);
                if (right1.area() > 0) freeRects.insert(right1);
            }
            else
            {
                if (up2.area() > 0) freeRects.insert(up2);
                if (right2.area() > 0) freeRects.insert(right2);
            }

            Rect2Di inserted{
                static_cast<int32_t>(best.x),
                static_cast<int32_t>(best.y),
                rectSize.width,
                rectSize.height
            };

            prune();

            return inserted;
        }

    } // namespace TextureAtlasDetails

    TextureAtlas2D::TextureAtlas2D(Size2Di size, OpenGL::TextureInternalFormat format,
        ImageList&& images,
        std::unique_ptr<PackingStategy> packingStrategy,
        uint8_t paddingPx)
        : atlasSize(size),
        format(format),
        texture(size.width, size.height, format, 0 /*mip levels*/)
    {
        if (size.width <= 0 || size.height <= 0)
            throw std::runtime_error("Bad texture atlas size");

        using DefaultStrategy = TextureAtlasDetails::MaxRectsStrategy;
        if (packingStrategy == nullptr)
            packingStrategy = std::make_unique<DefaultStrategy>();

        packingStrategy->reinitialize(static_cast<uint32_t>(size.width),
            static_cast<uint32_t>(size.height),
            paddingPx);

        auto comparator = [](const std::pair<TextureKey, Image>& A,
            const std::pair<TextureKey, Image>& B) -> bool
            {
                uint64_t areaA = static_cast<uint64_t>(A.second.getSize().width) *
                    A.second.getSize().height;
                uint64_t areaB = static_cast<uint64_t>(B.second.getSize().width) *
                    B.second.getSize().height;
                return areaA > areaB;
            };

        std::sort(images.begin(), images.end(), comparator);

        for (auto& image : images)
        {
            const TextureKey& key = image.first;
            Image& img = image.second;

            std::optional<TextureAtlasDetails::Rect2Di> rectOpt =
                packingStrategy->tryAllocatePixelRect(img.getSize());

            if (!rectOpt.has_value() || UVRects.contains(key))
                throw std::runtime_error("Failed to insert texture in atlas");

            TextureAtlasDetails::Rect2Di pixelRect = rectOpt.value();
            UVRect rect = TextureAtlasDetails::convertToUVRect(pixelRect, atlasSize);
            UVRects.emplace(key, rect);


            
            OpenGL::TextureDataFormat dataFormat = getGLPixelFormat(img.getPixelFormat());
            OpenGL::TextureDataType dataType = OpenGL::TextureDataType::UBYTE;

            texture.updateRegion(0 /*level*/,
                pixelRect.x, pixelRect.y,
                pixelRect.width, pixelRect.height,
                dataFormat, dataType,
                img.getData().data());
        }
    }

    std::optional<UVRect> TextureAtlas2D::getTextureUV(TextureKey textureKey) const noexcept
    {
        auto it = UVRects.find(textureKey);
        if (it == UVRects.end())
            return std::nullopt;
        return std::make_optional(it->second);
    }

    void TextureAtlas2D::bind(uint8_t textureSlot) const noexcept
    {
        texture.bind(textureSlot);
    }

    DynamicTextureAtlas2D::DynamicTextureAtlas2D(Size2Di size,
        OpenGL::TextureInternalFormat format,
        std::unique_ptr<PackingStategy> packingStrategy,
        uint8_t paddingPx)
        : atlasSize(size),
        format(format),
        strategy(std::move(packingStrategy)),
        texture(size.width, size.height, format, 0 /*mip levels*/)
    {
        if (size.width <= 0 || size.height <= 0)
            throw std::runtime_error("Bad texture atlas size");

        if (!strategy)
            strategy = std::make_unique<TextureAtlasDetails::MaxRectsStrategy>();

        strategy->reinitialize(static_cast<uint32_t>(size.width),
            static_cast<uint32_t>(size.height),
            paddingPx);
    }

    std::optional<UVRect> DynamicTextureAtlas2D::getTextureUV(TextureKey textureKey) const noexcept
    {
        auto it = UVRects.find(textureKey);
        if (it == UVRects.end())
            return std::nullopt;
        return std::make_optional(it->second);
    }

    void DynamicTextureAtlas2D::bind(uint8_t textureSlot) const noexcept
    {
        texture.bind(textureSlot);
    }

    std::optional<UVRect> DynamicTextureAtlas2D::insert(TextureKey textureKey, const Image& image)
    {
        if (UVRects.contains(textureKey))
            return std::nullopt;

        OpenGL::TextureDataFormat dataFormat = getGLPixelFormat(image.getPixelFormat());;
        OpenGL::TextureDataType dataType = OpenGL::TextureDataType::UBYTE;
        const Size2Di imageSize = image.getSize();
        std::optional<TextureAtlasDetails::Rect2Di> rectOpt = strategy->tryAllocatePixelRect(imageSize);

        if (!rectOpt.has_value())
            return std::nullopt;

        TextureAtlasDetails::Rect2Di pixelRect = rectOpt.value();
        UVRect rect = TextureAtlasDetails::convertToUVRect(pixelRect, atlasSize);

        texture.updateRegion(0 /*level*/,
            pixelRect.x, pixelRect.y,
            pixelRect.width, pixelRect.height,
            dataFormat, dataType,
            image.getData().data());

        UVRects.emplace(textureKey, rect);
        return std::make_optional(rect);
    }

} 