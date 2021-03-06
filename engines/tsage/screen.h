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

#ifndef TSAGE_SCREEN_H
#define TSAGE_SCREEN_H

#include "common/scummsys.h"
#include "common/array.h"
#include "graphics/screen.h"
#include "tsage/graphics.h"

namespace TsAGE {

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_CENTER_X 160
#define SCREEN_CENTER_Y 100
#define UI_INTERFACE_Y 168

class Screen : virtual public Graphics::Screen, virtual public GfxSurface {
public:
	/**
	 * Constructor
	 */
	Screen();

	/**
	 * Destructor
	 */
	virtual ~Screen() {}

	/**
	 * Update the screen
	 */
	virtual void update();
};

} // End of namespace TsAGE

#endif /* MADS_SCREEN_H */
