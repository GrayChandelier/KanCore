#include "../include/Font.hpp"

#include <vector>
#include <stdexcept>
#include <iostream>
namespace KanCore::Graphics
{

    Font::Font(OpenGL::TextureInternalFormat atlasFormat, Size2Di atlasSize)
        : atlas(atlasSize, atlasFormat)
    {
    }


    std::optional<FontDetails::Glyph> Font::getGlyph(unicode_char character) const noexcept
    {
        auto it = glyphs.find(character);
        if (it == glyphs.end())
            return std::nullopt;
        return it->second;
    }


    bool Font::tryAddGlyph(const FontDetails::FontUnit& unit)
    {
        if (glyphs.contains(unit.unicode))
            return false;

        const auto& desc = unit.glyphDesc;
        const auto& data = unit.glyphData;

        Size2Di size{
            static_cast<size_t>(desc.size.x),
            static_cast<size_t>(desc.size.y)
        };

        Graphics::Image img;
        Graphics::ImageDetails::fillImage(img, size, data.pixelFormat, data.data);

        bool inserted = atlas.insert(unit.unicode, img).has_value();

        if(inserted) glyphs.emplace(unit.unicode, desc);
       

        return inserted;
    }


    void Font::use(uint8_t fontSlot) const noexcept
    {
        atlas.bind(fontSlot);
    }


    std::shared_ptr<Font> Font::createShared(
        OpenGL::TextureInternalFormat atlasFormat,
        Size2Di atlasSize)
    {
        return std::make_shared<Font>(atlasFormat, atlasSize);
    }

    FontPtr FontLoaders::loadFromFile(
    const std::string& path,
    uint16_t pixelSize,
    bool monochrome,
    CharacterFilter whitelistFilter,

    uint32_t maxAtlasSize)
{
    FT_Library ft{};
    FT_Face face{};

    if (FT_Init_FreeType(&ft))
        throw std::runtime_error("Failed to init FreeType");

    if (FT_New_Face(ft, path.c_str(), 0, &face))
    {
        FT_Done_FreeType(ft);
        throw std::runtime_error("Failed to load font face from: " + path);
    }

    FT_Set_Pixel_Sizes(face, 0, pixelSize);

    int loadFlags = FT_LOAD_RENDER;
    if (!monochrome) loadFlags |= FT_LOAD_COLOR;

    OpenGL::TextureInternalFormat atlasFormat =
        monochrome ? OpenGL::TextureInternalFormat::R8
                   : OpenGL::TextureInternalFormat::RGBA8;

    size_t totalArea = 0;
    int maxGlyphSide = 0;

    FT_UInt glyphIndex;
    FT_ULong charcode = FT_Get_First_Char(face, &glyphIndex);

    while (glyphIndex != 0)
    {
        if (FT_Load_Glyph(face, glyphIndex, loadFlags) == 0)
        {
            if (!whitelistFilter || whitelistFilter(charcode))
            {
                FT_GlyphSlot slot = face->glyph;
                int w = slot->bitmap.width;
                int h = slot->bitmap.rows;
                totalArea += w * h;
                maxGlyphSide = std::max(maxGlyphSide, std::max(w, h));
            }
        }
        charcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
    }

    uint32_t atlasSide = 1;
    uint32_t minSide = static_cast<uint32_t>(std::ceil(std::sqrt(totalArea)));
    minSide = std::max(minSide, static_cast<uint32_t>(maxGlyphSide));

    while (atlasSide < minSide)
        atlasSide *= 2;

    if (atlasSide > maxAtlasSize)
        atlasSide = maxAtlasSize;

    std::cout << "Starting atlas size: " << atlasSide << " px\n";

    std::vector<std::vector<uint8_t>> rgbaBuffers; 

    auto font = Font::createShared(atlasFormat, { atlasSide, atlasSide });

    charcode = FT_Get_First_Char(face, &glyphIndex);

    while (glyphIndex != 0)
    {
        if (FT_Load_Glyph(face, glyphIndex, loadFlags))
        {
            charcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
            continue;
        }

        if (whitelistFilter && !whitelistFilter(charcode))
        {
            charcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
            continue;
        }

        FT_GlyphSlot slot = face->glyph;
        bool isColor = (slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA);

        FontDetails::Glyph g{};
        g.size = { float(slot->bitmap.width), float(slot->bitmap.rows) };
        g.bearing = { float(slot->bitmap_left), float(slot->bitmap_top) };
        g.advance = float(slot->advance.x) / 64.0f;

        FontDetails::GlyphData gd{};
        gd.dataType = OpenGL::TextureDataType::UBYTE;

        if (monochrome || !isColor)
        {
            gd.pixelFormat = PixelFormat::Mono8;

            if (!monochrome)
            {
                std::span span(slot->bitmap.buffer,
                               size_t(slot->bitmap.width) * slot->bitmap.rows);
                rgbaBuffers.emplace_back(Utils::Mono8_to_RGBA8(span));
                gd.data = std::span(rgbaBuffers.back());
                gd.pixelFormat = PixelFormat::RGBA8;
            }
            else
            {
                gd.data = std::span(slot->bitmap.buffer,
                                    size_t(slot->bitmap.width) * slot->bitmap.rows);
            }
        }
        else
        {
            gd.pixelFormat = PixelFormat::BGRA8;
            gd.data = std::span(slot->bitmap.buffer,
                                size_t(slot->bitmap.width) * slot->bitmap.rows * 4);
        }

        FontDetails::FontUnit unit{
            static_cast<unicode_char>(charcode),
            g,
            gd
        };

        font->tryAddGlyph(unit);

        charcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return font;
}




} 