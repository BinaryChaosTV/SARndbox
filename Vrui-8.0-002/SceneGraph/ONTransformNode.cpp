/***********************************************************************
ONTransformNode - Class for group nodes that apply an orthonormal
transformation to their children, with a simplified field interface for
direct control through application software.
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

#include <SceneGraph/ONTransformNode.h>

#include <string.h>
#include <AL/ALContextData.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>
#include <SceneGraph/ALRenderState.h>

namespace SceneGraph {

/****************************************
Static elements of class ONTransformNode:
****************************************/

const char* ONTransformNode::className="ONTransform";

/********************************
Methods of class ONTransformNode:
********************************/

ONTransformNode::ONTransformNode(void)
	:transform(ONTransform::identity)
	{
	}

const char* ONTransformNode::getClassName(void) const
	{
	return className;
	}

Box ONTransformNode::calcBoundingBox(void) const
	{
	/* Return the explicit bounding box if there is one: */
	if(explicitBoundingBox!=0)
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

void ONTransformNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Transform the collision query: */
	OGTransform ogTransform(transform.getValue());
	SphereCollisionQuery transformedCollisionQuery=collisionQuery.transform(ogTransform);
	
	/* Delegate to the base class: */
	GroupNode::testCollision(transformedCollisionQuery);
	
	/* Update the original collision query if the transformed copy was updated: */
	if(transformedCollisionQuery.getHitLambda()<collisionQuery.getHitLambda())
		collisionQuery.updateFromTransform(ogTransform,transformedCollisionQuery);
	}

void ONTransformNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	DOGTransform previousTransform=renderState.pushTransform(transform.getValue());
	
	/* Delegate to the base class: */
	GroupNode::glRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

void ONTransformNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	DOGTransform previousTransform=renderState.pushTransform(transform.getValue());
	
	/* Delegate to the base class: */
	GroupNode::alRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

}
