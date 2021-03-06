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

#include "scumm/he/intern_he.h"

namespace Scumm {

Moonbase::Moonbase() {
	_fowSentinelImage = -1;
	_fowSentinelState = -1;
	_fowSentinelConditionBits = 0;
}

Moonbase::~Moonbase() {
}

void Moonbase::renderFOW() {
	warning("STUB: renderFOW()");
}

void Moonbase::blitT14WizImage(uint8 *dst, int dstw, int dsth, int dstPitch, const Common::Rect *clipBox,
		 uint8 *wizd, int x, int y, int rawROP, int paramROP) {

	Common::Rect clippedDstRect(dstw, dsth);
	if (clipBox) {
		Common::Rect clip(clipBox->left, clipBox->top, clipBox->right, clipBox->bottom);
		if (clippedDstRect.intersects(clip)) {
			clippedDstRect.clip(clip);
		} else {
			return;
		}
	}

	int width = READ_LE_UINT16(wizd + 0x8 + 0);
	int height = READ_LE_UINT16(wizd + 0x8 + 2);

	Common::Rect srcLimitsRect(width, height);
	Common::Rect dstOperation(x, y, x + width, y + height);
	if (!clippedDstRect.intersects(dstOperation))
		return;
	Common::Rect clippedRect = clippedDstRect.findIntersectingRect(dstOperation);

	int cx = clippedRect.right - clippedRect.left;
	int cy = clippedRect.bottom - clippedRect.top;

	int sx = ((clippedRect.left - x) + srcLimitsRect.left);
	int sy = ((clippedRect.top - y) + srcLimitsRect.top);

	dst += clippedRect.top * dstPitch + clippedRect.left * 2;

	int headerSize = READ_LE_UINT32(wizd + 0x4);
	uint8 *dataPointer = wizd + 0x8 + headerSize;

	for (int i = 0; i < sy; i++) {
		uint16 lineSize = READ_LE_UINT16(dataPointer + 0);

		dataPointer += lineSize;
	}

	for (int i = 0; i < cy; i++) {
		uint16 lineSize      = READ_LE_UINT16(dataPointer + 0);
		uint8 *singlesOffset = READ_LE_UINT16(dataPointer + 2) + dataPointer;
		uint8 *quadsOffset   = READ_LE_UINT16(dataPointer + 4) + dataPointer;

		int pixels = 0;
		byte *dst1 = dst;
		byte *codes = dataPointer + 6;

		while (1) {
			int code = *codes - 2;
			codes++;

			if (code == 0) { // quad
				for (int c = 0; c < 4; c++) {
					if (pixels >= sx) {
						WRITE_LE_UINT16(dst1, READ_LE_UINT16(quadsOffset));
						dst1 += 2;
					}
					quadsOffset += 2;
					pixels++;
				}
			} else if (code < 0) { // single
				if (pixels >= sx) {
					WRITE_LE_UINT16(dst1, READ_LE_UINT16(singlesOffset));
					dst1 += 2;
				}
				singlesOffset += 2;
				pixels++;
			} else { // skip
				if ((code & 1) == 0) {
					code >>= 1;

					for (int j = 0; j < code; j++) {
						if (pixels >= sx)
							dst1 += 2;
						pixels++;
					}
				} else { // special case
					if (pixels >= sx) {
						int alpha = code >> 1;
						uint16 color = READ_LE_UINT16(singlesOffset);

						//WRITE_LE_UINT16(dst1, color); // ENABLE_PREMUL_ALPHA = 0
						// ENABLE_PREMUL_ALPHA = 2
						if (alpha > 32) {
							alpha -= 32;

							uint32 orig = READ_LE_UINT16(dst1);
							uint32 oR = orig & 0x7c00;
							uint32 oG = orig & 0x03e0;
							uint32 oB = orig & 0x1f;
							uint32 dR = ((((color & 0x7c00) - oR) * alpha) >> 5) + oR;
							uint32 dG = ((((color &  0x3e0) - oG) * alpha) >> 5) + oG;
							uint32 dB = ((((color &   0x1f) - oB) * alpha) >> 5) + oB;

							WRITE_LE_UINT16(dst1, (dR & 0x7c00) | (dG & 0x3e0) | (dB & 0x1f));
						} else {
							uint32 orig = READ_LE_UINT16(dst1);
							uint32 pass1 = ((((orig << 16) | orig) & 0x3e07c1f * alpha) >> 5) & 0x3e07c1f;
							uint32 pass2 = (((pass1 << 16) | pass1) + color) & 0xffff;
							WRITE_LE_UINT16(dst1, pass2);
						}

						dst1 += 2;
					}
					singlesOffset += 2;
					pixels++;
				}
			}

			if (pixels >= cx + sx)
				break;
		}

		dataPointer += lineSize;
		dst += dstPitch;
	}
}

} // End of namespace Scumm
