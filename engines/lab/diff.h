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

/*
 * This code is based on Labyrinth of Time code with assistance of
 *
 * Copyright (c) 1993 Terra Nova Development
 * Copyright (c) 2004 The Wyrmkeep Entertainment Co.
 *
 */

#ifndef LAB_DIFF_H
#define LAB_DIFF_H

#include "common/file.h"

namespace Lab {


struct DIFFHeader {
	uint16 _version;    // unused
	uint16 _width;
	uint16 _height;
	char _depth;        // unused
	char _fps;
	uint32 _bufferSize; // unused
	uint16 _machine;    // unused
	uint32 _flags;
};

struct BitMap {
	uint16 _bytesPerRow;
	uint16 _rows;       // unused
	byte _flags;
	byte *_planes[16];
};

#define BITMAPF_NONE  0
#define BITMAPF_VIDEO (1<<7)

/* unDiff.c */

void initOffsets(uint16 bytesPerRow);

bool unDIFFMemory(byte *dest,           /* Where to Un-DIFF */
                  byte *diff,           /* The DIFFed code. */
                  uint16 headerSize,    /* Size of header (1, 2 or 4 bytes) (only supports 1 currently */
                  uint16 copySize);     /* Size of minimum copy or skip. (1, 2 or 4 bytes) */

bool VUnDIFFMemory(byte *Dest, byte *diff, uint16 HeaderSize, uint16 CopySize, uint16 bytesperrow);
void runLengthDecode(byte *Dest, byte *Source);
void VRunLengthDecode(byte *Dest, byte *Source, uint16 bytesperrow);

/* readDiff.c */

void blackScreen();
void blackAllScreen();
void whiteScreen();
bool readDiff(bool playonce);
void diffNextFrame();
void readSound(bool waitTillFinished, Common::File *file);
void stopDiff();
void stopDiffEnd();
void stopSound();

} // End of namespace Lab

#endif /* LAB_DIFF_H */


