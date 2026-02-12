#pragma once

#include "OpenGL/GLTexture.hpp"
#include "Details.hpp"
#include "Image.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <set>
#include <memory>
#include <random>
#include <array>

namespace KanCore::Graphics
{
    using TextureKey = uint32_t;

    struct UVRect
    {
        Vec2f pos;
        Vec2f size;
    };

    namespace TextureAtlasDetails
    {
        struct Rect2Di
        {
            uint32_t x, y, width, height;
        };

        inline UVRect convertToUVRect(Rect2Di rect, Size2Di atlasSize)
        {
            float x = static_cast<float>(rect.x) / static_cast<float>(atlasSize.width);
            float y = static_cast<float>(rect.y) / static_cast<float>(atlasSize.height);
            float width = static_cast<float>(rect.width) / static_cast<float>(atlasSize.width);
            float height = static_cast<float>(rect.height) / static_cast<float>(atlasSize.height);
            return UVRect{ {x,y}, {width, height} };
        }

        struct Section
        {
            uint32_t x, y, width, height;
        };

        struct SectionComparator
        {
            bool operator()(const Section& a, const Section& b) const
            {
                uint64_t areaA = uint64_t(a.width) * a.height;
                uint64_t areaB = uint64_t(b.width) * b.height;
                if (areaA != areaB)
                    return areaA > areaB;
                if (a.height != b.height)
                    return a.height > b.height;
                if (a.width != b.width)
                    return a.width > b.width;
                if (a.x != b.x)
                    return a.x < b.x;
                return a.y < b.y;
            }
        };

        class IPackingStrategy
        {
        public:
            virtual void reinitialize(uint32_t atlasWidth, uint32_t atlasHeight, uint8_t paddingPx = 0) = 0;
            virtual std::optional<Rect2Di> tryAllocatePixelRect(const Size2Di& imageSize) = 0;
            virtual ~IPackingStrategy() = default;
        };

        class MaxRectsStrategy : public IPackingStrategy
        {
        private:
            struct FreeRect
            {
                uint32_t x = 0, y = 0, width = 0, height = 0;
                uint64_t area() const noexcept { return uint64_t(width) * height; }
            };

            struct FreeRectComparator
            {
                bool operator()(const FreeRect& a, const FreeRect& b) const
                {
                    uint64_t aa = a.area();
                    uint64_t ab = b.area();
                    if (aa != ab) return aa > ab;
                    if (a.height != b.height) return a.height > b.height;
                    if (a.width != b.width) return a.width > b.width;
                    if (a.x != b.x) return a.x < b.x;
                    return a.y < b.y;
                }
            };

            std::set<FreeRect, FreeRectComparator> freeRects;
            uint8_t padding = 0;

            static bool contains(const FreeRect& outer, const FreeRect& inner) noexcept;

            void prune();

        public:
            void reinitialize(uint32_t atlasWidth, uint32_t atlasHeight, uint8_t paddingPx = 0) override;

            std::optional<Rect2Di> tryAllocatePixelRect(const Size2Di& rectSize) override;
        };

    } 

    class ITextureAtlas
    {
    public:
        virtual std::optional<UVRect> getTextureUV(TextureKey textureKey) const noexcept = 0;
        virtual void bind(uint8_t textureSlot = 0) const noexcept = 0;
        virtual ~ITextureAtlas() noexcept = default;
    };

    using PackingStategy = TextureAtlasDetails::IPackingStrategy;
    using ImageList = std::vector<std::pair<TextureKey, Image>>;


    class TextureAtlas2D : public ITextureAtlas
    {
    private:
        OpenGL::GLTexture2D texture;
        const Size2Di atlasSize;
        const OpenGL::TextureInternalFormat format;

        std::unordered_map<TextureKey, UVRect> UVRects;

    public:
        TextureAtlas2D(Size2Di size, OpenGL::TextureInternalFormat format,
            ImageList&& images,
            std::unique_ptr<PackingStategy> packingStrategy = nullptr,
            uint8_t paddingPx = 0);

        std::optional<UVRect> getTextureUV(TextureKey textureKey) const noexcept override;
        void bind(uint8_t textureSlot = 0) const noexcept override;
    };

    class DynamicTextureAtlas2D : public ITextureAtlas
    {
    private:
        OpenGL::GLTexture2D texture;
        const Size2Di atlasSize;
        const OpenGL::TextureInternalFormat format;
    
        std::unordered_map<TextureKey, UVRect> UVRects;
        std::unique_ptr<PackingStategy> strategy;

    public:
        DynamicTextureAtlas2D(Size2Di size, OpenGL::TextureInternalFormat format,
            std::unique_ptr<PackingStategy> packingStrategy = nullptr,
            uint8_t paddingPx = 0);

        std::optional<UVRect> getTextureUV(TextureKey textureKey) const noexcept override;
        void bind(uint8_t textureSlot = 0) const noexcept override;

        std::optional<UVRect> insert(TextureKey textureKey, const Image& image);
    };

} 