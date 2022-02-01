/***********************************************************************
SimpleSet - Class representing unordered lists of values using dynamic
arrays, with removal implemented by moving the last element in the list
to the place of the removed element. Not really a set because inserting
elements multiple times is not prohibited.
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

#ifndef MISC_SIMPLESET_INCLUDED
#define MISC_SIMPLESET_INCLUDED

#include <stddef.h>
#include <stdlib.h>
#include <Misc/ArrayIterator.h>
#include <Misc/ArrayConstIterator.h>

namespace Misc {

template <class ElementParam>
class SimpleSet
	{
	/* Embedded classes: */
	public:
	typedef ElementParam Element; // Type of elements stored in the set
	typedef ArrayIterator<ElementParam> iterator; // Type for non-const iterators
	typedef ArrayConstIterator<ElementParam> const_iterator; // Type for const iterators
	
	/* Elements: */
	private:
	Element* beginPtr; // Pointer to allocated array and first set element
	Element* endPtr; // Pointer one behind last set element
	Element* allocEnd; // Pointer one behind allocated array
	
	/* Private methods: */
	static void destroyElements(Element* begin,Element* end) // Destroys the given array of elements
		{
		/* Call the Element class's destructor on all array elements: */
		for(Element* ePtr=begin;ePtr!=end;++ePtr)
			ePtr->~Element();
		}
	static Element* copyElements(Element* dest,const Element* begin,const Element* end) // Copies the given array of elements
		{
		/* Call the Element class's copy constructor via placement new on all array elements: */
		for(const Element* ePtr=begin;ePtr!=end;++ePtr,++dest)
			new (dest) Element(*ePtr);
		return dest;
		}
	static Element* moveElements(Element* dest,Element* begin,Element* end) // Moves the given array of elements
		{
		/* Call the Element class's copy constructor via placement new on all destination array elements, and then call the Element class's destructor on all source array elements: */
		for(Element* ePtr=begin;ePtr!=end;++ePtr,++dest)
			{
			new (dest) Element(*ePtr);
			ePtr->~Element();
			}
		return dest;
		}
	void grow(size_t newAllocSize) // Increases allocated space to the given number of elements
		{
		/* Save the current allocated space: */
		Element* oldBeginPtr=beginPtr;
		
		/* Re-allocate the space: */
		beginPtr=static_cast<Element*>(malloc(newAllocSize*sizeof(Element)));
		allocEnd=beginPtr+newAllocSize;
		
		/* Move the set elements into the new allocated space: */
		endPtr=moveElements(beginPtr,oldBeginPtr,endPtr);
		
		/* Release the previous allocated space: */
		free(oldBeginPtr);
		}
	
	
	/* Constructors and destructors: */
	public:
	SimpleSet(void) // Creates a set with no elements and no allocated space
		:beginPtr(0),endPtr(0),allocEnd(0)
		{
		}
	SimpleSet(const SimpleSet& source) // Copies the given set
		:beginPtr(0),endPtr(0),allocEnd(0)
		{
		/* Allocate the same number of elements as the source set unless the source set is empty: */
		size_t allocSize=source.allocEnd-source.beginPtr;
		if(allocSize!=0)
			{
			beginPtr=static_cast<Element*>(malloc(allocSize*sizeof(Element)));
			allocEnd=beginPtr+allocSize;
			
			/* Copy the source set's elements: */
			endPtr=copyElements(beginPtr,source.beginPtr,source.endPtr);
			}
		}
	SimpleSet& operator=(const SimpleSet& source) // Assignment operator
		{
		/* Check for self-assignment: */
		if(&source!=this)
			{
			/* Check if the allocated space is large enough to hold the source set's elements: */
			size_t allocSize=allocEnd-beginPtr;
			size_t sourceSize=source.endPtr-source.beginPtr;
			if(allocSize>=sourceSize)
				{
				/* Assign elements of the source set to existing elements of this set: */
				Element* sPtr=source.beginPtr;
				size_t size=endPtr-beginPtr;
				Element* assignEndPtr=size<=sourceSize?endPtr:beginPtr+sourceSize;
				for(Element* ePtr=beginPtr;ePtr!=assignEndPtr;++ePtr,++sPtr)
					*ePtr=*sPtr;
				
				/* Copy remaining elements in the source set, or destroy remaining elements in this set: */
				if(size<sourceSize)
					{
					/* Copy remaining elements: */
					endPtr=assignEndPtr;
					for(;sPtr!=source.endPtr;++sPtr,++endPtr)
						new (endPtr) Element(*sPtr);
					}
				else
					{
					/* Destroy remaining elements: */
					for(Element* ePtr=assignEndPtr;ePtr!=endPtr;++ePtr)
						ePtr->~Element();
					endPtr=assignEndPtr;
					}
				}
			else
				{
				/* Destroy all current set elements: */
				destroyElements(beginPtr,endPtr);
				
				/* Re-allocate space: */
				free(beginPtr);
				beginPtr=static_cast<Element*>(malloc(sourceSize*sizeof(Element)));
				allocEnd=beginPtr+sourceSize;
				
				/* Copy the source set's elements: */
				endPtr=copyElements(beginPtr,source.beginPtr,source.endPtr);
				}
			}
		return *this;
		}
	~SimpleSet(void) // Destroys all elements in the set
		{
		/* Destroy all elements and delete the allocated array: */
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
		return *beginPtr;
		}
	Element& front(void) // Ditto
		{
		return *beginPtr;
		}
	const Element& back(void) const // Accesses the last set element; assumes array is not empty
		{
		return endPtr[-1];
		}
	Element& back(void) // Ditto
		{
		return endPtr[-1];
		}
	const Element& operator[](size_t index) const // Returns the set element of the given index; assumes index is in bounds
		{
		return beginPtr[index];
		}
	Element& operator[](size_t index) // Ditto
		{
		return beginPtr[index];
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
	void add(const Element& newElement) // Adds the given element to the set; does not check if element is already in the set
		{
		/* Grow allocated space if there is no space: */
		if(endPtr==allocEnd)
			grow(((allocEnd-beginPtr)*3)/2+2); // No particular reason for this formula
		
		/* Copy the given element: */
		new (endPtr) Element(newElement);
		++endPtr;
		}
	void remove(const Element& element) // Removes the first instance of the given element from the set; does nothing if element is not in the set
		{
		/* Find the first instance of the given element in the set: */
		for(Element* ePtr=beginPtr;ePtr!=endPtr;++ePtr)
			if(*ePtr==element)
				{
				/* Copy the last element into the found element's slot (Element's assignment operator will catch self-assignment) and destroy the last element: */
				--endPtr;
				*ePtr=*endPtr;
				endPtr->~Element();
				
				/* Bail out: */
				break;
				}
		}
	void remove(const iterator& it) // Removes the element to which the iterator points
		{
		/* Copy the last element into the iterator's slot (Element's assignment operator will catch self-assignment) and destroy the last element: */
		--endPtr;
		*it=*endPtr;
		endPtr->~Element();
		}
	};

}

#endif
