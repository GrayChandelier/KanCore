#pragma once
#include "Details.hpp"
#include "Utils.hpp"

#include "../include/TextureAtlas.hpp"
#include <unordered_map>
#include <optional>
#include <span>
#include <memory>
#include <ft2build.h>
#include <string>
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
			virtual bool tryAddGlyph(const FontDetails::FontUnit& unit) = 0;
			~IFontLoadable() = default;
		};

		//Interface for Text
		class IFont
		{
		public:
			virtual std::optional<FontDetails::Glyph> getGlyph(unicode_char character) const noexcept = 0;
			virtual void use(uint8_t fontSlot) const noexcept = 0;
			
			~IFont() = default;
		};

		
	}


	class Font : public FontDetails::IFont, public FontDetails::IFontLoadable
	{

	private:
		Graphics::DynamicTextureAtlas2D atlas;
		std::unordered_map<unicode_char, FontDetails::Glyph> glyphs;
	public:
		Font(OpenGL::TextureInternalFormat atlasFormat, Size2Di atlasSize = {1024, 1024})
			: atlas(atlasSize, atlasFormat)
		{
			
		}

		std::optional<FontDetails::Glyph> getGlyph(unicode_char character) const noexcept override
		{
			auto it = glyphs.find(character);
			if (it == glyphs.end())
				return std::nullopt;

			return std::make_optional(it->second);
		}
	

		bool tryAddGlyph(const FontDetails::FontUnit& glyph) override
		{
			if (glyphs.contains(glyph.unicode))
				return false;

			const FontDetails::GlyphData& data = glyph.glyphData;
			const FontDetails::Glyph& desc = glyph.glyphDesc;

			Size2Di size = { static_cast<size_t>(desc.size.x), static_cast<size_t>(desc.size.y) };


			Graphics::Image img;
			Graphics::ImageDetails::fillImage(img, size, glyph.glyphData.pixelFormat, data.data);

			glyphs.emplace(glyph.unicode, desc);
			atlas.insert(glyph.unicode, img);

			return true;
		}

		static std::shared_ptr<Font> createShared(OpenGL::TextureInternalFormat atlasFormat, Size2Di atlasSize)
		{
			return std::make_shared<Font>(atlasFormat, atlasSize);
		}

		void use(uint8_t fontSlot) const noexcept override
		{
			atlas.bind(fontSlot);
		}
	};
	
	using FontPtr = std::shared_ptr<Font>;
	namespace FontLoaders
	{
		FontPtr loadFromFile(
			const std::string& path,
			uint16_t pixelSize,
			bool(filterWhitelist)(unicode_char candidate) = nullptr,
			bool monochrome = false,
			uint32_t atlasSize = 1024)
		{
			//==========================================

			FT_Library ft;
			FT_Face face;

			if (FT_Init_FreeType(&ft))
				throw std::runtime_error("Failed to init FreeType");

			if (FT_New_Face(ft, path.c_str(), 0, &face))
				throw std::runtime_error("Failed to load font face");

			FT_Set_Pixel_Sizes(face, 0, pixelSize);

			int loadFlags = FT_LOAD_RENDER;
			if (!monochrome) loadFlags |= FT_LOAD_COLOR;


			//==========================================
			OpenGL::TextureInternalFormat atlasFormat = monochrome ? OpenGL::TextureInternalFormat::R8
				: OpenGL::TextureInternalFormat::RGBA8;

			auto font = Font::createShared(atlasFormat, { atlasSize, atlasSize });

			//==========================================

			FT_UInt glyphIndex;
			FT_ULong charcode = FT_Get_First_Char(face, &glyphIndex);

			while (glyphIndex != 0)
			{
				if (FT_Load_Glyph(face, glyphIndex, loadFlags))
				{
					charcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
					continue;
				}

				if (filterWhitelist && !filterWhitelist(charcode))
				{
					charcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
					continue;
				}

				FT_GlyphSlot slot = face->glyph;
				bool isColor = (slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA);

				FontDetails::Glyph g;
				g.size = { float(slot->bitmap.width), float(slot->bitmap.rows) };
				g.bearing = { float(slot->bitmap_left), float(slot->bitmap_top) };
				g.advance = float(slot->advance.x) / 64.0f;

				FontDetails::GlyphData gd;
				gd.dataType = OpenGL::TextureDataType::UBYTE;

				std::vector<uint8_t> rgbaBuffer; // буфер для RGBA
				size_t dataSize = size_t(slot->bitmap.width) * slot->bitmap.rows * (monochrome ? 1 : 4);

				if (monochrome || !isColor)
				{
					gd.pixelFormat = PixelFormat::Mono8;

					if (!monochrome)
					{
						auto monoSpan = std::span(slot->bitmap.buffer, slot->bitmap.width * slot->bitmap.rows);
						rgbaBuffer = Utils::Mono8_to_RGBA8(monoSpan);
						gd.data = std::span(rgbaBuffer.data(), rgbaBuffer.size());
						gd.pixelFormat = PixelFormat::RGBA8;
					}
					else
						gd.data = std::span(slot->bitmap.buffer, slot->bitmap.width * slot->bitmap.rows);
					
				}
				else
				{
					gd.pixelFormat = PixelFormat::BGRA8;
					gd.data = std::span(slot->bitmap.buffer, dataSize);
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


}