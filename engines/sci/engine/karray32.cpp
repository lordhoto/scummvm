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
	uint16 op = argv[0].toUint16();

	// Use kString when accessing strings
	// This is possible, as strings inherit from arrays
	// and in this case (type 3) arrays are of type char *.
	// kString is almost exactly the same as kArray, so
	// this call is possible
	// TODO: we need to either merge SCI2 strings and
	// arrays together, and in the future merge them with
	// the SCI1 strings and arrays in the segment manager
	bool callStringFunc = false;
	if (op == 0) {
		// New, check if the target type is 3 (string)
		if (argv[2].toUint16() == 3)
			callStringFunc = true;
	} else {
		if (s->_segMan->getSegmentType(argv[1].getSegment()) == SEG_TYPE_STRING ||
			s->_segMan->getSegmentType(argv[1].getSegment()) == SEG_TYPE_SCRIPT) {
			callStringFunc = true;
		}

#if 0
		if (op == 6) {
			if (s->_segMan->getSegmentType(argv[3].getSegment()) == SEG_TYPE_STRING ||
				s->_segMan->getSegmentType(argv[3].getSegment()) == SEG_TYPE_SCRIPT) {
				callStringFunc = true;
			}
		}
#endif
	}

	if (callStringFunc) {
		Kernel *kernel = g_sci->getKernel();
		uint16 kernelStringFuncId = kernel->_kernelFunc_StringId;
		if (kernelStringFuncId) {
			const KernelFunction *kernelStringFunc = &kernel->_kernelFuncs[kernelStringFuncId];

			if (op < kernelStringFunc->subFunctionCount) {
				// subfunction-id is valid
				const KernelSubFunction *kernelStringSubCall = &kernelStringFunc->subFunctions[op];
				argc--;
				argv++; // remove subfunction-id from arguments
				// and call the kString subfunction
				return kernelStringSubCall->function(s, argc, argv);
			}
		}
	}

	switch (op) {
	case 0: { // New
		reg_t arrayHandle;
		SciArray<reg_t> *array = s->_segMan->allocateArray(&arrayHandle);
		array->setType(argv[2].toUint16());
		array->setSize(argv[1].toUint16());
		return arrayHandle;
	}
	case 1: { // Size
		SciArray<reg_t> *array = s->_segMan->lookupArray(argv[1]);
		return make_reg(0, array->getSize());
	}
	case 2: { // At (return value at an index)
		SciArray<reg_t> *array = s->_segMan->lookupArray(argv[1]);
		if (g_sci->getGameId() == GID_PHANTASMAGORIA2) {
			// HACK: Phantasmagoria 2 keeps trying to access past the end of an
			// array when it starts. I'm assuming it's trying to see where the
			// array ends, or tries to resize it. Adjust the array size
			// accordingly, and return NULL for now.
			if (array->getSize() == argv[2].toUint16()) {
				array->setSize(argv[2].toUint16());
				return NULL_REG;
			}
		}
		return array->getValue(argv[2].toUint16());
	}
	case 3: { // Atput (put value at an index)
		SciArray<reg_t> *array = s->_segMan->lookupArray(argv[1]);

		uint32 index = argv[2].toUint16();
		uint32 count = argc - 3;

		if (index + count > 65535)
			break;

		if (array->getSize() < index + count)
			array->setSize(index + count);

		for (uint16 i = 0; i < count; i++)
			array->setValue(i + index, argv[i + 3]);

		return argv[1]; // We also have to return the handle
	}
	case 4: // Free
		// Freeing of arrays is handled by the garbage collector
		return s->r_acc;
	case 5: { // Fill
		SciArray<reg_t> *array = s->_segMan->lookupArray(argv[1]);
		uint16 index = argv[2].toUint16();

		// A count of -1 means fill the rest of the array
		uint16 count = argv[3].toSint16() == -1 ? array->getSize() - index : argv[3].toUint16();
		uint16 arraySize = array->getSize();

		if (arraySize < index + count)
			array->setSize(index + count);

		for (uint16 i = 0; i < count; i++)
			array->setValue(i + index, argv[4]);

		return argv[1];
	}
	case 6: { // Cpy
		if (argv[1].isNull() || argv[3].isNull()) {
			if (getSciVersion() == SCI_VERSION_3) {
				// FIXME: Happens in SCI3, probably because of a missing kernel function.
				warning("kArray(Cpy): Request to copy from or to a null pointer");
				return NULL_REG;
			} else {
				// SCI2-2.1: error out
				error("kArray(Cpy): Request to copy from or to a null pointer");
			}
		}

		reg_t arrayHandle = argv[1];
		SciArray<reg_t> *array1 = s->_segMan->lookupArray(argv[1]);
		SciArray<reg_t> *array2 = s->_segMan->lookupArray(argv[3]);
		uint32 index1 = argv[2].toUint16();
		uint32 index2 = argv[4].toUint16();

		// The original engine ignores bad copies too
		if (index2 > array2->getSize())
			break;

		// A count of -1 means fill the rest of the array
		uint32 count = argv[5].toSint16() == -1 ? array2->getSize() - index2 : argv[5].toUint16();

		if (array1->getSize() < index1 + count)
			array1->setSize(index1 + count);

		for (uint16 i = 0; i < count; i++)
			array1->setValue(i + index1, array2->getValue(i + index2));

		return arrayHandle;
	}
	case 7: // Cmp
		// Not implemented in SSCI
		warning("kArray(Cmp) called");
		return s->r_acc;
	case 8: { // Dup
		if (argv[1].isNull()) {
			warning("kArray(Dup): Request to duplicate a null pointer");
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
		SegmentObj *sobj = s->_segMan->getSegmentObj(argv[1].getSegment());
		if (!sobj || sobj->getType() != SEG_TYPE_ARRAY)
			error("kArray(Dup): Request to duplicate a segment which isn't an array");

		reg_t arrayHandle;
		SciArray<reg_t> *dupArray = s->_segMan->allocateArray(&arrayHandle);
		// This must occur after allocateArray, as inserting a new object
		// in the heap object list might invalidate this pointer. Also refer
		// to the same issue in kClone()
		SciArray<reg_t> *array = s->_segMan->lookupArray(argv[1]);

		dupArray->setType(array->getType());
		dupArray->setSize(array->getSize());

		for (uint32 i = 0; i < array->getSize(); i++)
			dupArray->setValue(i, array->getValue(i));

		return arrayHandle;
	}
	case 9: // Getdata
		if (!s->_segMan->isHeapObject(argv[1]))
			return argv[1];

		return readSelector(s->_segMan, argv[1], SELECTOR(data));
	default:
		error("Unknown kArray subop %d", op);
	}

	return NULL_REG;
}

} // End of namespace Sci

#endif
