/* ScummVM - Scumm Interpreter
 * Copyright (C) 2005-2006 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef LURE_DEBUGGER_H
#define LURE_DEBUGGER_H

#include "common/debugger.h"

namespace Lure {

class KyraEngine;

class Debugger : public Common::Debugger<Debugger> {
private:
	int strToInt(const char *s);
public:
	Debugger();
	virtual ~Debugger() {}  // we need this for __SYMBIAN32__ archaic gcc/UIQ

protected:
	virtual void preEnter();
	virtual void postEnter();

	bool cmd_exit(int argc, const char **argv);
	bool cmd_help(int argc, const char **argv);
	bool cmd_enterRoom(int argc, const char **argv);
	bool cmd_listRooms(int argc, const char **argv);
	bool cmd_listFields(int argc, const char **argv);
	bool cmd_setField(int argc, const char **argv);
	bool cmd_queryField(int argc, const char **argv);
	bool cmd_giveItem(int argc, const char **argv);
	bool cmd_hotspots(int argc, const char **argv);
	bool cmd_hotspot(int argc, const char **argv);
};

} // End of namespace Lure

#endif
