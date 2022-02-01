/***********************************************************************
TimeStamp - Class for cyclical time stamps with microsecond resolution
and >4000s cycle time.
Copyright (c) 2020 Oliver Kreylos

This file is part of the Realtime Processing Library (Realtime).

The Realtime Processing Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Realtime Processing Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Realtime Processing Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef REALTIME_TIMESTAMP_INCLUDED
#define REALTIME_TIMESTAMP_INCLUDED

#include <time.h>
#include <sys/time.h>
#include <Misc/SizedTypes.h>
#include <Realtime/Config.h>

namespace Realtime {

class TimeStamp
	{
	/* Embedded classes: */
	public:
	typedef Misc::SInt32 TSType; // Type to represent time stamps
	
	/* Elements: */
	private:
	TSType timeStamp; // Absolute or relative time in microseconds
	
	/* Constructors and destructors: */
	public:
	TimeStamp(void) // Dummy constructor
		{
		}
	TimeStamp(TSType sTimeStamp) // Elementwise constructor
		:timeStamp(sTimeStamp)
		{
		}
	TimeStamp(const timeval& timeVal) // Creates time stamp from low-resolution time structure
		:timeStamp(TSType(long(timeVal.tv_sec)*1000000L+long(timeVal.tv_usec)))
		{
		}
	TimeStamp(const timespec& timeSpec) // Creates time stamp from high-resolution time structure
		:timeStamp(TSType(long(timeSpec.tv_sec)*1000000L+(long(timeSpec.tv_nsec)+500L)/1000L))
		{
		}
	static TimeStamp now(void) // Returns the current time as a time stamp
		{
		#if REALTIME_CONFIG_HAVE_POSIX_CLOCKS
		
		/* Query the monotonic high-resolution timer: */
		timespec now;
		clock_gettime(CLOCK_MONOTONIC,&now);
		return TimeStamp(TSType(long(now.tv_sec)*1000000L+(long(now.tv_nsec)+500L)/1000L));
		
		#else
		
		/* Query the system clock: */
		timeval now;
		gettimeofday(&now,0);
		return TimeStamp(TSType(long(now.tv_sec)*1000000L+long(now.tv_usec)));
		
		#endif
		}
	
	/* Methods: */
	TSType getTs(void) const // Returns a raw time stamp
		{
		return timeStamp;
		}
	operator double(void) const // Returns relative time in seconds
		{
		return double(timeStamp)/1.0e6;
		}
	bool before(const TimeStamp& other) const // Returns true if this absolute time stamp is before the other absolute time stamp within the cycle period
		{
		return TSType(timeStamp-other.timeStamp)<TSType(0); // The subtraction will take care of wrap-around
		}
	bool notAfter(const TimeStamp& other) const // Returns true if this absolute time stamp is not after the other absolute time stamp within the cycle period
		{
		return TSType(timeStamp-other.timeStamp)<=TSType(0); // The subtraction will take care of wrap-around
		}
	bool notBefore(const TimeStamp& other) const // Returns true if this absolute time stamp is not before the other absolute time stamp within the cycle period
		{
		return TSType(timeStamp-other.timeStamp)>=TSType(0); // The subtraction will take care of wrap-around
		}
	bool after(const TimeStamp& other) const // Returns true if this absolute time stamp is after the other absolute time stamp within the cycle period
		{
		return TSType(timeStamp-other.timeStamp)>TSType(0); // The subtraction will take care of wrap-around
		}
	TimeStamp& operator+=(const TimeStamp& offset) // Offsets an absolute or relative time by the given relative time
		{
		timeStamp+=offset.timeStamp;
		return *this;
		}
	TimeStamp operator+(const TimeStamp& offset) const // Ditto
		{
		return TimeStamp(timeStamp+offset.timeStamp);
		}
	TimeStamp& operator-=(const TimeStamp& other) // Calculates the offset between two absolute times or difference between two relative times
		{
		timeStamp-=other.timeStamp;
		return *this;
		}
	TimeStamp operator-(const TimeStamp& other) const // Ditto
		{
		return TimeStamp(timeStamp-other.timeStamp);
		}
	TimeStamp& operator*=(int factor) // Multiplies a relative time by the given integer
		{
		timeStamp*=factor;
		return *this;
		}
	TimeStamp operator*(int factor) const // Ditto
		{
		return TimeStamp(timeStamp*factor);
		}
	TimeStamp& operator/=(int divisor) // Divides a relative time by the given integer and rounds
		{
		timeStamp=(timeStamp+divisor/2)/divisor;
		return *this;
		}
	TimeStamp operator/(int divisor) const // Ditto
		{
		return TimeStamp((timeStamp+divisor/2)/divisor);
		}
	};

}

#endif
