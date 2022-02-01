/***********************************************************************
BoxNode - Class for axis-aligned boxes as renderable geometry.
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

#include <SceneGraph/BoxNode.h>

#include <string.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLVertexTemplates.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/********************************
Static elements of class BoxNode:
********************************/

const char* BoxNode::className="Box";

/************************
Methods of class BoxNode:
************************/

void BoxNode::createList(GLContextData& renderState) const
	{
	/* Draw the box faces as quads: */
	glBegin(GL_QUADS);
	
	/* Bottom face: */
	glNormal3f(0.0f,-1.0f,0.0f);
	glTexCoord2f(0.0f,0.0f);
	glVertex(box.min[0],box.min[1],box.min[2]);
	glTexCoord2f(1.0f,0.0f);
	glVertex(box.max[0],box.min[1],box.min[2]);
	glTexCoord2f(1.0f,1.0f);
	glVertex(box.max[0],box.min[1],box.max[2]);
	glTexCoord2f(0.0f,1.0f);
	glVertex(box.min[0],box.min[1],box.max[2]);
	
	/* Front face: */
	glNormal3f(0.0f,0.0f,1.0f);
	glTexCoord2f(0.0f,0.0f);
	glVertex(box.min[0],box.min[1],box.max[2]);
	glTexCoord2f(1.0f,0.0f);
	glVertex(box.max[0],box.min[1],box.max[2]);
	glTexCoord2f(1.0f,1.0f);
	glVertex(box.max[0],box.max[1],box.max[2]);
	glTexCoord2f(0.0f,1.0f);
	glVertex(box.min[0],box.max[1],box.max[2]);
	
	/* Right face: */
	glNormal3f(1.0f,0.0f,0.0f);
	glTexCoord2f(0.0f,0.0f);
	glVertex(box.max[0],box.min[1],box.max[2]);
	glTexCoord2f(1.0f,0.0f);
	glVertex(box.max[0],box.min[1],box.min[2]);
	glTexCoord2f(1.0f,1.0f);
	glVertex(box.max[0],box.max[1],box.min[2]);
	glTexCoord2f(0.0f,1.0f);
	glVertex(box.max[0],box.max[1],box.max[2]);
	
	/* Back face: */
	glNormal3f(0.0f,0.0f,-1.0f);
	glTexCoord2f(0.0f,0.0f);
	glVertex(box.max[0],box.min[1],box.min[2]);
	glTexCoord2f(1.0f,0.0f);
	glVertex(box.min[0],box.min[1],box.min[2]);
	glTexCoord2f(1.0f,1.0f);
	glVertex(box.min[0],box.max[1],box.min[2]);
	glTexCoord2f(0.0f,1.0f);
	glVertex(box.max[0],box.max[1],box.min[2]);
	
	/* Left face: */
	glNormal3f(-1.0f,0.0f,0.0f);
	glTexCoord2f(0.0f,0.0f);
	glVertex(box.min[0],box.min[1],box.min[2]);
	glTexCoord2f(1.0f,0.0f);
	glVertex(box.min[0],box.min[1],box.max[2]);
	glTexCoord2f(1.0f,1.0f);
	glVertex(box.min[0],box.max[1],box.max[2]);
	glTexCoord2f(0.0f,1.0f);
	glVertex(box.min[0],box.max[1],box.min[2]);
	
	/* Top face: */
	glNormal3f(0.0f, 1.0f,0.0f);
	glTexCoord2f(0.0f,0.0f);
	glVertex(box.min[0],box.max[1],box.max[2]);
	glTexCoord2f(1.0f,0.0f);
	glVertex(box.max[0],box.max[1],box.max[2]);
	glTexCoord2f(1.0f,1.0f);
	glVertex(box.max[0],box.max[1],box.min[2]);
	glTexCoord2f(0.0f,1.0f);
	glVertex(box.min[0],box.max[1],box.min[2]);
	
	glEnd();
	}

BoxNode::BoxNode(void)
	:center(Point::origin),
	 size(Size(2,2,2)),
	 box(Point(-1,-1,-1),Point(1,1,1))
	{
	}

const char* BoxNode::getClassName(void) const
	{
	return className;
	}

EventOut* BoxNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventOut(this,center);
	else if(strcmp(fieldName,"size")==0)
		return makeEventOut(this,size);
	else
		return GeometryNode::getEventOut(fieldName);
	}

EventIn* BoxNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventIn(this,center);
	else if(strcmp(fieldName,"size")==0)
		return makeEventIn(this,size);
	else
		return GeometryNode::getEventIn(fieldName);
	}

void BoxNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"center")==0)
		{
		vrmlFile.parseField(center);
		}
	else if(strcmp(fieldName,"size")==0)
		{
		vrmlFile.parseField(size);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

unsigned int BoxNode::update(void)
	{
	Point pmin=center.getValue();
	Point pmax=center.getValue();
	for(int i=0;i<3;++i)
		{
		Scalar halfSize=Math::div2(size.getValue()[i]);
		pmin[i]-=halfSize;
		pmax[i]+=halfSize;
		}
	box=Box(pmin,pmax);
	
	/* Invalidate the display list: */
	DisplayList::update();
	
	return NoCascade;
	}

Box BoxNode::calcBoundingBox(void) const
	{
	/* Return the box itself: */
	return box;
	}

void BoxNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Test the sphere against the box's faces and find the normal axis of the face where the sphere enters the box: */
	const Point& c0=collisionQuery.getC0();
	const Vector& c0c1=collisionQuery.getC0c1();
	Scalar radius=collisionQuery.getRadius();
	Scalar lMin(Math::Constants<Scalar>::min);
	Scalar lMax=collisionQuery.getHitLambda();
	int hitAxis=-1;
	for(int axis=0;axis<3;++axis)
		{
		if(c0c1[axis]!=Scalar(0))
			{
			/* Calculate the collision parameter interval where the sphere hits the box along the current axis: */
			Scalar l0,l1;
			if(c0c1[axis]<Scalar(0))
				{
				l0=(box.max[axis]+radius-c0[axis])/c0c1[axis];
				l1=(box.min[axis]-radius-c0[axis])/c0c1[axis];
				}
			else
				{
				l0=(box.min[axis]-radius-c0[axis])/c0c1[axis];
				l1=(box.max[axis]+radius-c0[axis])/c0c1[axis];
				}
			
			/* Intersect the interval with the current total: */
			if(l0>=lMin)
				{
				lMin=l0;
				hitAxis=axis;
				}
			lMax=Math::min(lMax,l1);
			
			/* Bail out if the intersection interval became invalid: */
			if(lMin>=lMax)
				return;
			}
		else if(c0[axis]<box.min[axis]-radius||c0[axis]>box.max[axis]+radius)
			{
			/* Bail out immediately: */
			return;
			}
		}
	
	/* Bail out if the sphere is already past the box: */
	if(lMax<=Scalar(0))
		return;
	
	/* Calculate the point where the sphere enters the box: */
	int fas[2];
	Scalar hp[2];
	int outMask=0x0;
	int outDir[2];
	for(int i=0;i<2;++i)
		{
		fas[i]=(hitAxis+i+1)%3;
		hp[i]=c0[fas[i]]+c0c1[fas[i]]*lMin;
		if(hp[i]<box.min[fas[i]])
			{
			/* Outside on the min side: */
			outMask|=0x1<<i;
			outDir[i]=-1;
			}
		if(hp[i]>box.max[fas[i]])
			{
			/* Outside on the max side: */
			outMask|=0x1<<i;
			outDir[i]=1;
			}
		}
	
	/* Check if the face intersection is valid: */
	if(outMask==0x0)
		{
		/* Calculate the outwards-pointing normal vector of the hit face: */
		Vector hitNormal=Vector::zero;
		hitNormal[hitAxis]=c0c1[hitAxis]<Scalar(0)?Scalar(1):Scalar(-1);
		
		if(lMin>=Scalar(0))
			{
			/* This is the actual collision; report it: */
			collisionQuery.update(lMin,hitNormal);
			}
		else
			{
			/* Sphere is already intersecting the box; check if the motion will make it worse: */
			Scalar mid=Math::mid(box.min[hitAxis],box.max[hitAxis]);
			if((c0c1[hitAxis]<Scalar(0)?c0[hitAxis]>mid:c0[hitAxis]<mid)&&collisionQuery.getHitLambda()>Scalar(0))
				{
				/* Keep it from getting worse: */
				collisionQuery.update(Scalar(0),hitNormal);
				}
			}
		
		return;
		}
	
	/* Check whether the outside hit is a vertex hit or an edge hit: */
	if(outMask==0x3)
		{
		/* Construct the corner vertex and its opposite vertex and determine which adjacent edges to test: */
		Point v,ov;
		if(c0c1[hitAxis]<Scalar(0))
			{
			v[hitAxis]=box.max[hitAxis];
			ov[hitAxis]=box.min[hitAxis];
			}
		else
			{
			v[hitAxis]=box.min[hitAxis];
			ov[hitAxis]=box.max[hitAxis];
			}
		bool testEdges[2];
		for(int i=0;i<2;++i)
			{
			if(outDir[i]>0)
				{
				v[fas[i]]=box.max[fas[i]];
				ov[fas[i]]=box.min[fas[i]];
				testEdges[i]=c0c1[fas[i]]<Scalar(0);
				}
			else
				{
				v[fas[i]]=box.min[fas[i]];
				ov[fas[i]]=box.max[fas[i]];
				testEdges[i]=c0c1[fas[i]]>Scalar(0);
				}
			}
		collisionQuery.testVertexAndUpdate(v);
		
		/* Test the adjacent edges if necessary: */
		for(int i=0;i<2;++i)
			if(testEdges[i])
				{
				Point v2=v;
				v2[fas[i]]=ov[fas[i]];
				collisionQuery.testVertexAndUpdate(v2);
				collisionQuery.testEdgeAndUpdate(v,v2);
				}
		if(testEdges[0]||testEdges[1])
			{
			Point v2=v;
			v2[hitAxis]=ov[hitAxis];
			collisionQuery.testVertexAndUpdate(v2);
			collisionQuery.testEdgeAndUpdate(v,v2);
			}
		}
	else
		{
		/* Construct the edge: */
		Point e0,e1;
		e1[hitAxis]=e0[hitAxis]=c0c1[hitAxis]<Scalar(0)?box.max[hitAxis]:box.min[hitAxis];
		int outEdge;
		if(outMask==0x2)
			{
			outEdge=fas[0];
			e0[outEdge]=box.min[outEdge];
			e1[outEdge]=box.max[outEdge];
			e1[fas[1]]=e0[fas[1]]=outDir[1]>0?box.max[fas[1]]:box.min[fas[1]];
			}
		else
			{
			outEdge=fas[1];
			e1[fas[0]]=e0[fas[0]]=outDir[0]>0?box.max[fas[0]]:box.min[fas[0]];
			e0[outEdge]=box.min[outEdge];
			e1[outEdge]=box.max[outEdge];
			}
		collisionQuery.testEdgeAndUpdate(e0,e1);
		
		/* Test one of the adjacent vertices: */
		collisionQuery.testVertexAndUpdate(c0c1[outEdge]<Scalar(0)?e0:e1);
		}
	}

void BoxNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	renderState.setFrontFace(GL_CCW);
	renderState.enableCulling(GL_BACK);
	
	/* Render the display list: */
	DisplayList::glRenderAction(renderState.contextData);
	}

}
