/* ScummVM - Scumm Interpreter
 * Copyright (C) 2003 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#ifndef QUEEN_H
#define QUEEN_H

#include <stdio.h>
#include "base/engine.h"
#include "common/util.h"
#include "common/timer.h"
#include "sound/mixer.h"
#include "queen/resource.h"
#include "queen/logic.h"

namespace Queen {

class Graphics;
class Logic;
class Display;

class QueenEngine : public Engine {
	void errorString(const char *buf_input, char *buf_output);
protected:
	byte _game;
	byte _key_pressed;
	bool _quickLaunch; // set when starting with -x

	uint16 _debugMode;
	int _numScreenUpdates;

	int _number_of_savegames;
	int _sdl_mouse_x, _sdl_mouse_y;

	FILE *_dump_file;
	
	Graphics *_graphics;
	Resource *_resource;
	Logic *_logic;
	Display *_display;

	GameDetector *_detector; // necessary for music
	
public:
	QueenEngine(GameDetector *detector, OSystem *syst);
	virtual ~QueenEngine();

	void delay(uint amount);

protected:
	byte _fastMode;

	void go();

	//! Called when we go from one room to another
	void roomChanged(); // SETUP_ROOM

	void initialise();

	static void timerHandler(void *ptr);
	void gotTimerTick();
};

// XXX: Temporary hack to allow Graphics to call delay()
extern QueenEngine *g_queen;

} // End of namespace Queen

#endif
