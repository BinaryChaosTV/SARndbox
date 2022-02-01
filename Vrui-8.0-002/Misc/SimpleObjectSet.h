/***********************************************************************
SimpleObjectSet - Class representing unordered lists of pointers to new-
allocated objects using dynamic arrays, with removal implemented by
moving the last element in the list to the place of the removed element.
Not really a set because inserting elements multiple times is not
prohibited.
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

#ifndef MISC_SIMPLEOBJECTSET_INCLUDED
#define MISC_SIMPLEOBJECTSET_INCLUDED

#include <stddef.h>
#include <stdlib.h>
#include <Misc/ArrayIterator.h>
#include <Misc/ArrayConstIterator.h>

namespace Misc {

template <class ElementParam>
class SimpleObjectSet
	{
	/* Embedded classes: */
	public:
	typedef ElementParam Element; // Type of elements referenced from the set
	typedef ElementParam* ElementPtr; // Type of pointers stored in the set
	
	class iterator // Class for iterators to non-const objects
		{
		/* Embedded classes: */
		public:
		typedef ElementParam Element; // Type of elements being iterated over
		typedef ElementParam* ElementPtr; // Type of pointers stored in the set
		
		/* Elements: */
		private:
		ElementPtr* element; // Pointer to the current element's pointer
		
		/* Constructors and destructors: */
		public:
		iterator(void) // Creates an invalid iterator
			:element(0)
			{
			}
		iterator(ElementPtr* sElement) // Creates an iterator to the given array element pointer
			:element(sElement)
			{
			}
		
		/* Methods: */
		friend bool operator==(const iterator& it1,const iterator& it2) // Equality operator
			{
			return it1.element==it2.element;
			}
		friend bool operator!=(const iterator& it1,const iterator& it2) // Inequality operator
			{
			return it1.element!=it2.element;
			}
		
		/* Indirection methods: */
		ElementPtr* getElement(void) const // Returns the current element's pointer
			{
			return element;
			}
		Element& operator*(void) const // Returns the current element
			{
			return **element;
			}
		Element* operator->(void) const // Returns the current element for structure element selection
			{
			return *element;
			}
		
		/* Iteration methods: */
		iterator& operator++(void) // Pre-increment
			{
			++element;
			return *this;
			}
		iterator operator++(int) // Post-increment
			{
			ElementPtr* result=element;
			++element;
			return iterator(result);
			}
		iterator& operator--(void) // Pre-decrement
			{
			--element;
			return *this;
			}
		iterator operator--(int) // Post-decrement
			{
			ElementPtr* result=element;
			--element;
			return iterator(result);
			}
		iterator& operator+=(ptrdiff_t offset) // Addition assignment operator
			{
			element+=offset;
			return *this;
			}
		iterator operator+(ptrdiff_t offset) const // Addition operator
			{
			return iterator(element+offset);
			}
		iterator& operator-=(ptrdiff_t offset) // Subtraction assignment operator
			{
			element-=offset;
			return *this;
			}
		iterator operator-(ptrdiff_t offset) const // Subtraction operator
			{
			return iterator(element-offset);
			}
		friend ptrdiff_t operator-(const iterator& it1,const iterator& it2) // Difference operator; assumes both iterators iterate over the same array
			{
			return it1.element-it2.element;
			}
		};
	
	class const_iterator // Class for iterators to const objects
		{
		/* Embedded classes: */
		public:
		typedef ElementParam Element; // Type of elements being iterated over
		typedef const ElementParam* ElementPtr; // Type of pointers stored in the set
		
		/* Elements: */
		private:
		const ElementPtr* element; // Pointer to the current element's pointer
		
		/* Constructors and destructors: */
		public:
		const_iterator(void) // Creates an invalid const iterator
			:element(0)
			{
			}
		const_iterator(const ElementPtr* sElement) // Creates a const iterator to the given array element pointer
			:element(sElement)
			{
			}
		const_iterator(const iterator& source) // Creates a const iterator from the given non-const iterator
			:element(source.getElement())
			{
			}
		
		/* Methods: */
		friend bool operator==(const const_iterator& it1,const const_iterator& it2) // Equality operator
			{
			return it1.element==it2.element;
			}
		friend bool operator==(const const_iterator& it1,const iterator& it2) // Equality operator
			{
			return it1.element==it2.getElement();
			}
		friend bool operator==(const iterator& it1,const const_iterator& it2) // Equality operator
			{
			return it1.getElement()==it2.element;
			}
		friend bool operator!=(const const_iterator& it1,const const_iterator& it2) // Inequality operator
			{
			return it1.element!=it2.element;
			}
		friend bool operator!=(const const_iterator& it1,const iterator& it2) // Inequality operator
			{
			return it1.element!=it2.getElement();
			}
		friend bool operator!=(const iterator& it1,const const_iterator& it2) // Inequality operator
			{
			return it1.getElement()!=it2.element;
			}
		
		/* Indirection methods: */
		const Element& operator*(void) const // Returns the current element
			{
			return **element;
			}
		const Element* operator->(void) const // Returns the current element for structure element selection
			{
			return *element;
			}
		
		/* Iteration methods: */
		const_iterator& operator++(void) // Pre-increment
			{
			++element;
			return *this;
			}
		const_iterator operator++(int) // Post-increment
			{
			const ElementPtr* result=element;
			++element;
			return const_iterator(result);
			}
		const_iterator& operator--(void) // Pre-decrement
			{
			--element;
			return *this;
			}
		const_iterator operator--(int) // Post-decrement
			{
			const ElementPtr* result=element;
			--element;
			return const_iterator(result);
			}
		const_iterator& operator+=(ptrdiff_t offset) // Addition assignment operator
			{
			element+=offset;
			return *this;
			}
		const_iterator operator+(ptrdiff_t offset) const // Addition operator
			{
			return const_iterator(element+offset);
			}
		const_iterator& operator-=(ptrdiff_t offset) // Subtraction assignment operator
			{
			element-=offset;
			return *this;
			}
		const_iterator operator-(ptrdiff_t offset) const // Subtraction operator
			{
			return const_iterator(element-offset);
			}
		friend ptrdiff_t operator-(const const_iterator& it1,const const_iterator& it2) // Difference operator; assumes both iterators iterate over the same array
			{
			return it1.element-it2.element;
			}
		friend ptrdiff_t operator-(const const_iterator& it1,const iterator& it2) // Difference operator; assumes both iterators iterate over the same array
			{
			return it1.element-it2.getElement();
			}
		friend ptrdiff_t operator-(const iterator& it1,const const_iterator& it2) // Difference operator; assumes both iterators iterate over the same array
			{
			return it1.getElement()-it2.element;
			}
		};
	
	/* Elements: */
	private:
	ElementPtr* beginPtr; // Pointer to allocated array and first set element pointer
	ElementPtr* endPtr; // Pointer one behind last set element pointer
	ElementPtr* allocEnd; // Pointer one behind allocated array
	
	/* Private methods: */
	static void destroyElements(ElementPtr* begin,ElementPtr* end) // Destroys the given array of element pointers
		{
		/* Delete all array elements: */
		for(ElementPtr* ePtr=begin;ePtr!=end;++ePtr)
			delete *ePtr;
		}
	void grow(size_t newAllocSize) // Increases allocated space to the given number of element pointers
		{
		/* Re-allocate the space: */
		size_t size=endPtr-beginPtr;
		beginPtr=static_cast<ElementPtr*>(realloc(beginPtr,newAllocSize*sizeof(ElementPtr)));
		endPtr=beginPtr+size;
		allocEnd=beginPtr+newAllocSize;
		}
	
	
	/* Constructors and destructors: */
	public:
	SimpleObjectSet(void) // Creates a set with no elements and no allocated space
		:beginPtr(0),endPtr(0),allocEnd(0)
		{
		}
	private:
	SimpleObjectSet(const SimpleObjectSet& source); // Prohibit copy constructor
	SimpleObjectSet& operator=(const SimpleObjectSet& source); // Prohibit assignment operator
	public:
	~SimpleObjectSet(void) // Destroys all elements in the set
		{
		/* Destroy all elements and delete the allocated pointer array: */
		destroyElements(beginPtr,endPtr);
		free(beginPtr);
		}
	
	/* Methods: */
	bool empty(void) const // Returns true if the set contains no elements
		{
		return beginPtr==endPtr;
		}
	size_t size(void) const // Returns the number of elements in the set
		{
		return endPtr-beginPtr;
		}
	
	/* Element access methods: */
	const Element& front(void) const // Accesses the first set element; assumes array is not empty
		{
		return **beginPtr;
		}
	Element& front(void) // Ditto
		{
		return **beginPtr;
		}
	const Element& back(void) const // Accesses the last set element; assumes array is not empty
		{
		return *endPtr[-1];
		}
	Element& back(void) // Ditto
		{
		return *endPtr[-1];
		}
	const Element& operator[](size_t index) const // Returns the set element of the given index; assumes index is in bounds
		{
		return *beginPtr[index];
		}
	Element& operator[](size_t index) // Ditto
		{
		return *beginPtr[index];
		}
	
	/* Element iteration methods: */
	const_iterator begin(void) const // Returns an iterator to the first element
		{
		return const_iterator(beginPtr);
		}
	iterator begin(void) // Ditto
		{
		return iterator(beginPtr);
		}
	const_iterator end(void) const // Returns an iterator after the last element
		{
		return const_iterator(endPtr);
		}
	iterator end(void) // Ditto
		{
		return iterator(endPtr);
		}
	
	/* Set manipulation methods: */
	size_t capacity(void) const // Returns the number of elements that fit into currently allocated space
		{
		return allocEnd-beginPtr;
		}
	void reserve(size_t newAllocSize) // Increases allocated space to hold at least the given number of elements
		{
		/* Grow allocated space if it is too small: */
		size_t allocSize=allocEnd-beginPtr;
		if(allocSize<newAllocSize)
			grow(newAllocSize);
		}
	void clear(void) // Removes all elements from the set
		{
		/* Destroy all elements: */
		destroyElements(beginPtr,endPtr);
		endPtr=beginPtr;
		}
	void add(ElementPtr newElement) // Adds the given new-allocated element to the set and takes ownership
		{
		/* Grow allocated space if there is no space: */
		if(endPtr==allocEnd)
			grow(((allocEnd-beginPtr)*3)/2+2); // No particular reason for this formula
		
		/* Copy the given element pointer: */
		*endPtr=newElement;
		++endPtr;
		}
	ElementPtr remove(ElementPtr element) // Removes the given element from the set without destroying it; does nothing if element is not in the set; returns pointer to removed element or null
		{
		ElementPtr result=0;
		
		/* Find the given element in the set: */
		for(ElementPtr* ePtr=beginPtr;ePtr!=endPtr;++ePtr)
			if(*ePtr==element)
				{
				result=*ePtr;
				
				/* Copy the last element into the found element's slot and remove the last element: */
				--endPtr;
				*ePtr=*endPtr;
				
				/* Bail out: */
				break;
				}
		
		return result;
		}
	ElementPtr remove(const iterator& it) // Removes the element to which the iterator points without destroying it; returns pointer to removed element
		{
		ElementPtr result=*it.getElement();
		
		/* Copy the last element into the iterator's slot and remove the last element: */
		--endPtr;
		*it.getElement()=*endPtr;
		
		return result;
		}
	};

}

#endif
