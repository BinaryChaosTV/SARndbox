/***********************************************************************
ColorPalette - Class for composite widgets to display and edit a palette
of colors.
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

#ifndef GLMOTIF_COLORPALETTE_INCLUDED
#define GLMOTIF_COLORPALETTE_INCLUDED

#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GL/gl.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/NewButton.h>
#include <GLMotif/HSVColorSelector.h>

/* Forward declarations: */
namespace GLMotif {
class ColorSwatch;
}

namespace GLMotif {

class ColorPalette:public RowColumn
	{
	/* Embedded classes: */
	public:
	class ValueChangedCallbackData:public Misc::CallbackData
		{
		/* Elements: */
		public:
		ColorPalette* colorPalette; // Pointer to the color palette widget causing the event
		Color newColor; // The new selected color
		
		/* Constructors and destructors: */
		ValueChangedCallbackData(ColorPalette* sColorPalette,const Color& sNewColor)
			:colorPalette(sColorPalette),newColor(sNewColor)
			{
			}
		};
	
	/* Elements: */
	private:
	HSVColorSelector* colorSelector; // The color selector
	RowColumn* swatchPanel; // The panel of color swatches
	Color activeSwatchColor; // Color used to highlight the current active color swatch
	Color color; // The currently displayed color
	Color* trackedColor; // Pointer to a color variable that tracks the widget's current value
	Misc::CallbackList valueChangedCallbacks; // List of callbacks to be called when the current color changes due to a user interaction
	ColorSwatch* activeSwatch; // The currently active color swatch
	
	/* Private methods: */
	private:
	void colorSelectorValueChangedCallback(HSVColorSelector::ValueChangedCallbackData* cbData);
	void colorSwatchSelectedCallback(NewButton::SelectCallbackData* cbData);
	
	/* Constructors and destructors: */
	public:
	ColorPalette(const char* sName,Container* sParent,bool sManageChild =true);
	
	/* Methods from class Widget: */
	virtual void updateVariables(void);
	
	/* New methods: */
	void setColor(const Color& newColor); // Sets the current color and updates the current color swatch
	const Color& getColor(void) const // Returns the current color
		{
		return color;
		}
	void track(Color& newTrackedColor); // Tracks the given color variable
	Misc::CallbackList& getValueChangedCallbacks(void) // Returns list of value changed callbacks
		{
		return valueChangedCallbacks;
		}
	};

}

#endif
