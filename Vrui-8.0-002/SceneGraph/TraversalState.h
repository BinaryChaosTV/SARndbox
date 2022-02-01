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

#ifndef SCENEGRAPH_TRAVERSALSTATE_INCLUDED
#define SCENEGRAPH_TRAVERSALSTATE_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/Geometry.h>

namespace SceneGraph {

class TraversalState
	{
	/* Elements: */
	protected:
	Point baseViewerPos; // Viewer position in initial model coordinates
	Vector baseUpVector; // Up vector in initial model coordinates
	DOGTransform currentTransform; // Transformation from initial model coordinates to current model coordinates
	
	/* Constructors and destructors: */
	public:
	TraversalState(void); // Creates an uninitialized traversal state
	
	/* Methods: */
	
	/* Traversal scoping: */
	void startTraversal(const DOGTransform& newCurrentTransform,const Point& newBaseViewerPos,const Vector& newBaseUpVector); // Starts a new scene graph traversal from the given initial transformation and viewer position and up direction in initial model space
	
	/* Status queries: */
	Point getViewerPos(void) const // Returns the viewer position in current model coordinates
		{
		return Point(currentTransform.inverseTransform(baseViewerPos));
		}
	Vector getUpVector(void) const // Returns the up direction in current model coordinates
		{
		return Vector(currentTransform.inverseTransform(baseUpVector));
		}
	const DOGTransform& getTransform(void) const // Returns the current model transformation matrix
		{
		return currentTransform;
		}
	
	/* Transformation stack management: */
	DOGTransform pushTransform(const DOGTransform& deltaTransform) // Pushes the given double-precision transformation onto the matrix stack and returns the previous transformation
		{
		/* Save, return, and update the current transformation: */
		DOGTransform result=currentTransform;
		currentTransform*=deltaTransform;
		currentTransform.renormalize();
		return result;
		}
	DOGTransform pushTransform(const ONTransform& deltaTransform) // Ditto, with a single-precision orthonormal transformation
		{
		/* Save, return, and update the current transformation: */
		DOGTransform result=currentTransform;
		currentTransform*=deltaTransform;
		currentTransform.renormalize();
		return result;
		}
	DOGTransform pushTransform(const OGTransform& deltaTransform) // Ditto, with a single-precision orthogonal transformation
		{
		/* Save, return, and update the current transformation: */
		DOGTransform result=currentTransform;
		currentTransform*=deltaTransform;
		currentTransform.renormalize();
		return result;
		}
	void popTransform(const DOGTransform& previousTransform) // Resets the matrix stack to the given transformation; must be result from previous pushTransform call
		{
		/* Reinstate the previous transformation: */
		currentTransform=previousTransform;
		}
	};

}

#endif
