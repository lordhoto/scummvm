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

#include "common/scummsys.h"

#ifdef ENABLE_SCI32

#include "sci/engine/state.h"
#include "sci/engine/kernel.h"

namespace Sci {

reg_t kArray(EngineState *s, int argc, reg_t *argv) {
	if (!s)
		return make_reg(0, getSciVersion());
	error("not supposed to call this");
}

reg_t kArrayNew(EngineState *s, int argc, reg_t *argv) {
	reg_t arrayHandle;
	SciArray<reg_t> *array = s->_segMan->allocateArray(&arrayHandle);
	array->setType(argv[1].toUint16());
	array->setSize(argv[0].toUint16());
	return arrayHandle;
}

reg_t kArraySize(EngineState *s, int argc, reg_t *argv) {
	SciArray<reg_t> *array = s->_segMan->lookupArray(argv[1]);
	return make_reg(0, array->getSize());
}

reg_t kArrayAt(EngineState *s, int argc, reg_t *argv) {
	SciArray<reg_t> *array = s->_segMan->lookupArray(argv[0]);
	if (g_sci->getGameId() == GID_PHANTASMAGORIA2) {
		// HACK: Phantasmagoria 2 keeps trying to access past the end of an
		// array when it starts. I'm assuming it's trying to see where the
		// array ends, or tries to resize it. Adjust the array size
		// accordingly, and return NULL for now.
		if (array->getSize() == argv[1].toUint16()) {
			array->setSize(argv[1].toUint16());
			return NULL_REG;
		}
	}
	return array->getValue(argv[1].toUint16());
}

reg_t kArrayPut(EngineState *s, int argc, reg_t *argv) {
	SciArray<reg_t> *array = s->_segMan->lookupArray(argv[0]);

	uint32 index = argv[1].toUint16();
	uint32 count = argc - 2;

	if (index + count > 65535)
		return NULL_REG;

	if (array->getSize() < index + count)
		array->setSize(index + count);

	for (uint16 i = 0; i < count; i++)
		array->setValue(i + index, argv[i + 2]);

	return argv[0]; // We also have to return the handle
}

reg_t kArrayFree(EngineState *s, int argc, reg_t *argv) {
	// Freeing of arrays is handled by the garbage collector
	return s->r_acc;
}

reg_t kArrayFill(EngineState *s, int argc, reg_t *argv) {
	SciArray<reg_t> *array = s->_segMan->lookupArray(argv[0]);
	uint16 index = argv[1].toUint16();

	// A count of -1 means fill the rest of the array
	uint16 count = argv[2].toSint16() == -1 ? array->getSize() - index : argv[2].toUint16();
	uint16 arraySize = array->getSize();

	if (arraySize < index + count)
		array->setSize(index + count);

	for (uint16 i = 0; i < count; i++)
		array->setValue(i + index, argv[3]);

	return argv[0];
}

reg_t kArrayCpy(EngineState *s, int argc, reg_t *argv) {
	if (argv[0].isNull() || argv[2].isNull()) {
		if (getSciVersion() == SCI_VERSION_3) {
			// FIXME: Happens in SCI3, probably because of a missing kernel function.
			warning("kArrayCpy: Request to copy from or to a null pointer");
			return NULL_REG;
		} else {
			// SCI2-2.1: error out
			error("kArrayCpy: Request to copy from or to a null pointer");
		}
	}

	reg_t arrayHandle = argv[0];
	SciArray<reg_t> *array1 = s->_segMan->lookupArray(argv[0]);
	SciArray<reg_t> *array2 = s->_segMan->lookupArray(argv[2]);
	uint32 index1 = argv[1].toUint16();
	uint32 index2 = argv[3].toUint16();

	// The original engine ignores bad copies too
	if (index2 > array2->getSize())
		return NULL_REG;

	// A count of -1 means fill the rest of the array
	uint32 count = argv[4].toSint16() == -1 ? array2->getSize() - index2 : argv[4].toUint16();

	if (array1->getSize() < index1 + count)
		array1->setSize(index1 + count);

	for (uint16 i = 0; i < count; i++)
		array1->setValue(i + index1, array2->getValue(i + index2));

	return arrayHandle;
}

reg_t kArrayDup(EngineState *s, int argc, reg_t *argv) {
	if (argv[0].isNull()) {
		warning("kArrayDup: Request to duplicate a null pointer");
#if 0
		// Allocate an array anyway
		reg_t arrayHandle;
		SciArray<reg_t> *dupArray = s->_segMan->allocateArray(&arrayHandle);
		dupArray->setType(3);
		dupArray->setSize(0);
		return arrayHandle;
#endif
		return NULL_REG;
	}
	SegmentObj *sobj = s->_segMan->getSegmentObj(argv[0].getSegment());
	if (!sobj || sobj->getType() != SEG_TYPE_ARRAY)
		error("kArrayDup: Request to duplicate a segment which isn't an array");

	reg_t arrayHandle;
	SciArray<reg_t> *dupArray = s->_segMan->allocateArray(&arrayHandle);
	// This must occur after allocateArray, as inserting a new object
	// in the heap object list might invalidate this pointer. Also refer
	// to the same issue in kClone()
	SciArray<reg_t> *array = s->_segMan->lookupArray(argv[0]);

	dupArray->setType(array->getType());
	dupArray->setSize(array->getSize());

	for (uint32 i = 0; i < array->getSize(); i++)
		dupArray->setValue(i, array->getValue(i));

	return arrayHandle;
}

reg_t kArrayGetData(EngineState *s, int argc, reg_t *argv) {
	if (!s->_segMan->isHeapObject(argv[0]))
		return argv[0];

	return readSelector(s->_segMan, argv[0], SELECTOR(data));
}

} // End of namespace Sci

#endif
