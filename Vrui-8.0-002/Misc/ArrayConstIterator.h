/***********************************************************************
ArrayArrayConstIterator - Class for iterators into standard C arrays of
constant elements.
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

#ifndef MISC_ARRAYCONSTITERATOR_INCLUDED
#define MISC_ARRAYCONSTITERATOR_INCLUDED

#include <stddef.h>
#include <Misc/ArrayIterator.h>

namespace Misc {

template <class ElementParam>
class ArrayConstIterator
	{
	/* Embedded classes: */
	public:
	typedef ElementParam Element; // Type of elements being iterated over
	typedef ArrayIterator<ElementParam> iterator; // Type of compatible non-const iterator
	
	/* Elements: */
	private:
	const Element* element; // Pointer to the current element
	
	/* Constructors and destructors: */
	public:
	ArrayConstIterator(void) // Creates an invalid iterator
		:element(0)
		{
		}
	ArrayConstIterator(const Element* sElement) // Creates an iterator to the given array element
		:element(sElement)
		{
		}
	ArrayConstIterator(const iterator sIt) // Creates an iterator from the given non-const iterator
		:element(&*sIt)
		{
		}
	
	/* Methods: */
	friend bool operator==(const ArrayConstIterator& it1,const ArrayConstIterator& it2) // Equality operator
		{
		return it1.element==it2.element;
		}
	friend bool operator==(const ArrayConstIterator& it1,const iterator& it2) // Equality operator with a non-const iterator
		{
		return it1.element==&*it2;
		}
	friend bool operator==(const iterator& it1,const ArrayConstIterator& it2) // Ditto
		{
		return &*it1==it2.element;
		}
	friend bool operator!=(const ArrayConstIterator& it1,const ArrayConstIterator& it2) // Inequality operator
		{
		return it1.element!=it2.element;
		}
	friend bool operator!=(const ArrayConstIterator& it1,const iterator& it2) // Inequality operator with a non-const iterator
		{
		return it1.element!=&*it2;
		}
	friend bool operator!=(const iterator& it1,const ArrayConstIterator& it2) // Ditto
		{
		return &*it1!=it2.element;
		}
	
	/* Indirection methods: */
	const Element& operator*(void) const // Returns the current element
		{
		return *element;
		}
	const Element* operator->(void) const // Returns the current element for structure element selection
		{
		return element;
		}
	
	/* Iteration methods: */
	ArrayConstIterator& operator++(void) // Pre-increment
		{
		++element;
		return *this;
		}
	ArrayConstIterator operator++(int) // Post-increment
		{
		Element* result=element;
		++element;
		return ArrayConstIterator(result);
		}
	ArrayConstIterator& operator--(void) // Pre-decrement
		{
		--element;
		return *this;
		}
	ArrayConstIterator operator--(int) // Post-decrement
		{
		Element* result=element;
		--element;
		return ArrayConstIterator(result);
		}
	ArrayConstIterator& operator+=(ptrdiff_t offset) // Addition assignment operator
		{
		element+=offset;
		return *this;
		}
	ArrayConstIterator operator+(ptrdiff_t offset) const // Addition operator
		{
		return ArrayConstIterator(element+offset);
		}
	ArrayConstIterator& operator-=(ptrdiff_t offset) // Subtraction assignment operator
		{
		element-=offset;
		return *this;
		}
	ArrayConstIterator operator-(ptrdiff_t offset) const // Subtraction operator
		{
		return ArrayConstIterator(element-offset);
		}
	friend ptrdiff_t operator-(const ArrayConstIterator& it1,const ArrayConstIterator& it2) // Difference operator; assumes both iterators iterate over the same array
		{
		return it1.element-it2.element;
		}
	friend ptrdiff_t operator-(const ArrayConstIterator& it1,const iterator& it2) // Difference operator with a non-const iterator; assumes both iterators iterate over the same array
		{
		return it1.element-&*it2;
		}
	friend ptrdiff_t operator-(const iterator& it1,const ArrayConstIterator& it2) // Difference operator with a non-const iterator; assumes both iterators iterate over the same array
		{
		return &*it1-it2.element;
		}
	};

}

#endif
