/***********************************************************************
Config - Configuration header file for the OpenAL Support Library.
Copyright (c) 2010-2021 Oliver Kreylos

This file is part of the OpenAL Support Library (ALSupport).

The OpenAL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenAL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenAL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef ALCONFIG_INCLUDED
#define ALCONFIG_INCLUDED

#define ALSUPPORT_CONFIG_HAVE_OPENAL 1

#if ALSUPPORT_CONFIG_HAVE_OPENAL
#ifdef __APPLE__
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif
#endif

#if !ALSUPPORT_CONFIG_HAVE_OPENAL

/* Define basic OpenAL data types so that not everything has to be bracketed: */
typedef char ALboolean;
typedef char ALchar;
typedef signed char ALbyte;
typedef unsigned char ALubyte;
typedef short ALshort;
typedef unsigned short ALushort;
typedef int ALint;
typedef unsigned int ALuint;
typedef int ALsizei;
typedef int ALenum;
typedef float ALfloat;
typedef double ALdouble;
typedef void ALvoid;

/* Define the most common OpenAL constants and enumerants: */
#define AL_NONE                                  0
#define AL_FALSE                                 0
#define AL_TRUE                                  1

#endif

#endif
