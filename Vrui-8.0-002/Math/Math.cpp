/***********************************************************************
Math - Genericized versions of standard C math functions.
Copyright (c) 2001-2021 Oliver Kreylos

This file is part of the Templatized Math Library (Math).

The Templatized Math Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Math Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Math Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Math/Math.h>

namespace Math {

namespace {

/**************************************
Generic implementation of gcd function:
**************************************/

/* Implementation for signed integers: */
template <class IntegerParam>
inline IntegerParam gcd_signed_impl(IntegerParam a,IntegerParam b)
	{
	/* Make both numbers positive: */
	a=IntegerParam(Math::abs(a));
	b=IntegerParam(Math::abs(b));
	
	/* Ensure that a is the bigger number: */
	if(a<=b)
		{
		IntegerParam t=a;
		a=b;
		b=t;
		}
	
	/* Run the algorithm: */
	while(b!=IntegerParam(0))
		{
		IntegerParam ap=b;
		b=a%b;
		a=ap;
		}
	
	return a;
	}

/* Implementation for unsigned integers: */
template <class IntegerParam>
inline IntegerParam gcd_unsigned_impl(IntegerParam a,IntegerParam b)
	{
	/* Ensure that a is the bigger number: */
	if(a<=b)
		{
		IntegerParam t=a;
		a=b;
		b=t;
		}
	
	/* Run the algorithm: */
	while(b!=IntegerParam(0))
		{
		IntegerParam ap=b;
		b=a%b;
		a=ap;
		}
	
	return a;
	}

/**************************************
Generic implementation of lcm function:
**************************************/

/* Implementation for signed integers: */
template <class IntegerParam>
inline IntegerParam lcm_signed_impl(IntegerParam a,IntegerParam b)
	{
	/* Make both numbers positive: */
	a=IntegerParam(Math::abs(a));
	b=IntegerParam(Math::abs(b));
	
	/* First calculate the greatest common divisor: */
	IntegerParam gcd=gcd_unsigned_impl(a,b);
	
	/* Multiply the numbers and factor out the gcd: */
	return (a/gcd)*b;
	}

/* Implementation for signed integers: */
template <class IntegerParam>
inline IntegerParam lcm_unsigned_impl(IntegerParam a,IntegerParam b)
	{
	/* First calculate the greatest common divisor: */
	IntegerParam gcd=gcd_unsigned_impl(a,b);
	
	/* Multiply the numbers and factor out the gcd: */
	return (a/gcd)*b;
	}

}

/**************************************************************
Implementations of gcd and lcm functions for all integer types:
**************************************************************/

template <>
signed char gcd<signed char>(signed char a,signed char b)
	{
	return gcd_signed_impl(a,b);
	}

template <>
signed short gcd<signed short>(signed short a,signed short b)
	{
	return gcd_signed_impl(a,b);
	}

template <>
signed int gcd<signed int>(signed int a,signed int b)
	{
	return gcd_signed_impl(a,b);
	}

template <>
signed long gcd<signed long>(signed long a,signed long b)
	{
	return gcd_signed_impl(a,b);
	}

template <>
unsigned char gcd<unsigned char>(unsigned char a,unsigned char b)
	{
	return gcd_unsigned_impl(a,b);
	}

template <>
unsigned short gcd<unsigned short>(unsigned short a,unsigned short b)
	{
	return gcd_unsigned_impl(a,b);
	}

template <>
unsigned int gcd<unsigned int>(unsigned int a,unsigned int b)
	{
	return gcd_unsigned_impl(a,b);
	}

template <>
unsigned long gcd<unsigned long>(unsigned long a,unsigned long b)
	{
	return gcd_unsigned_impl(a,b);
	}

template <>
signed char lcm<signed char>(signed char a,signed char b)
	{
	return lcm_signed_impl(a,b);
	}

template <>
signed short lcm<signed short>(signed short a,signed short b)
	{
	return lcm_signed_impl(a,b);
	}

template <>
signed int lcm<signed int>(signed int a,signed int b)
	{
	return lcm_signed_impl(a,b);
	}

template <>
signed long lcm<signed long>(signed long a,signed long b)
	{
	return lcm_signed_impl(a,b);
	}

template <>
unsigned char lcm<unsigned char>(unsigned char a,unsigned char b)
	{
	return lcm_unsigned_impl(a,b);
	}

template <>
unsigned short lcm<unsigned short>(unsigned short a,unsigned short b)
	{
	return lcm_unsigned_impl(a,b);
	}

template <>
unsigned int lcm<unsigned int>(unsigned int a,unsigned int b)
	{
	return lcm_unsigned_impl(a,b);
	}

template <>
unsigned long lcm<unsigned long>(unsigned long a,unsigned long b)
	{
	return lcm_unsigned_impl(a,b);
	}

}
