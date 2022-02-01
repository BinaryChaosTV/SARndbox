/***********************************************************************
TransformNode - Class for group nodes that apply an orthogonal
transformation to their children.
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

#include <SceneGraph/TransformNode.h>

#include <string.h>
#include <Math/Math.h>
#include <AL/ALContextData.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>
#include <SceneGraph/ALRenderState.h>

namespace SceneGraph {

/**************************************
Static elements of class TransformNode:
**************************************/

const char* TransformNode::className="Transform";

/******************************
Methods of class TransformNode:
******************************/

TransformNode::TransformNode(void)
	:center(Point::origin),
	 rotation(Rotation::identity),
	 scale(Size(1,1,1)),
	 scaleOrientation(Rotation::identity),
	 translation(Vector::zero)
	{
	}

const char* TransformNode::getClassName(void) const
	{
	return className;
	}

EventOut* TransformNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventOut(this,center);
	else if(strcmp(fieldName,"rotation")==0)
		return makeEventOut(this,rotation);
	else if(strcmp(fieldName,"scale")==0)
		return makeEventOut(this,scale);
	else if(strcmp(fieldName,"scaleOrientation")==0)
		return makeEventOut(this,scaleOrientation);
	else if(strcmp(fieldName,"translation")==0)
		return makeEventOut(this,translation);
	else
		return GroupNode::getEventOut(fieldName);
	}

EventIn* TransformNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventIn(this,center);
	else if(strcmp(fieldName,"rotation")==0)
		return makeEventIn(this,rotation);
	else if(strcmp(fieldName,"scale")==0)
		return makeEventIn(this,scale);
	else if(strcmp(fieldName,"scaleOrientation")==0)
		return makeEventIn(this,scaleOrientation);
	else if(strcmp(fieldName,"translation")==0)
		return makeEventIn(this,translation);
	else
		return GroupNode::getEventIn(fieldName);
	}

void TransformNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"center")==0)
		{
		vrmlFile.parseField(center);
		}
	else if(strcmp(fieldName,"rotation")==0)
		{
		vrmlFile.parseField(rotation);
		}
	else if(strcmp(fieldName,"scale")==0)
		{
		vrmlFile.parseField(scale);
		}
	else if(strcmp(fieldName,"scaleOrientation")==0)
		{
		vrmlFile.parseField(scaleOrientation);
		}
	else if(strcmp(fieldName,"translation")==0)
		{
		vrmlFile.parseField(translation);
		}
	else
		GroupNode::parseField(fieldName,vrmlFile);
	}

unsigned int TransformNode::update(void)
	{
	typedef DOGTransform::Scalar DScalar;
	typedef DOGTransform::Vector DVector;
	typedef DOGTransform::Point DPoint;
	typedef DOGTransform::Rotation DRotation;
	
	/* Calculate the transformation: */
	transform=DOGTransform::translate(DVector(translation.getValue()));
	DScalar uniformScale=Math::pow(DScalar(scale.getValue()[0])*DScalar(scale.getValue()[1])*DScalar(scale.getValue()[2]),DScalar(1)/DScalar(3));
	if(center.getValue()!=Point::origin)
		{
		DPoint dcenter(center.getValue());
		transform*=DOGTransform::translateFromOriginTo(dcenter);
		if(uniformScale!=DScalar(1))
			transform*=DOGTransform::scale(uniformScale);
		transform*=DOGTransform::rotate(DRotation(rotation.getValue()));
		transform*=DOGTransform::translateToOriginFrom(dcenter);
		}
	else
		{
		if(uniformScale!=Scalar(1))
			transform*=OGTransform::scale(uniformScale);
		transform*=OGTransform::rotate(rotation.getValue());
		}
	transform.renormalize();
	
	/* Call the base class method: */
	return GroupNode::update();
	}

Box TransformNode::calcBoundingBox(void) const
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
			childBox.transform(transform);
			result.addBox(childBox);
			}
		return result;
		}
	}

void TransformNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Transform the collision query: */
	SphereCollisionQuery transformedCollisionQuery=collisionQuery.transform(transform);
	
	/* Delegate to the base class: */
	GroupNode::testCollision(transformedCollisionQuery);
	
	/* Update the original collision query if the transformed copy was updated: */
	if(transformedCollisionQuery.getHitLambda()<collisionQuery.getHitLambda())
		collisionQuery.updateFromTransform(transform,transformedCollisionQuery);
	}

void TransformNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	DOGTransform previousTransform=renderState.pushTransform(transform);
	
	/* Delegate to the base class: */
	GroupNode::glRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

void TransformNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	DOGTransform previousTransform=renderState.pushTransform(transform);
	
	/* Delegate to the base class: */
	GroupNode::alRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

}
