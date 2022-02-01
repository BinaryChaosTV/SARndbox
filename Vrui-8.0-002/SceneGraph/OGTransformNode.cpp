/***********************************************************************
OGTransformNode - Class for group nodes that apply an orthogonal
transformation to their children, with a simplified field interface for
direct control through application software.
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

#include <SceneGraph/OGTransformNode.h>

#include <string.h>
#include <AL/ALContextData.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>
#include <SceneGraph/ALRenderState.h>

namespace SceneGraph {

/****************************************
Static elements of class OGTransformNode:
****************************************/

const char* OGTransformNode::className="OGTransform";

/********************************
Methods of class OGTransformNode:
********************************/

OGTransformNode::OGTransformNode(void)
	:transform(OGTransform::identity)
	{
	}

const char* OGTransformNode::getClassName(void) const
	{
	return className;
	}

Box OGTransformNode::calcBoundingBox(void) const
	{
	/* Return the explicit bounding box if there is one: */
	if(explicitBoundingBox)
		return *explicitBoundingBox;
	else
		{
		/* Calculate the group's bounding box as the union of the transformed children's boxes: */
		Box result=Box::empty;
		for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			{
			Box childBox=(*chIt)->calcBoundingBox();
			childBox.transform(transform.getValue());
			result.addBox(childBox);
			}
		return result;
		}
	}

void OGTransformNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Transform the collision query: */
	SphereCollisionQuery transformedCollisionQuery=collisionQuery.transform(transform.getValue());
	
	/* Delegate to the base class: */
	GroupNode::testCollision(transformedCollisionQuery);
	
	/* Update the original collision query if the transformed copy was updated: */
	if(transformedCollisionQuery.getHitLambda()<collisionQuery.getHitLambda())
		collisionQuery.updateFromTransform(transform.getValue(),transformedCollisionQuery);
	}

void OGTransformNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	DOGTransform previousTransform=renderState.pushTransform(transform.getValue());
	
	/* Delegate to the base class: */
	GroupNode::glRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

void OGTransformNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	DOGTransform previousTransform=renderState.pushTransform(transform.getValue());
	
	/* Delegate to the base class: */
	GroupNode::alRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

}
