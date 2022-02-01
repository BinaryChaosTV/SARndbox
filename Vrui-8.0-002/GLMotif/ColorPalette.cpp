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

#include <GLMotif/ColorPalette.h>

#include <GLMotif/StyleSheet.h>
#include <GLMotif/ColorSwatch.h>

namespace GLMotif {

void ColorPalette::colorSelectorValueChangedCallback(HSVColorSelector::ValueChangedCallbackData* cbData)
	{
	/* Update the current color: */
	color=cbData->newColor;
	
	/* Update the active color swatch: */
	activeSwatch->setForegroundColor(color);
	
	if(trackedColor!=0)
		{
		/* Update the tracked color: */
		*trackedColor=color;
		}
	
	/* Call the value changed callbacks: */
	ValueChangedCallbackData myCbData(this,color);
	valueChangedCallbacks.call(&myCbData);
	}

void ColorPalette::colorSwatchSelectedCallback(NewButton::SelectCallbackData* cbData)
	{
	/* Deactivate the current active color swatch: */
	NewButton* colorSwatchButton=static_cast<NewButton*>(activeSwatch->getParent());
	colorSwatchButton->setBorderColor(swatchPanel->getBorderColor());
	
	/* Activate the new active color swatch: */
	cbData->button->setBorderColor(activeSwatchColor);
	activeSwatch=static_cast<ColorSwatch*>(cbData->button->getChild());
	
	/* Update the current color: */
	color=activeSwatch->getForegroundColor();
	
	/* Update the color selector: */
	colorSelector->setCurrentColor(color);
	
	if(trackedColor!=0)
		{
		/* Update the tracked color: */
		*trackedColor=color;
		}
	
	/* Call the value changed callbacks: */
	ValueChangedCallbackData myCbData(this,color);
	valueChangedCallbacks.call(&myCbData);
	}

ColorPalette::ColorPalette(const char* sName,Container* sParent,bool sManageChild)
	:RowColumn(sName,sParent,false),
	 activeSwatchColor(1.0f,1.0f,1.0f),
	 trackedColor(0),
	 activeSwatch(0)
	{
	/* Get the current style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Create the composite widget layout: */
	setOrientation(HORIZONTAL);
	setPacking(PACK_TIGHT);
	setNumMinorWidgets(1);
	
	/* Define the default color palette: */
	Color defaultColors[4*6]=
		{
		Color(0.0f,0.0f,0.0f),Color(0.2f,0.2f,0.2f),Color(0.4f,0.4f,0.4f),Color(0.6f,0.6f,0.6f),Color(0.8f,0.8f,0.8f),Color(1.0f,1.0f,1.0f),
		Color(0.5f,0.0f,0.0f),Color(0.5f,0.5f,0.0f),Color(0.0f,0.5f,0.0f),Color(0.0f,0.5f,0.5f),Color(0.0f,0.0f,0.5f),Color(0.5f,0.0f,0.5f),
		Color(1.0f,0.0f,0.0f),Color(1.0f,1.0f,0.0f),Color(0.0f,1.0f,0.0f),Color(0.0f,1.0f,1.0f),Color(0.0f,0.0f,1.0f),Color(1.0f,0.0f,1.0f),
		Color(1.0f,0.5f,0.5f),Color(1.0f,1.0f,0.5f),Color(0.5f,1.0f,0.5f),Color(0.5f,1.0f,1.0f),Color(0.5f,0.5f,1.0f),Color(1.0f,0.5f,1.0f)
		};
	
	/* Select the initial color: */
	int initialActiveSwatch=12;
	color=defaultColors[initialActiveSwatch];
	
	/* Create the HSV-space color selector: */
	colorSelector=new HSVColorSelector("ColorSelector",this);
	colorSelector->setPreferredSize(16.0f*ss->size);
	colorSelector->setIndicatorSize(0.75f*ss->size);
	colorSelector->setCurrentColor(defaultColors[initialActiveSwatch]);
	colorSelector->getValueChangedCallbacks().add(this,&ColorPalette::colorSelectorValueChangedCallback);
	
	/* Create the panel of color swatches: */
	swatchPanel=new GLMotif::RowColumn("SwatchPanel",this,false);
	swatchPanel->setOrientation(VERTICAL);
	swatchPanel->setPacking(PACK_GRID);
	swatchPanel->setNumMinorWidgets(6);
	
	for(int i=0;i<4*6;++i)
		{
		/* Create the color swatch frame as a button: */
		char colorSwatchName[14]="ColorSwatch00";
		colorSwatchName[11]='0'+i/10;
		colorSwatchName[12]='0'+i%10;
		NewButton* colorSwatch=new NewButton(colorSwatchName,swatchPanel,false);
		colorSwatch->setBorderWidth(ss->size*0.5f);
		
		/* Create the color swatch: */
		ColorSwatch* swatch=new ColorSwatch("Swatch",colorSwatch);
		swatch->setBorderWidth(ss->size*0.5f);
		swatch->setForegroundColor(defaultColors[i]);
		swatch->setPreferredSize(Vector(2.0f*ss->size,2.0f*ss->size,0.0f));
		colorSwatch->getSelectCallbacks().add(this,&ColorPalette::colorSwatchSelectedCallback);
		
		if(i==initialActiveSwatch)
			{
			activeSwatch=swatch;
			colorSwatch->setBorderColor(activeSwatchColor);
			}
		
		colorSwatch->manageChild();
		}
	
	swatchPanel->manageChild();
	
	if(sManageChild)
		manageChild();
	}

void ColorPalette::updateVariables(void)
	{
	if(trackedColor!=0)
		{
		/* Update the current color: */
		color=*trackedColor;
		
		/* Update the color selector and the active color swatch: */
		colorSelector->setCurrentColor(color);
		activeSwatch->setForegroundColor(color);
		}
	}

void ColorPalette::setColor(const Color& newColor)
	{
	/* Update the current color: */
	color=newColor;
	
	if(trackedColor!=0)
		{
		/* Update the tracked color: */
		*trackedColor=newColor;
		}
	
	/* Update the color selector and the active color swatch: */
	colorSelector->setCurrentColor(color);
	activeSwatch->setForegroundColor(color);
	}

void ColorPalette::track(Color& newTrackedColor)
	{
	/* Change the tracked color variable: */
	trackedColor=&newTrackedColor;
	
	/* Update the current color: */
	color=*trackedColor;
	
	/* Update the color selector and the active color swatch: */
	colorSelector->setCurrentColor(color);
	activeSwatch->setForegroundColor(color);
	}

}
