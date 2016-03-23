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

#include "sci/engine/karray32.h"
#include "sci/engine/state.h"
#include "sci/engine/kernel.h"
#include "sci/engine/savegame.h"

#include "common/algorithm.h"

namespace Sci {

reg_t kArray(EngineState *s, int argc, reg_t *argv) {
	if (!s)
		return make_reg(0, getSciVersion());
	error("not supposed to call this");
}

reg_t kArrayNew(EngineState *s, int argc, reg_t *argv) {
	reg_t arrayHandle;
	Array32 *const array = s->_segMan->allocateArray(argv[1].toUint16(), &arrayHandle);
	array->setSize(argv[0].toUint16());
	return arrayHandle;
}

reg_t kArraySize(EngineState *s, int argc, reg_t *argv) {
	const reg_t arrayRef = argv[1];
	if (arrayRef.isNull()) {
		error("kArraySize: Invalid null reference as parameter");
	}

	Array32 *const array = s->_segMan->lookupArray(arrayRef);
	return make_reg(0, array->getSize());
}

reg_t kArrayAt(EngineState *s, int argc, reg_t *argv) {
	const reg_t arrayRef = argv[0];
	if (arrayRef.isNull()) {
		error("kArrayAt: Invalid null reference as parameter");
	}

	Array32 *const array = s->_segMan->lookupArray(arrayRef);
	return array->getElement(argv[1].toUint16());
}

reg_t kArrayPut(EngineState *s, int argc, reg_t *argv) {
	const reg_t arrayRef = argv[0];
	if (arrayRef.isNull()) {
		error("kArrayPut: Invalid null reference as parameter");
	}

	Array32 *const array = s->_segMan->lookupArray(arrayRef);

	const uint index = argv[1].toUint16();
	const uint count = argc - 2;

	if (index + count > 65535) {
		error("kArrayPut: Array size too big %u + %u > 65535", index, count);
	}

	array->assureSize(index + count);
	array->setElements(index, count, argv + 2);

	return arrayRef;
}

reg_t kArrayFree(EngineState *s, int argc, reg_t *argv) {
	if (argv[0].isNull()) {
		error("kArrayFree: Invalid null reference as parameter");
	}

	// Freeing of arrays is handled by the garbage collector
	return s->r_acc;
}

reg_t kArrayFill(EngineState *s, int argc, reg_t *argv) {
	const reg_t arrayRef = argv[0];
	if (arrayRef.isNull()) {
		error("kArrayFill: Invalid null reference as parameter");
	}

	Array32 *const array = s->_segMan->lookupArray(arrayRef);
	const uint index = argv[1].toUint16();

	// A count of -1 means fill the rest of the array.
	const uint arraySize = array->getSize();
	const uint count = argv[2].toSint16() == -1 ? arraySize - index : argv[2].toUint16();

	array->assureSize(index + count);
	array->fill(index, count, argv[3]);

	return arrayRef;
}

reg_t kArrayCpy(EngineState *s, int argc, reg_t *argv) {
	const reg_t dstArrayRef = argv[0];
	if (dstArrayRef.isNull()) {
		error("kArrayCpy: Invalid null reference as destination parameter");
	}

	const reg_t srcArrayRef = argv[2];
	if (dstArrayRef.isNull()) {
		error("kArrayCpy: Invalid null reference as source parameter");
	}

	const Array32 *const src = s->_segMan->lookupArray(srcArrayRef);
	Array32 *const dst = s->_segMan->lookupArray(dstArrayRef);

	const uint srcIndex = argv[3].toUint16();
	const uint srcSize = src->getSize();

	// A count of -1 means copy the rest of the array.
	const bool copyRest = argv[4].toSint16() == -1;

	// NOTE: The original only checks out of bounds reads when no count is
	// given. We on the other do more strict checks later on. Thus, we bail
	// out early to replicate original behavior.
	if (copyRest && srcIndex >= srcSize) {
		return dstArrayRef;
	}

	const uint count = copyRest ? srcSize - srcIndex : argv[4].toUint16();

	const uint dstIndex = argv[1].toUint16();
	if (dstIndex + count > 65535) {
		error("kArrayCpy: Array size too big %u + %u > 65535", dstIndex, count);
	}

	dst->assureSize(dstIndex + count);
	dst->copyElements(src, srcIndex, dstIndex, count);

	return dstArrayRef;
}

reg_t kArrayDup(EngineState *s, int argc, reg_t *argv) {
	const reg_t srcArrayRef = argv[0];
	if (srcArrayRef.isNull()) {
		error("kArrayDup: Invalid null reference as source");
	}

	// TODO: The original also allows to duplicate strings here. We might want
	// to implement this behavior too or try to look into adapting game
	// scripts, if they use kArrayDup to duplicate strings.
	const SegmentObj *const sobj = s->_segMan->getSegmentObj(argv[0].getSegment());
	if (!sobj || sobj->getType() != SEG_TYPE_ARRAY) {
		error("kArrayDup: Request to duplicate a segment which isn't an array");
	}

	// Due to our way we store arrays in ArrayTable we can look up fine here
	// and do not worry about the pointer getting invalidated later.
	const Array32 *const src = s->_segMan->lookupArray(argv[0]);

	reg_t dstArrayRef;
	Array32 *const dst = s->_segMan->allocateArray(src->getType(), &dstArrayRef);

	const uint srcSize = src->getSize();
	dst->setSize(srcSize);
	dst->copyElements(src, 0, 0, srcSize);

	return dstArrayRef;
}

reg_t kArrayGetData(EngineState *s, int argc, reg_t *argv) {
	if (!s->_segMan->isHeapObject(argv[0]))
		return argv[0];

	return readSelector(s->_segMan, argv[0], SELECTOR(data));
}

#pragma mark - Array32 Implementation -

namespace {
struct TypeHelperRegT {
	static reg_t toStorage(const reg_t value) {
		return value;
	}

	static reg_t fromStorage(const reg_t value) {
		return value;
	}

	static const bool isRaw = false;
};

template<typename T, Array32::Type type>
struct TypeHelperFundamental {
	static T toStorage(const reg_t value) {
		if (value.getSegment() != 0) {
			error("TypeHelper<type: %u>::toStorage(%04x:%04x): Invalid value with segment non-zero", type, PRINT_REG(value));
		}

		return value.toUint16();
	}

	static reg_t fromStorage(const T value) {
		return make_reg(0, value);
	}

	static const bool isRaw = true;
};
} // End of anonymous namespace

template<class T, Array32::Type type, class TypeHelper>
class Array32Implementation : public Array32 {
private:
	typedef Array32Implementation<T, type, TypeHelper> ThisType;

public:
	Array32Implementation() : Array32(type) {}

	virtual void setSize(uint size) override {
		_storage.resize(size);
	}

	virtual void assureSize(uint size) override {
		// Please be aware that we can not use _storage.reserve here because
		// we actually need the elements to be existing.
		if (size > _storage.size()) {
			_storage.resize(size);
		}
	}

	virtual uint getSize() const override {
		return _storage.size();
	}

	virtual void setElement(uint index, reg_t value) override {
		if (index >= _storage.size()) {
			error("Array32Implementation<type: %u>::setElement: Index out of bounds (%u >= %u)", type, index, _storage.size());
		}

		_storage[index] = TypeHelper::toStorage(value);
	}

	virtual reg_t getElement(uint index) const override {
		if (index >= _storage.size()) {
			error("Array32Implementation<type: %u>::getElement: Index out of bounds (%u >= %u)", type, index, _storage.size());
		}

		return TypeHelper::fromStorage(_storage[index]);
	}

	virtual void setElements(uint index, uint count, reg_t *values) override {
		if (index + count > _storage.size()) {
			error("Array32Implementation<type: %u>::setElements: Access out of bounds (%u + %u > %u)", type, index, count, _storage.size());
		}

		for (uint i = 0; i < count; ++i) {
			_storage[index + i] = TypeHelper::toStorage(*values++);
		}
	}

	virtual void copyElements(const Array32 *source, uint srcIndex, uint dstIndex, uint count) override {
		assert(source);
		if (source->getType() != type) {
			error("Array32Implementation<type: %u>::copyElements: Incompatible array types (destination: %u vs source: %u)", type, type, source->getType());
		}

		if (srcIndex + count > source->getSize()) {
			error("Array32Implementation<type: %u>::copyElements: Source access out of bounds (%u + %u > %u)", type, srcIndex, count, source->getSize());
		}

		if (dstIndex + count > _storage.size()) {
			error("Array32Implementation<type: %u>::copyElements: Destination access out of bounds (%u + %u > %u)", type, dstIndex, count, _storage.size());
		}

		typename StorageContainer::const_iterator src = ((const ThisType *)source)->_storage.begin() + srcIndex;
		typename StorageContainer::iterator dst = _storage.begin() + dstIndex;
		Common::copy(src, src + count, dst);
	}

	virtual void fill(uint index, uint count, reg_t value) override {
		if (index + count > _storage.size()) {
			error("Array32Implementation<type: %u>::copyElements: Access out of bounds (%u + %u > %u)", type, index, count, _storage.size());
		}

		typename StorageContainer::iterator dst = _storage.begin() + index;
		Common::fill(dst, dst + count, TypeHelper::toStorage(value));
	}

	virtual void *getStoragePointer() override {
		return _storage.begin();
	}

	virtual const void *getStoragePointer() const override {
		return _storage.begin();
	}

	virtual uint getStorageSize() const override {
		return _storage.size() * sizeof(T);
	}

	virtual bool isRawData() const override {
		return TypeHelper::isRaw;
	}

	virtual void saveLoadWithSerializer(Common::Serializer &s) override {
		uint32 size = _storage.size();
		s.syncAsUint32LE(size);

		if (s.isLoading()) {
			_storage.resize(size);
		}

		for (typename StorageContainer::iterator i = _storage.begin(), end = _storage.end();
		     i != end; ++i) {
			syncWithSerializer(s, *i);
		}
	}

private:
	typedef Common::Array<T> StorageContainer;
	StorageContainer _storage;
};

Array32::~Array32() {
}

Array32 *Array32::makeArray(uint type) {
	// NOTE: We use a slightly different storage scheme than the original.
	// The original used the same underlying storage format for kTypeInt and
	// kTypeID. Due to our handles/references working based on reg_t we need
	// to use reg_t as storage for kTypeID though.
	//
	// Original array memory layout is (for kTypeInt, kTypeID, kTypeByte):
	//  0x00:    uint16 with element size
	//  0x02:    uint16 with number of elements
	//  0x04...: actual elements (uint16 for kTypeInt and kTypeID,
	//                            byte   for kTypeByte)
	switch (type) {
	case kTypeInt:
		return new Array32Implementation<uint16, kTypeInt, TypeHelperFundamental<uint16, kTypeInt> >();

	case kTypeID:
		return new Array32Implementation<reg_t, kTypeID, TypeHelperRegT>();

	case kTypeByte:
		return new Array32Implementation<byte, kTypeByte, TypeHelperFundamental<byte, kTypeByte> >();

	case kTypeString:
		// TODO: As soon as SCI32's kString is properly implemented allow to
		// create strings at this point too.
		error("Array32::makeArray: kTypeString not implemented");
		return nullptr;

	default:
		error("Array32::makeArray: Unknown type %u", type);
	}
}

} // End of namespace Sci

#endif
