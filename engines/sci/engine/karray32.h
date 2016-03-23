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

#ifndef SCI_ENGINE_KARRAY32_H
#define SCI_ENGINE_KARRAY32_H

#include "common/scummsys.h"

#ifdef ENABLE_SCI32

#include "sci/engine/vm_types.h"

#include "common/array.h"
#include "common/str.h"
#include "common/serializer.h"

namespace Sci {

/**
 * Interface for the array storage for kArray.
 *
 * This abstracts away from implementation details like the concrete storage.
 */
class Array32 : public Common::Serializable {
public:
	/**
	 * Internal array type identifier.
	 *
	 * The individual types are the type identifiers used by SSCI.
	 */
	enum Type {
		/**
		 * Array used to store uint16 values.
		 */
		kTypeInt    = 0,

		/**
		 * Array used to store handles/reg_t values.
		 */
		kTypeID     = 1,

		/**
		 * Array used to store bytes.
		 */
		kTypeByte   = 2,

		/**
		 * Array used to store strings.
		 *
		 * This array type is primarily used by kString and does work only on
		 * a few selected kArray sub ops.
		 */
		kTypeString = 3
	};

	virtual ~Array32();

	/**
	 * @see Array32::Type
	 * @return The type of the array.
	 */
	Type getType() const { return _type; }

	/**
	 * @param size  The new target size of the array.
	 */
	virtual void setSize(uint size) = 0;

	/**
	 * Assure that the array can store the requested number of elements.
	 *
	 * @param size  Minimum size the array needs to contain.
	 */
	virtual void assureSize(uint size) = 0;

	/**
	 * @return The current size of the array.
	 */
	virtual uint getSize() const = 0;

	/**
	 * Set value for an element.
	 *
	 * @param index  Index of the element to set.
	 * @param value  The value to set.
	 */
	virtual void setElement(uint index, reg_t value) = 0;

	/**
	 * Retrieve the value for an element.
	 *
	 * @param index  Index of the element to retreive.
	 * @return Value of the element inside a reg_t.
	 */
	virtual reg_t getElement(uint index) const = 0;

	/**
	 * Set value for a number of consecutive entries.
	 *
	 * This does not take care of assuring the array fits all the elements.
	 *
	 * @param index   Index of the first element to set.
	 * @param count   Number of elements to set.
	 * @param values  An array of values to set.
	 */
	virtual void setElements(uint index, uint count, reg_t *values) = 0;

	/**
	 * Copy elements from another array.
	 *
	 * This requires both arrays to be of the same type.
	 * This does not take care of assuring the array fits all the elements.
	 *
	 * @param source    The array to copy elements from.
	 * @param srcIndex  The first element to copy from.
	 * @param dstIndex  The first element to use as storage.
	 * @param count     The number of elements to copy.
	 */
	virtual void copyElements(const Array32 *source, uint srcIndex, uint dstIndex, uint count) = 0;

	/**
	 * Fill a part of the array with a single value.
	 *
	 * This does not take care of assuring the array fits all the elements.
	 *
	 * @param index  Index of the first element to set.
	 * @param count  Number of elements to set.
	 * @param value  The value to set.
	 */
	virtual void fill(uint index, uint count, reg_t value) = 0;

	/**
	 * @return Raw pointer into array storage.
	 */
	virtual void *getStoragePointer() = 0;

	/**
	 * @return Raw pointer into array storage.
	 */
	virtual const void *getStoragePointer() const = 0;

	/**
	 * @return Storage size in bytes.
	 */
	virtual uint getStorageSize() const = 0;

	/**
	 * @return Whether the storage is raw data or reg_t based.
	 */
	virtual bool isRawData() const = 0;

	/**
	 * Serialize the contents of the array.
	 *
	 * Please be aware that this does not store any array type information, so
	 * one has to be careful that during loading the same array type is used.
	 *
	 * @param s  Serializer to use.
	 */
	virtual void saveLoadWithSerializer(Common::Serializer &s) override = 0;

	/**
	 * Instantiate an array.
	 *
	 * @see Array32::Type
	 * @param type  The type of the array.
	 * @return A pointer to the created array instance.
	 */
	static Array32 *makeArray(uint type);

protected:
	Array32(Type type) : _type(type) {}

private:
	const Type _type;
};

} // End of namespace Sci

#endif // ENABLE_SCI32

#endif
