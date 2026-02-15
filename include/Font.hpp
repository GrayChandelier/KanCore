#pragma once

#include "Details.hpp"
#include "Utils.hpp"
#include "../include/TextureAtlas.hpp"


#include <unordered_map>
#include <optional>
#include <span>
#include <memory>
#include <string>
#include <functional>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace KanCore::Graphics
{

    using unicode_char = uint32_t;

    class Font;

    namespace FontDetails
    {
        struct Glyph
        {
            Vec2f size;
            Vec2f bearing;
            float advance;
            UVRect uv;
            bool colorful = false;
        };

        struct GlyphData
        {
            PixelFormat pixelFormat;
            OpenGL::TextureDataType dataType;
            std::span<uint8_t> data;
        };

        struct FontUnit
        {
            unicode_char unicode;
            Glyph glyphDesc;
            GlyphData glyphData;
        };

        class IFontLoadable
        {
        public:
            virtual bool tryAddGlyph(const FontUnit& unit) = 0;
            virtual ~IFontLoadable() = default;
        };


        class IFont
        {
        public:
            virtual std::optional<Glyph> getGlyph(unicode_char character) const noexcept = 0;
            virtual void use(uint8_t fontSlot) const noexcept = 0;
            virtual ~IFont() = default;
        };
    }

    class Font final : public FontDetails::IFont, public FontDetails::IFontLoadable
    {
    private:
        
        Graphics::DynamicTextureAtlas2D atlas;
        std::unordered_map<unicode_char, FontDetails::Glyph> glyphs;

        size_t characterPixelSize;

    public:
        explicit Font(
            OpenGL::TextureInternalFormat atlasFormat,
            Size2Di atlasSize, size_t characterPixelSize
        );

        std::optional<FontDetails::Glyph> getGlyph(unicode_char character) const noexcept override;
        bool tryAddGlyph(const FontDetails::FontUnit& unit) override;

        size_t getCharacterPixelSize() const noexcept;

        void initUndefinedCharacter(uint16_t size, PixelFormat format, uint8_t bytesPerPixel);

        void use(uint8_t fontSlot) const noexcept override;

        static std::shared_ptr<Font> createShared(
            OpenGL::TextureInternalFormat atlasFormat,
            Size2Di atlasSize, size_t characterPixelSize
        );
    };

    using FontPtr = std::shared_ptr<Font>;

    using CharacterFilter = std::function< bool(unicode_char)>;
    namespace FontLoaders
    {
        FontPtr loadFromFile(
            const std::string& path,
            uint16_t pixelSize,
            bool monochrome = false,
            CharacterFilter whitelistFilter = nullptr,
            uint32_t maxAtlasSize = 8192
        );
    }

} 