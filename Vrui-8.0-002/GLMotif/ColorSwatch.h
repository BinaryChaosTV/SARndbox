/***********************************************************************
ColorSwatch - Class for widgets displaying an RGB color without
lighting, for color selection.
Copyright (c) 2021 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLMOTIF_COLORSWATCH_INCLUDED
#define GLMOTIF_COLORSWATCH_INCLUDED

#include <GLMotif/Widget.h>

namespace GLMotif {

class ColorSwatch:public Widget
	{
	/* Elements: */
	protected:
	Vector preferredSize; // The widget's preferred size
	
	/* Constructors and destructors: */
	public:
	ColorSwatch(const char* sName,Container* sParent,bool manageChild =true);
	
	/* Methods inherited from Widget: */
	virtual Vector calcNaturalSize(void) const;
	virtual void draw(GLContextData& contextData) const;
	
	/* New methods: */
	void setPreferredSize(const Vector& newPreferredSize); // Sets a new preferred size
	};

}

#endif
