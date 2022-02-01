/***********************************************************************
TraversalState - Base class encapsulating the traversal state of a scene
graph during any of the processing passes.
Copyright (c) 2021 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/TraversalState.h>

namespace SceneGraph {

/*******************************
Methods of class TraversalState:
*******************************/

TraversalState::TraversalState(void)
	:baseViewerPos(Point::origin),baseUpVector(0,1,0),
	 currentTransform(DOGTransform::identity)
	{
	}

void TraversalState::startTraversal(const DOGTransform& newCurrentTransform,const Point& newBaseViewerPos,const Vector& newBaseUpVector)
	{
	/* Store the viewer position and up vector in initial model coordinates: */
	baseViewerPos=newBaseViewerPos;
	baseUpVector=newBaseUpVector;
	
	/* Reset the current transformation: */
	currentTransform=newCurrentTransform;
	}

}
