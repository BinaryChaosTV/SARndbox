/***********************************************************************
Config - Internal configuration header file for the Vrui Library.
Copyright (c) 2014-2020 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_INTERNAL_CONFIG_INCLUDED
#define VRUI_INTERNAL_CONFIG_INCLUDED

#define VRUI_INTERNAL_CONFIG_HAVE_XRANDR 1
#define VRUI_INTERNAL_CONFIG_HAVE_XINPUT2 1
#define VRUI_INTERNAL_CONFIG_HAVE_LIBDBUS 1

#define VRUI_INTERNAL_CONFIG_VRWINDOW_USE_SWAPGROUPS 0

#define VRUI_INTERNAL_CONFIG_INPUT_H_HAS_STRUCTS 1

#define VRUI_INTERNAL_CONFIG_LIBDIR_DEBUG "/usr/local/lib/x86_64-linux-gnu/Vrui-8.0/debug"
#define VRUI_INTERNAL_CONFIG_LIBDIR_RELEASE "/usr/local/lib/x86_64-linux-gnu/Vrui-8.0"
#define VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_DEBUG "/usr/local/bin/debug"
#define VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_RELEASE "/usr/local/bin"
#define VRUI_INTERNAL_CONFIG_PLUGINDIR_DEBUG "/usr/local/lib/x86_64-linux-gnu/Vrui-8.0/debug"
#define VRUI_INTERNAL_CONFIG_PLUGINDIR_RELEASE "/usr/local/lib/x86_64-linux-gnu/Vrui-8.0"

#ifdef DEBUG
	#define VRUI_INTERNAL_CONFIG_LIBDIR VRUI_INTERNAL_CONFIG_LIBDIR_DEBUG
	#define VRUI_INTERNAL_CONFIG_EXECUTABLEDIR VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_DEBUG
	#define VRUI_INTERNAL_CONFIG_PLUGINDIR VRUI_INTERNAL_CONFIG_PLUGINDIR_DEBUG
#else
	#define VRUI_INTERNAL_CONFIG_LIBDIR VRUI_INTERNAL_CONFIG_LIBDIR_RELEASE
	#define VRUI_INTERNAL_CONFIG_EXECUTABLEDIR VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_RELEASE
	#define VRUI_INTERNAL_CONFIG_PLUGINDIR VRUI_INTERNAL_CONFIG_PLUGINDIR_RELEASE
#endif

#define VRUI_INTERNAL_CONFIG_TOOLDIR_DEBUG "/usr/local/lib/x86_64-linux-gnu/Vrui-8.0/debug/VRTools"
#define VRUI_INTERNAL_CONFIG_TOOLDIR_RELEASE "/usr/local/lib/x86_64-linux-gnu/Vrui-8.0/VRTools"
#define VRUI_INTERNAL_CONFIG_VISLETDIR_DEBUG "/usr/local/lib/x86_64-linux-gnu/Vrui-8.0/debug/VRVislets"
#define VRUI_INTERNAL_CONFIG_VISLETDIR_RELEASE "/usr/local/lib/x86_64-linux-gnu/Vrui-8.0/VRVislets"
#ifdef DEBUG
	#define VRUI_INTERNAL_CONFIG_TOOLDIR VRUI_INTERNAL_CONFIG_TOOLDIR_DEBUG
	#define VRUI_INTERNAL_CONFIG_VISLETDIR VRUI_INTERNAL_CONFIG_VISLETDIR_DEBUG
#else
	#define VRUI_INTERNAL_CONFIG_TOOLDIR VRUI_INTERNAL_CONFIG_TOOLDIR_RELEASE
	#define VRUI_INTERNAL_CONFIG_VISLETDIR VRUI_INTERNAL_CONFIG_VISLETDIR_RELEASE
#endif

#define VRUI_INTERNAL_CONFIG_TOOLNAMETEMPLATE "lib%s.so"
#define VRUI_INTERNAL_CONFIG_VISLETNAMETEMPLATE "lib%s.so"

#define VRUI_INTERNAL_CONFIG_CONFIGFILENAME "Vrui"
#define VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX ".cfg"
#define VRUI_INTERNAL_CONFIG_SYSCONFIGDIR "/usr/local/etc/Vrui-8.0"
#define VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE 1
#define VRUI_INTERNAL_CONFIG_USERCONFIGDIR ".config/Vrui-8.0"
#define VRUI_INTERNAL_CONFIG_APPCONFIGDIR "Applications"
#define VRUI_INTERNAL_CONFIG_DEFAULTROOTSECTION "Desktop"

#define VRUI_INTERNAL_CONFIG_VERSION 8000002
#define VRUI_INTERNAL_CONFIG_ETCDIR "/usr/local/etc/Vrui-8.0"
#define VRUI_INTERNAL_CONFIG_SHAREDIR "/usr/local/share/Vrui-8.0"

#endif
