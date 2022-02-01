/***********************************************************************
SwitchNode - Class for group nodes that traverse zero or one of their
children based on a selection field.
Copyright (c) 2018-2021 Oliver Kreylos

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

#ifndef SCENEGRAPH_SWITCHNODE_INCLUDED
#define SCENEGRAPH_SWITCHNODE_INCLUDED

#include <vector>
#include <Misc/Autopointer.h>
#include <Geometry/Point.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GraphNode.h>

namespace SceneGraph {

class SwitchNode:public GraphNode
	{
	/* Embedded classes: */
	public:
	typedef MF<GraphNodePointer> MFGraphNode;
	
	/* Elements: */
	static const char* className; // The class's name
	
	/* Fields: */
	MFGraphNode choice;
	SFInt whichChoice;
	
	/* Constructors and destructors: */
	SwitchNode(void); // Creates an empty switch node
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual EventOut* getEventOut(const char* fieldName) const;
	virtual EventIn* getEventIn(const char* fieldName);
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual unsigned int update(void);
	virtual unsigned int cascadingUpdate(Node& child,unsigned int childUpdateResult);
	
	/* Methods from class GraphNode: */
	virtual Box calcBoundingBox(void) const;
	virtual unsigned int updatePassMask(void);
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	virtual void alRenderAction(ALRenderState& renderState) const;
	};

typedef Misc::Autopointer<SwitchNode> SwitchNodePointer;

}

#endif
