/***********************************************************************
DOGTransformNode - Class for group nodes that apply a double-precision
orthogonal transformation to their children, with a simplified field
interface for direct control through application software.
Copyright (c) 2020-2021 Oliver Kreylos

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

#ifndef SCENEGRAPH_DOGTRANSFORMNODE_INCLUDED
#define SCENEGRAPH_DOGTRANSFORMNODE_INCLUDED

#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GroupNode.h>

namespace SceneGraph {

class DOGTransformNode:public GroupNode
	{
	/* Embedded classes: */
	public:
	typedef SF<DOGTransform> SFDOGTransform;
	
	/* Elements: */
	static const char* className; // The class's name
	
	/* Fields: */
	SFDOGTransform transform;
	
	/* Derived state: */
	protected:
	
	/* Constructors and destructors: */
	public:
	DOGTransformNode(void); // Creates an empty transform node with an identity transformation
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	
	/* Methods from class GraphNode: */
	virtual Box calcBoundingBox(void) const;
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	virtual void alRenderAction(ALRenderState& renderState) const;
	
	/* New methods: */
	void setTransform(const DOGTransform& newTransform) // Sets the transformation and performs necessary updates
		{
		transform.setValue(newTransform);
		}
	};

typedef Misc::Autopointer<DOGTransformNode> DOGTransformNodePointer;

}

#endif
