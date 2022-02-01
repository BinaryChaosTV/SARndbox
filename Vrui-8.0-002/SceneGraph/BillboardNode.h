/***********************************************************************
BillboardNode - Class for group nodes that transform their children to
always face the viewer.
Copyright (c) 2009-2021 Oliver Kreylos

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

#ifndef SCENEGRAPH_BILLBOARDNODE_INCLUDED
#define SCENEGRAPH_BILLBOARDNODE_INCLUDED

#include <Misc/Autopointer.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GroupNode.h>

/* Forward declarations: */
namespace SceneGraph {
class TraversalState;
}

namespace SceneGraph {

class BillboardNode:public GroupNode
	{
	/* Embedded classes: */
	private:
	typedef DOGTransform::Scalar DScalar;
	typedef DOGTransform::Vector DVector;
	typedef DOGTransform::Rotation DRotation;
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFVector axisOfRotation;
	
	/* Derived elements: */
	protected:
	DVector aor; // Axis of rotation in double precision
	DScalar aor2; // Squared length of rotation axis
	DVector orthoZAxis; // Billboard's Z axis orthonormalized with regard to the axis of rotation
	DVector rotationNormal; // Vector normal to the plane spanned by the rotation axis and the orthonormalized Z axis
	
	/* Protected methods: */
	DOGTransform calcBillboardTransform(TraversalState& traversalState) const; // Returns the billboard transformation based on the given scene graph traversal state
	
	/* Constructors and destructors: */
	public:
	BillboardNode(void); // Creates an empty billboard node
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual EventOut* getEventOut(const char* fieldName) const;
	virtual EventIn* getEventIn(const char* fieldName);
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual unsigned int update(void);
	
	/* Methods from class GraphNode: */
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	virtual void alRenderAction(ALRenderState& renderState) const;
	};

typedef Misc::Autopointer<BillboardNode> BillboardNodePointer;

}

#endif
