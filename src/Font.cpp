#include "../include/Font.hpp"

#include <vector>
#include <stdexcept>
#include <iostream>
namespace KanCore::Graphics
{

    Font::Font(OpenGL::TextureInternalFormat atlasFormat, Size2Di atlasSize, size_t characterPixelSize)
        : atlas(atlasSize, atlasFormat, nullptr /*default packing strategy*/, 2 /*padding px*/), characterPixelSize(characterPixelSize)
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

        

        Size2Di size{
            static_cast<size_t>(unit.glyphDesc.size.x),
            static_cast<size_t>(unit.glyphDesc.size.y)
        };

        Graphics::Image img;
        Graphics::ImageDetails::fillImage(img, size, unit.glyphData.pixelFormat, unit.glyphData.data);

        std::optional<UVRect> place = atlas.insert(unit.unicode, img);
        bool inserted = place.has_value();

        if (inserted)
        {
            UVRect rect = place.value();
            FontDetails::Glyph desc = unit.glyphDesc;

            desc.uv.pos.x = rect.pos.x;
            desc.uv.pos.y = rect.pos.y;
            desc.uv.size.x = rect.size.x;
            desc.uv.size.y = rect.size.y;

            glyphs.emplace(unit.unicode, desc);
        }
       
        
        return inserted;
    }

    void Font::initUndefinedCharacter(uint16_t size, PixelFormat format, uint8_t bytesPerPixel) 
    {
        std::vector<uint8_t> data(size * size * bytesPerPixel, 0);

        uint8_t border = 255; 

        uint16_t thickness = size * 0.125;

        for (uint16_t y = 0; y < size; ++y)
        {
            for (uint16_t x = 0; x < size; ++x)
            {
                bool isBorder = (x < thickness) || (x >= size - thickness) ||
                    (y < thickness) || (y >= size - thickness);
                if (!isBorder) continue;

                uint32_t idx = (y * size + x) * bytesPerPixel;

                if (format == PixelFormat::RGBA8)
                {
                    data[idx + 0] = border;
                    data[idx + 1] = border;
                    data[idx + 2] = border;
                    data[idx + 3] = 255;   
                }
                else if (format == PixelFormat::Mono8)
                {
                    data[idx] = border;
                }
            }
        }



        FontDetails::FontUnit unit{};
        unit.glyphData.data = std::span(data);
        unit.glyphData.dataType = OpenGL::TextureDataType::UBYTE;
        unit.glyphData.pixelFormat = format;

        // √лиф Ч квадрат размером size x size
        unit.glyphDesc.size = { float(size), float(size) };
        unit.glyphDesc.bearing = { 0.f, float(size * 1.125) }; // подн€т на высоту квадрата
        unit.glyphDesc.advance = float(size * 1.25);          // шаг пера на ширину квадрата

        // UV будет рассчитан в tryAddGlyph при вставке в атлас
        unit.glyphDesc.uv = {};

        // —пециальный код дл€ undefined character
        unit.unicode = 0xFFFF;

        tryAddGlyph(unit);
        
    }
    void Font::use(uint8_t fontSlot) const noexcept
    {
       
        atlas.bind(fontSlot);
    }

    size_t Font::getCharacterPixelSize() const noexcept
    {
        return characterPixelSize;
    }
    std::shared_ptr<Font> Font::createShared(
        OpenGL::TextureInternalFormat atlasFormat,
        Size2Di atlasSize, size_t characterPixelSize)
    {
        return std::make_shared<Font>(atlasFormat, atlasSize, characterPixelSize);
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

    const size_t UNDEFINED_CHARACTER_SIZE = pixelSize * 0.5;
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

    totalArea += UNDEFINED_CHARACTER_SIZE * UNDEFINED_CHARACTER_SIZE;
    uint32_t atlasSide = 1;
    uint32_t minSide = static_cast<uint32_t>(std::ceil(std::sqrt(totalArea)));
    minSide = std::max(minSide, static_cast<uint32_t>(maxGlyphSide));

    while (atlasSide < minSide)
        atlasSide *= 2;

    if (atlasSide > maxAtlasSize)
        atlasSide = maxAtlasSize;

    std::cout << "Starting atlas size: " << atlasSide << " px\n";

    std::vector<std::vector<uint8_t>> rgbaBuffers; 

    auto font = Font::createShared(atlasFormat, { atlasSide, atlasSide }, pixelSize);

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

        if(isColor) g.colorful = true;

        FontDetails::FontUnit unit{
            static_cast<unicode_char>(charcode),
            g,
            gd
        };

        
        font->tryAddGlyph(unit);

        charcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
    }

    if(monochrome)
        font->initUndefinedCharacter(UNDEFINED_CHARACTER_SIZE, PixelFormat::Mono8, 1);
    else
        font->initUndefinedCharacter(UNDEFINED_CHARACTER_SIZE, PixelFormat::RGBA8, 4);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return font;
}




} 