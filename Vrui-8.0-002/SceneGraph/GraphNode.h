/***********************************************************************
GraphNode - Base class for nodes that can be parts of a scene graph.
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

#ifndef SCENEGRAPH_GRAPHNODE_INCLUDED
#define SCENEGRAPH_GRAPHNODE_INCLUDED

#include <Misc/SizedTypes.h>
#include <Misc/Autopointer.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/Node.h>

/* Forward declarations: */
namespace SceneGraph {
class SphereCollisionQuery;
class GLRenderState;
class ALRenderState;
}

namespace SceneGraph {

class GraphNode:public Node
	{
	/* Embedded classes: */
	public:
	typedef Misc::UInt32 PassMask; // Type to represent bit masks of processing passes
	
	enum Pass // Enumerated type for processing or rendering passes in which a graph node can participate
		{
		CollisionPass=0x1, // Node participates in collision detection
		GLRenderPass=0x2, // Node participates in opaque OpenGL rendering
		GLTransparentRenderPass=0x4, // Node participates in transparent OpenGL rendering
		ALRenderPass=0x8 // Node participates in OpenAL audio rendering
		};
	
	enum UpdateResult
		{
		CascadePassAdded=Node::NumUpdateResults, // A processing pass was added to this node's pass mask
		CascadePassRemoved, // A processing pass was removed from this node's pass mask
		CascadePassMaskChanged, // This node's pass mask was changed in some unspecified way
		
		NumUpdateResults
		};
	
	/* Elements: */
	protected:
	PassMask passMask; // Bit mask of processing or rendering passes in which this node participates
	
	/* Protected methods: */
	unsigned int setPassMask(PassMask newPassMask) // Helper function to set the pass mask and return an appropriate update result
		{
		unsigned int result=NoCascade;
		
		/* Check if anything changed: */
		if(passMask!=newPassMask)
			{
			/* Return the proper result: */
			if((passMask&newPassMask)==passMask)
				result=CascadePassAdded;
			else if((passMask&newPassMask)==newPassMask)
				result=CascadePassRemoved;
			else
				result=CascadePassMaskChanged;
			passMask=newPassMask;
			}
		
		return result;
		}
	
	/* Constructors and destructors: */
	public:
	GraphNode(void); // Creates a graph node participating in collision detection and opaque OpenGL rendering
	
	/* New methods: */
	public:
	PassMask getPassMask(void) const // Returns this node's pass mask
		{
		return passMask;
		}
	bool participatesInPass(PassMask queryPassMask) const // Returns true if this node participates in any of the passes contained in the given pass mask
		{
		return (passMask&queryPassMask)!=0x0U;
		}
	virtual Box calcBoundingBox(void) const; // Returns the bounding box of the node
	virtual unsigned int updatePassMask(void); // Updates the node's pass mask; returns a non-zero result if the update has cascading effects towards the root of the scene graph
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const; // Tests the node for collision with a moving sphere
	virtual void glRenderAction(GLRenderState& renderState) const; // Renders the node into the given OpenGL context
	virtual void alRenderAction(ALRenderState& renderState) const; // Renders the node into the given OpenAL context
	};

typedef Misc::Autopointer<GraphNode> GraphNodePointer;

}

#endif
