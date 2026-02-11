#pragma once
#include "Details.hpp"
#include "OpenGL/GLTextureArray.hpp"
#include <unordered_map>
#include <optional>
#include <span>
#include <memory>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace KanCore::Graphics
{
	using unicode_char = uint32_t;

	namespace FontDetails
	{
		struct Glyph
		{
			Vec2f size;
			Vec2f bearing;
			float advance;
		};

		struct GlyphData
		{
			OpenGL::TextureDataFormat dataFormat;
			OpenGL::TextureDataType dataType;
			std::span<const uint8_t> data;

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

		class FontLoader
		{
		public:
			std::shared_ptr<Font> createFontObject(Size2Di maxRect, size_t layers, OpenGL::TextureInternalFormat format, int mipmap = 0)
			{
				return std::make_shared<Font>(maxRect, layers, format, mipmap);

			}
			bool tryAddGlyph(IFontLoadable& target, const FontUnit& glyph)
			{
				return target.tryAddGlyph(glyph);
			}
		};
	}


	class Font : public FontDetails::IFont, public FontDetails::IFontLoadable
	{
		friend class FontDetails::FontLoader;
	private:
		const Size2Di maxGlyphSize;
	    size_t nextLayer = 0;

		OpenGL::GLTexture2DArray glyphTextures;
		struct GlyphInfo
		{
			size_t layer;
			FontDetails::Glyph glyph;
		};
		std::unordered_map< unicode_char, GlyphInfo> glyphs;

		inline void addGlyphToTexture(const FontDetails::GlyphData& glyph, size_t layer) noexcept
		{
			glyphTextures.setLayerData(0, layer, glyph.dataFormat, glyph.dataType, glyph.data.data());
		}

		//for FontLoader
		bool tryAddGlyph(const FontDetails::FontUnit& unit) override
		{
			if (nextLayer >= glyphTextures.getLayersCount())
				return false;

			auto glyphIt = glyphs.find(unit.unicode);
			if (glyphIt != glyphs.end())
				return false;

			if (unit.glyphDesc.size.x > maxGlyphSize.width || unit.glyphDesc.size.y > maxGlyphSize.height)
				return false;

			glyphs.insert({ unit.unicode, GlyphInfo{nextLayer, unit.glyphDesc} });
			addGlyphToTexture(unit.glyphData, nextLayer);


			nextLayer++;
			return true;
		}

		std::optional<FontDetails::Glyph> getGlyph(unicode_char character) const noexcept override
		{
			auto glyphIt = glyphs.find(character);
			if (glyphIt == glyphs.end())
				return std::nullopt;

			return std::make_optional<FontDetails::Glyph>(glyphIt->second.glyph);
		}

		void use(uint8_t fontSlot) const noexcept override
		{
			glyphTextures.bind(fontSlot);
		}

		Font(Size2Di maxRect, size_t layers, OpenGL::TextureInternalFormat format, int mipmap = 0)
			: glyphTextures(maxRect.width, maxRect.height, layers, format, mipmap), maxGlyphSize(maxRect)
		{}
	};
	
	using FontPtr = std::shared_ptr<Font>;
	namespace FontLoaders
	{
		FontPtr loadFromFile(const std::string& path, uint16_t pixelSize = 48, bool monochrome = false)
		{

		}

	}

}