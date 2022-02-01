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

#include <SceneGraph/BillboardNode.h>

#include <string.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <AL/ALContextData.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>
#include <SceneGraph/ALRenderState.h>

namespace SceneGraph {

/**************************************
Static elements of class BillboardNode:
**************************************/

const char* BillboardNode::className="Billboard";

/******************************
Methods of class BillboardNode:
******************************/

DOGTransform BillboardNode::calcBillboardTransform(TraversalState& traversalState) const
	{
	typedef DOGTransform::Scalar DScalar;
	typedef DOGTransform::Vector DVector;
	typedef DOGTransform::Rotation DRotation;
	
	/* Calculate the billboard transformation: */
	DOGTransform transform;
	DVector viewDirection(traversalState.getViewerPos()-Point::origin);
	if(aor2!=DScalar(0))
		{
		/* Rotate the billboard around its axis: */
		viewDirection-=aor*((viewDirection*aor)/aor2);
		DScalar vdLen=viewDirection.mag();
		if(vdLen!=DScalar(0))
			{
			DScalar angle=Math::acos((viewDirection*orthoZAxis)/vdLen);
			if(rotationNormal*viewDirection<DScalar(0))
				angle=-angle;
			transform=DOGTransform::rotate(DRotation::rotateAxis(aor,angle));
			}
		else
			transform=DOGTransform::identity;
		}
	else
		{
		/* Align the billboard's Z axis with the viewing direction: */
		transform=DOGTransform::rotate(DRotation::rotateFromTo(DVector(0,0,1),viewDirection));
		
		/* Rotate the billboard's Y axis into the plane formed by the viewing direction and the up direction: */
		DVector up=transform.inverseTransform(DVector(traversalState.getUpVector()));
		if(up[0]!=DScalar(0)||up[1]!=DScalar(0))
			{
			DScalar angle=Math::atan2(-up[0],up[1]);
			transform*=DOGTransform::rotate(DRotation::rotateZ(angle));
			}
		}
	
	return transform;
	}

BillboardNode::BillboardNode(void)
	:axisOfRotation(Vector(0,1,0)),
	 aor(0,1,0),aor2(1),
	 orthoZAxis(0,0,1),
	 rotationNormal(1,0,0)
	{
	}

const char* BillboardNode::getClassName(void) const
	{
	return className;
	}

EventOut* BillboardNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"axisOfRotation")==0)
		return makeEventOut(this,axisOfRotation);
	else
		return GroupNode::getEventOut(fieldName);
	}

EventIn* BillboardNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"axisOfRotation")==0)
		return makeEventIn(this,axisOfRotation);
	else
		return GroupNode::getEventIn(fieldName);
	}

void BillboardNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"axisOfRotation")==0)
		{
		vrmlFile.parseField(axisOfRotation);
		}
	else
		GroupNode::parseField(fieldName,vrmlFile);
	}

unsigned int BillboardNode::update(void)
	{
	/* Compute the orthonormalized Z axis: */
	aor=DVector(axisOfRotation.getValue());
	aor2=aor.sqr();
	if(aor2!=DScalar(0))
		{
		orthoZAxis=DVector(0,0,1);
		orthoZAxis-=aor*((orthoZAxis*aor)/aor2);
		orthoZAxis.normalize();
		rotationNormal=aor^orthoZAxis;
		}
	
	/* Call the base class method: */
	return GroupNode::update();
	}

void BillboardNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Billboard nodes can't collide in the current setup */
	}

void BillboardNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Calculate and apply the billboard transformation: */
	DOGTransform previousTransform=renderState.pushTransform(calcBillboardTransform(renderState));
	
	/* Delegate to the base class: */
	GroupNode::glRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

void BillboardNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Calculate and apply the billboard transformation: */
	DOGTransform previousTransform=renderState.pushTransform(calcBillboardTransform(renderState));
	
	/* Delegate to the base class: */
	GroupNode::alRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

}
