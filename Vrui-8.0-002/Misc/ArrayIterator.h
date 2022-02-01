/***********************************************************************
ArrayIterator - Class for iterators into standard C arrays of elements.
Copyright (c) 2021 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_ARRAYITERATOR_INCLUDED
#define MISC_ARRAYITERATOR_INCLUDED

#include <stddef.h>

namespace Misc {

template <class ElementParam>
class ArrayIterator
	{
	/* Embedded classes: */
	public:
	typedef ElementParam Element; // Type of elements being iterated over
	
	/* Elements: */
	private:
	Element* element; // Pointer to the current element
	
	/* Constructors and destructors: */
	public:
	ArrayIterator(void) // Creates an invalid iterator
		:element(0)
		{
		}
	ArrayIterator(Element* sElement) // Creates an iterator to the given array element
		:element(sElement)
		{
		}
	
	/* Methods: */
	friend bool operator==(const ArrayIterator& it1,const ArrayIterator& it2) // Equality operator
		{
		return it1.element==it2.element;
		}
	friend bool operator!=(const ArrayIterator& it1,const ArrayIterator& it2) // Inequality operator
		{
		return it1.element!=it2.element;
		}
	
	/* Indirection methods: */
	Element& operator*(void) const // Returns the current element
		{
		return *element;
		}
	Element* operator->(void) const // Returns the current element for structure element selection
		{
		return element;
		}
	
	/* Iteration methods: */
	ArrayIterator& operator++(void) // Pre-increment
		{
		++element;
		return *this;
		}
	ArrayIterator operator++(int) // Post-increment
		{
		Element* result=element;
		++element;
		return ArrayIterator(result);
		}
	ArrayIterator& operator--(void) // Pre-decrement
		{
		--element;
		return *this;
		}
	ArrayIterator operator--(int) // Post-decrement
		{
		Element* result=element;
		--element;
		return ArrayIterator(result);
		}
	ArrayIterator& operator+=(ptrdiff_t offset) // Addition assignment operator
		{
		element+=offset;
		return *this;
		}
	ArrayIterator operator+(ptrdiff_t offset) const // Addition operator
		{
		return ArrayIterator(element+offset);
		}
	ArrayIterator& operator-=(ptrdiff_t offset) // Subtraction assignment operator
		{
		element-=offset;
		return *this;
		}
	ArrayIterator operator-(ptrdiff_t offset) const // Subtraction operator
		{
		return ArrayIterator(element-offset);
		}
	friend ptrdiff_t operator-(const ArrayIterator& it1,const ArrayIterator& it2) // Difference operator; assumes both iterators iterate over the same array
		{
		return it1.element-it2.element;
		}
	};

}

#endif
