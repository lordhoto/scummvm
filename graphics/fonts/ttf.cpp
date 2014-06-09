/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// Since FreeType2 includes files, which contain forbidden symbols, we need to
// allow all symbols here.
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "common/scummsys.h"
#ifdef USE_FREETYPE2

#include "graphics/fonts/ttf.h"
#include "graphics/font.h"
#include "graphics/surface.h"

#include "common/singleton.h"
#include "common/stream.h"
#include "common/hashmap.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

namespace Graphics {

namespace {

inline int ftCeil26_6(FT_Pos x) {
	return (x + 63) / 64;
}

} // End of anonymous namespace

class TTFLibrary : public Common::Singleton<TTFLibrary> {
public:
	TTFLibrary();
	~TTFLibrary();

	/**
	 * Check whether FreeType2 is initialized properly.
	 */
	bool isInitialized() const { return _initialized; }

	bool loadFont(const uint8 *file, const uint32 size, FT_Face &face);
	void closeFont(FT_Face &face);
private:
	FT_Library _library;
	bool _initialized;
};

void shutdownTTF() {
	TTFLibrary::destroy();
}

#define g_ttf ::Graphics::TTFLibrary::instance()

TTFLibrary::TTFLibrary() : _library(), _initialized(false) {
	if (!FT_Init_FreeType(&_library))
		_initialized = true;
}

TTFLibrary::~TTFLibrary() {
	if (_initialized) {
		FT_Done_FreeType(_library);
		_initialized = false;
	}
}

bool TTFLibrary::loadFont(const uint8 *file, const uint32 size, FT_Face &face) {
	assert(_initialized);

	return (FT_New_Memory_Face(_library, file, size, 0, &face) == 0);
}

void TTFLibrary::closeFont(FT_Face &face) {
	assert(_initialized);

	FT_Done_Face(face);
}

class TTFFont : public Font {
public:
	TTFFont();
	virtual ~TTFFont();

	bool load(Common::SeekableReadStream &stream, int size, uint dpi, TTFRenderMode renderMode, const uint32 *mapping, bool forceBold);

	virtual int getFontHeight() const;

	virtual int getMaxCharWidth() const;

	virtual int getCharWidth(uint32 chr) const;

	virtual int getKerningOffset(uint32 left, uint32 right) const;

	virtual void drawChar(Surface *dst, uint32 chr, int x, int y, uint32 color) const;
private:
	bool _initialized;
	FT_Face _face;

	uint8 *_ttfFile;
	uint32 _size;

	int _width, _height;
	int _ascent, _descent;

	struct Glyph {
		Surface image;
		int xOffset, yOffset;
		int advance;
		FT_UInt slot;
	};

	bool cacheGlyph(Glyph &glyph, uint32 chr) const;
	typedef Common::HashMap<uint32, Glyph> GlyphCache;
	mutable GlyphCache _glyphs;
	bool _allowLateCaching;
	void assureCached(uint32 chr) const;

	FT_Int32 _loadFlags;
	FT_Render_Mode _renderMode;
	bool _hasKerning;
	bool _fakeBold;
};

TTFFont::TTFFont()
    : _initialized(false), _face(), _ttfFile(0), _size(0), _width(0), _height(0), _ascent(0),
      _descent(0), _glyphs(), _loadFlags(FT_LOAD_TARGET_NORMAL), _renderMode(FT_RENDER_MODE_NORMAL),
      _hasKerning(false), _fakeBold(false), _allowLateCaching(false) {
}

TTFFont::~TTFFont() {
	if (_initialized) {
		g_ttf.closeFont(_face);

		delete[] _ttfFile;
		_ttfFile = 0;

		for (GlyphCache::iterator i = _glyphs.begin(), end = _glyphs.end(); i != end; ++i)
			i->_value.image.free();

		_initialized = false;
	}
}

bool TTFFont::load(Common::SeekableReadStream &stream, int size, uint dpi, TTFRenderMode renderMode, const uint32 *mapping, bool forceBold) {
	if (!g_ttf.isInitialized())
		return false;

	_size = stream.size();
	if (!_size)
		return false;

	_ttfFile = new uint8[_size];
	assert(_ttfFile);

	if (stream.read(_ttfFile, _size) != _size) {
		delete[] _ttfFile;
		_ttfFile = 0;

		return false;
	}

	if (!g_ttf.loadFont(_ttfFile, _size, _face)) {
		delete[] _ttfFile;
		_ttfFile = 0;

		return false;
	}

	// We only support scalable fonts.
	if (!FT_IS_SCALABLE(_face)) {
		delete[] _ttfFile;
		_ttfFile = 0;

		g_ttf.closeFont(_face);

		return false;
	}

	// Check whether we need to fake a bold font in the Windows way.
	if (forceBold && !(_face->style_flags & FT_STYLE_FLAG_BOLD)) {
		_fakeBold = true;
	}

	// Check whether we have kerning support
	_hasKerning = (FT_HAS_KERNING(_face) != 0);

	if (FT_Set_Char_Size(_face, 0, size * 64, dpi, dpi)) {
		delete[] _ttfFile;
		_ttfFile = 0;

		return false;
	}

	switch (renderMode) {
	case kTTFRenderModeNormal:
		_loadFlags = FT_LOAD_TARGET_NORMAL;
		_renderMode = FT_RENDER_MODE_NORMAL;
		break;

	case kTTFRenderModeLight:
		_loadFlags = FT_LOAD_TARGET_LIGHT;
		_renderMode = FT_RENDER_MODE_LIGHT;
		break;

	case kTTFRenderModeMonochrome:
		_loadFlags = FT_LOAD_TARGET_MONO;
		_renderMode = FT_RENDER_MODE_MONO;
		break;
	}

	FT_Fixed yScale = _face->size->metrics.y_scale;
	_ascent = ftCeil26_6(FT_MulFix(_face->ascender, yScale));
	_descent = ftCeil26_6(FT_MulFix(_face->descender, yScale));

	_width = ftCeil26_6(FT_MulFix(_face->max_advance_width, _face->size->metrics.x_scale));
	// When faking bold we increase the width by one pixel.
	if (_fakeBold) {
		++_width;
	}
	_height = _ascent - _descent + 1;

	if (!mapping) {
		// Allow loading of all unicode characters.
		_allowLateCaching = true;

		// Load all ISO-8859-1 characters.
		for (uint i = 0; i < 256; ++i) {
			if (!cacheGlyph(_glyphs[i], i)) {
				_glyphs.erase(i);
			}
		}
	} else {
		// We have a fixed map of characters do not load more later.
		_allowLateCaching = false;

		for (uint i = 0; i < 256; ++i) {
			const uint32 unicode = mapping[i] & 0x7FFFFFFF;
			const bool isRequired = (mapping[i] & 0x80000000) != 0;
			// Check whether loading an important glyph fails and error out if
			// that is the case.
			if (!cacheGlyph(_glyphs[i], unicode)) {
				_glyphs.erase(i);
				if (isRequired)
					return false;
			}
		}
	}

	_initialized = (_glyphs.size() != 0);
	return _initialized;
}

int TTFFont::getFontHeight() const {
	return _height;
}

int TTFFont::getMaxCharWidth() const {
	return _width;
}

int TTFFont::getCharWidth(uint32 chr) const {
	assureCached(chr);
	GlyphCache::const_iterator glyphEntry = _glyphs.find(chr);
	if (glyphEntry == _glyphs.end())
		return 0;
	else
		return glyphEntry->_value.advance;
}

int TTFFont::getKerningOffset(uint32 left, uint32 right) const {
	if (!_hasKerning)
		return 0;

	assureCached(left);
	assureCached(right);

	FT_UInt leftGlyph, rightGlyph;
	GlyphCache::const_iterator glyphEntry;

	glyphEntry = _glyphs.find(left);
	if (glyphEntry != _glyphs.end()) {
		leftGlyph = glyphEntry->_value.slot;
	} else {
		return 0;
	}

	glyphEntry = _glyphs.find(right);
	if (glyphEntry != _glyphs.end()) {
		rightGlyph = glyphEntry->_value.slot;
	} else {
		return 0;
	}

	if (!leftGlyph || !rightGlyph)
		return 0;

	FT_Vector kerningVector;
	FT_Get_Kerning(_face, leftGlyph, rightGlyph, FT_KERNING_DEFAULT, &kerningVector);
	return (kerningVector.x / 64);
}

namespace {

template<typename ColorType>
void renderGlyph(uint8 *dstPos, const int dstPitch, const uint8 *srcPos, const int srcPitch, const int w, const int h, ColorType color, const PixelFormat &dstFormat) {
	uint8 sR, sG, sB;
	dstFormat.colorToRGB(color, sR, sG, sB);

	for (int y = 0; y < h; ++y) {
		ColorType *rDst = (ColorType *)dstPos;
		const uint8 *src = srcPos;

		for (int x = 0; x < w; ++x) {
			if (*src == 255) {
				*rDst = color;
			} else if (*src) {
				const uint8 a = *src;

				uint8 dR, dG, dB;
				dstFormat.colorToRGB(*rDst, dR, dG, dB);

				dR = ((255 - a) * dR + a * sR) / 255;
				dG = ((255 - a) * dG + a * sG) / 255;
				dB = ((255 - a) * dB + a * sB) / 255;

				*rDst = dstFormat.RGBToColor(dR, dG, dB);
			}

			++rDst;
			++src;
		}

		dstPos += dstPitch;
		srcPos += srcPitch;
	}
}

} // End of anonymous namespace

void TTFFont::drawChar(Surface *dst, uint32 chr, int x, int y, uint32 color) const {
	assureCached(chr);
	GlyphCache::const_iterator glyphEntry = _glyphs.find(chr);
	if (glyphEntry == _glyphs.end())
		return;

	const Glyph &glyph = glyphEntry->_value;

	x += glyph.xOffset;
	y += glyph.yOffset;

	if (x > dst->w)
		return;
	if (y > dst->h)
		return;

	int w = glyph.image.w;
	int h = glyph.image.h;

	const uint8 *srcPos = (const uint8 *)glyph.image.getPixels();

	// Make sure we are not drawing outside the screen bounds
	if (x < 0) {
		srcPos -= x;
		w += x;
		x = 0;
	}

	if (x + w > dst->w)
		w = dst->w - x;

	if (w <= 0)
		return;

	if (y < 0) {
		srcPos -= y * glyph.image.pitch;
		h += y;
		y = 0;
	}

	if (y + h > dst->h)
		h = dst->h - y;

	if (h <= 0)
		return;

	uint8 *dstPos = (uint8 *)dst->getBasePtr(x, y);

	if (dst->format.bytesPerPixel == 1) {
		for (int cy = 0; cy < h; ++cy) {
			uint8 *rDst = dstPos;
			const uint8 *src = srcPos;

			for (int cx = 0; cx < w; ++cx) {
				// We assume a 1Bpp mode is a color indexed mode, thus we can
				// not take advantage of anti-aliasing here.
				if (*src >= 0x80)
					*rDst = color;

				++rDst;
				++src;
			}

			dstPos += dst->pitch;
			srcPos += glyph.image.pitch;
		}
	} else if (dst->format.bytesPerPixel == 2) {
		renderGlyph<uint16>(dstPos, dst->pitch, srcPos, glyph.image.pitch, w, h, color, dst->format);
	} else if (dst->format.bytesPerPixel == 4) {
		renderGlyph<uint32>(dstPos, dst->pitch, srcPos, glyph.image.pitch, w, h, color, dst->format);
	}
}

namespace {

void blitMono(uint8 *dst, uint dstPitch, const uint8 *src, uint srcPitch, const int w, const int h) {
	for (int y = 0; y < h; ++y) {
		const uint8 *curSrc = src;
		uint8 *curDst = dst;
		uint8 mask = 0;

		for (int x = 0; x < w; ++x) {
			if ((x % 8) == 0)
				mask = *curSrc++;

			if (mask & 0x80)
				*curDst = 255;

			mask <<= 1;
			++curDst;
		}

		src += srcPitch;
		dst += dstPitch;
	}
}

} // End of anonymous namespace

bool TTFFont::cacheGlyph(Glyph &glyph, uint32 chr) const {
	FT_UInt slot = FT_Get_Char_Index(_face, chr);
	if (!slot)
		return false;

	glyph.slot = slot;

	// We use the light target and render mode to improve the looks of the
	// glyphs. It is most noticable in FreeSansBold.ttf, where otherwise the
	// 't' glyph looks like it is cut off on the right side.
	if (FT_Load_Glyph(_face, slot, _loadFlags))
		return false;

	// In case we fake bold rendering we need to adjust outline fonts and for
	// bitmap fonts we need to render it twice.
	FT_Glyph_Metrics metrics = _face->glyph->metrics;
	int xAdjust = 0;
	if (_fakeBold) {
		switch (_face->glyph->format) {
		case FT_GLYPH_FORMAT_OUTLINE: {
			// Method taken from http://bugs.winehq.org/show_bug.cgi?id=7520#c28
			const FT_Pos strength = _face->size->metrics.y_ppem * (1 << 6) / 24;
			const FT_Error err = FT_Outline_Embolden(&_face->glyph->outline, strength);
			if (err) {
				error("Could not embolden outline for faking bold: %d", err);
			}

			FT_BBox bbox;
			FT_Outline_Get_CBox(&_face->glyph->outline, &bbox);
			// We don't need to adjust all these values. However, to avoid
			// potentional future uses from resulting in crazy values we
			// just update all of these...
			metrics.width = bbox.xMax - bbox.xMin;
			metrics.height = bbox.yMax - bbox.yMin;
			metrics.horiBearingX = bbox.xMin;
			metrics.horiBearingY = bbox.yMax;
			metrics.horiAdvance += (1 << 6);
			metrics.vertAdvance += (1 << 6);
			metrics.vertBearingX = metrics.horiBearingX - metrics.horiAdvance / 2;
			metrics.vertBearingY = (metrics.vertAdvance - metrics.height) / 2;
			} break;

		case FT_GLYPH_FORMAT_BITMAP:
			if (_face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO) {
				error("Unsupported bitmap pixel mode for faking bold: %d", _face->glyph->bitmap.pixel_mode);
			}
			// We will render the glyph twice, like Windows does, thus we
			// need to adjust all horizontal widths.
			xAdjust = 1;
			break;

		default:
			error("Unsupported glyph format for faking bold: %d", _face->glyph->format);
		}
	}

	// We only try to render the gylph when it's not a bitmap already.
	if (_face->glyph->format != FT_GLYPH_FORMAT_BITMAP && FT_Render_Glyph(_face->glyph, _renderMode))
		return false;

	// (In)sanity check...
	if (_face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
		return false;

	glyph.xOffset = _face->glyph->bitmap_left;
	int xMax = glyph.xOffset + ftCeil26_6(metrics.width) + xAdjust;
	glyph.yOffset = _ascent - _face->glyph->bitmap_top;

	glyph.advance = ftCeil26_6(_face->glyph->advance.x) + xAdjust;

	// In case we got a negative xMin we adjust that, this might make some
	// characters make a bit odd, but it's the only way we can assure no
	// invalid memory writes with the current font API
	if (glyph.xOffset < 0) {
		xMax -= glyph.xOffset;
		glyph.xOffset = 0;

		if (xMax > glyph.advance)
			glyph.advance = xMax;
	}

	const FT_Bitmap &bitmap = _face->glyph->bitmap;
	glyph.image.create(bitmap.width + xAdjust, bitmap.rows, PixelFormat::createFormatCLUT8());

	const uint8 *src = bitmap.buffer;
	int srcPitch = bitmap.pitch;
	if (srcPitch < 0) {
		src += (bitmap.rows - 1) * srcPitch;
		srcPitch = -srcPitch;
	}

	uint8 *dst = (uint8 *)glyph.image.getPixels();
	memset(dst, 0, glyph.image.h * glyph.image.pitch);

	switch (bitmap.pixel_mode) {
	case FT_PIXEL_MODE_MONO:
		blitMono(dst, glyph.image.pitch, src, srcPitch, bitmap.width, bitmap.rows);
		if (_fakeBold) {
			blitMono(dst + 1, glyph.image.pitch, src, srcPitch, bitmap.width, bitmap.rows);
		}
		break;

	case FT_PIXEL_MODE_GRAY:
		for (int y = 0; y < bitmap.rows; ++y) {
			memcpy(dst, src, bitmap.width);
			dst += glyph.image.pitch;
			src += srcPitch;
		}
		break;

	default:
		warning("TTFFont::cacheGlyph: Unsupported pixel mode %d", bitmap.pixel_mode);
		glyph.image.free();
		return false;
	}

	return true;
}

void TTFFont::assureCached(uint32 chr) const {
	if (!chr || !_allowLateCaching || _glyphs.contains(chr)) {
		return;
	}

	Glyph newGlyph;
	if (cacheGlyph(newGlyph, chr)) {
		_glyphs[chr] = newGlyph;
	}
}

Font *loadTTFFont(Common::SeekableReadStream &stream, int size, uint dpi, TTFRenderMode renderMode, const uint32 *mapping, bool forceBold) {
	TTFFont *font = new TTFFont();

	if (!font->load(stream, size, dpi, renderMode, mapping, forceBold)) {
		delete font;
		return 0;
	}

	return font;
}

} // End of namespace Graphics

namespace Common {
DECLARE_SINGLETON(Graphics::TTFLibrary);
} // End of namespace Common

#endif

