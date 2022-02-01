/***********************************************************************
CylinderNode - Class for upright circular cylinders as renderable
geometry.
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

#include <SceneGraph/CylinderNode.h>

#include <string.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLNormalTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {


/*************************************
Static elements of class CylinderNode:
*************************************/

const char* CylinderNode::className="Cylinder";

/*****************************
Methods of class CylinderNode:
*****************************/

void CylinderNode::createList(GLContextData& renderState) const
	{
	Scalar h=height.getValue();
	Scalar h2=Math::div2(h);
	Scalar r=radius.getValue();
	int ns=numSegments.getValue();
	
	if(side.getValue())
		{
		/* Draw the cylinder side: */
		glBegin(GL_QUAD_STRIP);
		glNormal(Scalar(0),Scalar(0),Scalar(-1));
		glTexCoord2f(0.0f,1.0f);
		glVertex(Scalar(0),h2,-r);
		glTexCoord2f(0.0f,0.0f);
		glVertex(Scalar(0),-h2,-r);
		for(int i=1;i<ns;++i)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(ns);
			float texS=float(i)/float(ns);
			Scalar c=Math::cos(angle);
			Scalar s=Math::sin(angle);
			glNormal(-s,Scalar(0),-c);
			glTexCoord2f(texS,1.0f);
			glVertex(-s*r, h2,-c*r);
			glTexCoord2f(texS,0.0f);
			glVertex(-s*r,-h2,-c*r);
			}
		glNormal(Scalar(0),Scalar(0),Scalar(-1));
		glTexCoord2f(1.0f,1.0f);
		glVertex(Scalar(0),h2,-r);
		glTexCoord2f(1.0f,0.0f);
		glVertex(Scalar(0),-h2,-r);
		glEnd();
		}
	
	if(bottom.getValue())
		{
		/* Draw the cylinder bottom: */
		glBegin(GL_TRIANGLE_FAN);
		glNormal(Scalar(0),Scalar(-1),Scalar(0));
		glTexCoord2f(0.5f,0.5f);
		glVertex(Scalar(0),-h2,Scalar(0));
		glTexCoord2f(0.5f,0.0f);
		glVertex(Scalar(0),-h2,-r);
		for(int i=ns-1;i>0;--i)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(ns);
			Scalar c=Math::cos(angle);
			Scalar s=Math::sin(angle);
			glTexCoord2f(-float(s)*0.5f+0.5f,-float(c)*0.5f+0.5f);
			glVertex(-s*r,-h2,-c*r);
			}
		glTexCoord2f(0.5f,0.0f);
		glVertex(Scalar(0),-h2,-r);
		glEnd();
		}
	
	if(top.getValue())
		{
		/* Draw the cylinder top: */
		glBegin(GL_TRIANGLE_FAN);
		glNormal(Scalar(0),Scalar(1),Scalar(0));
		glTexCoord2f(0.5f,0.5f);
		glVertex(Scalar(0),h2,Scalar(0));
		glTexCoord2f(0.5f,1.0f);
		glVertex(Scalar(0),h2,-r);
		for(int i=1;i<ns;++i)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(ns);
			Scalar c=Math::cos(angle);
			Scalar s=Math::sin(angle);
			glTexCoord2f(-float(s)*0.5f+0.5f,float(c)*0.5f+0.5f);
			glVertex(-s*r,h2,-c*r);
			}
		glTexCoord2f(0.5f,1.0f);
		glVertex(Scalar(0),h2,-r);
		glEnd();
		}
	}

CylinderNode::CylinderNode(void)
	:height(Scalar(2)),
	 radius(Scalar(1)),
	 numSegments(12),
	 side(true),
	 bottom(true),
	 top(true)
	{
	}

const char* CylinderNode::getClassName(void) const
	{
	return className;
	}

void CylinderNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"height")==0)
		{
		vrmlFile.parseField(height);
		}
	else if(strcmp(fieldName,"radius")==0)
		{
		vrmlFile.parseField(radius);
		}
	else if(strcmp(fieldName,"numSegments")==0)
		{
		vrmlFile.parseField(numSegments);
		}
	else if(strcmp(fieldName,"side")==0)
		{
		vrmlFile.parseField(side);
		}
	else if(strcmp(fieldName,"bottom")==0)
		{
		vrmlFile.parseField(bottom);
		}
	else if(strcmp(fieldName,"top")==0)
		{
		vrmlFile.parseField(top);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

unsigned int CylinderNode::update(void)
	{
	/* Invalidate the display list: */
	DisplayList::update();
	
	return NoCascade;
	}

Box CylinderNode::calcBoundingBox(void) const
	{
	/* Calculate the bounding box: */
	Scalar r=radius.getValue();
	Scalar h2=Math::div2(height.getValue());
	return Box(Point(-r,-h2,-r),Point(r,h2,r));
	}

void CylinderNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	const Point& c0=collisionQuery.getC0();
	const Vector& c0c1=collisionQuery.getC0c1();
	Scalar r=collisionQuery.getRadius();
	
	Scalar bh=c0[0]*c0c1[0]+c0[2]*c0c1[2]; // Halved linear coefficient from quadratic formula
	Scalar rSqr=Math::sqr(radius.getValue()+r);
	Scalar c0Sqr=Math::sqr(c0[0])+Math::sqr(c0[2]);
	Scalar c=c0Sqr-rSqr; // Constant coefficient from quadratic formula
	if(bh<Scalar(0))
		{
		Scalar a=Math::sqr(c0c1[0])+Math::sqr(c0c1[2]); // Quadratic coefficient from quadratic formula
		Scalar discq=bh*bh-a*c; // Quarter discriminant from quadratic formula
		if(discq>=Scalar(0))
			{
			/* Find the quadratic equation's smaller root: */
			// Scalar lambda=bh>=Scalar(0)?(-bh-Math::sqrt(discq))/a:c/(-bh+Math::sqrt(discq)); // Slightly more stable formula
			Scalar lambda=c/(-bh+Math::sqrt(discq)); // Stable formulation for negative bh, which is the only bh that counts
			if(lambda<collisionQuery.getHitLambda())
				{
				/* Check if the collision is on the cylinder's mantle: */
				Scalar y=c0[1]+c0c1[1]*lambda;
				Scalar h2=Math::div2(height.getValue());
				Scalar h2r=h2+r;
				if(y<-h2r)
					{
					if(c0c1[1]>Scalar(0))
						{
						Scalar lambda=(-h2r-c0[1])/c0c1[1];
						if(lambda<collisionQuery.getHitLambda()&&Math::sqr(c0[0]+c0c1[0]*lambda)+Math::sqr(c0[2]+c0c1[2]*lambda)<=rSqr)
							{
							/* Report a bottom-face collision: */
							collisionQuery.update(lambda,Vector(0,-1,0));
							}
						}
					}
				else if(y>h2r)
					{
					if(c0c1[1]<Scalar(0))
						{
						Scalar lambda=(h2r-c0[1])/c0c1[1];
						if(lambda<collisionQuery.getHitLambda()&&Math::sqr(c0[0]+c0c1[0]*lambda)+Math::sqr(c0[2]+c0c1[2]*lambda)<=rSqr)
							{
							/* Report a top-face collision: */
							collisionQuery.update(lambda,Vector(0,1,0));
							}
						}
					}
				else if(lambda>=Scalar(0))
					{
					/* Report a mantle hit: */
					collisionQuery.update(lambda,Vector(c0[0]+c0c1[0]*lambda,0,c0[2]+c0c1[2]*lambda));
					}
				else
					{
					/* The sphere is already intersecting the cylinder; prevent it from getting worse: */
					collisionQuery.update(Scalar(0),Vector(c0[0],0,c0[2]));
					}
				}
			}
		}
	else if(c<=Scalar(0))
		{
		/* Check the sphere's path against the cylinder's top or bottom face: */
		Scalar h2=Math::div2(height.getValue());
		Scalar h2r=h2+r;
		if(c0[1]<-h2r)
			{
			if(c0c1[1]>Scalar(0))
				{
				Scalar lambda=(-h2r-c0[1])/c0c1[1];
				if(lambda<collisionQuery.getHitLambda()&&Math::sqr(c0[0]+c0c1[0]*lambda)+Math::sqr(c0[2]+c0c1[2]*lambda)<=rSqr)
					{
					/* Report a bottom-face collision: */
					collisionQuery.update(lambda,Vector(0,-1,0));
					}
				}
			}
		else if(c0[1]>h2r)
			{
			if(c0c1[1]<Scalar(0))
				{
				Scalar lambda=(h2r-c0[1])/c0c1[1];
				if(lambda<collisionQuery.getHitLambda()&&Math::sqr(c0[0]+c0c1[0]*lambda)+Math::sqr(c0[2]+c0c1[2]*lambda)<=rSqr)
					{
					/* Report a top-face collision: */
					collisionQuery.update(lambda,Vector(0,1,0));
					}
				}
			}
		else
			{
			/* Sphere is already intersecting the cylinder, ensure it doesn't get worse: */
			if(c0[1]<-h2)
				{
				if(c0c1[1]>Scalar(0))
					collisionQuery.update(Scalar(0),Vector(0,-1,0));
				}
			else if(c0[1]>h2)
				{
				if(c0c1[1]<Scalar(0))
					collisionQuery.update(Scalar(0),Vector(0,1,0));
				}
			else if(c0Sqr<Math::sqr(radius.getValue()))
				{
				if(c0[1]<Scalar(0)&&c0c1[1]>Scalar(0))
					collisionQuery.update(Scalar(0),Vector(0,-1,0));
				else if(c0[1]>Scalar(0)&&c0c1[1]<Scalar(0))
					collisionQuery.update(Scalar(0),Vector(0,1,0));
				}
			}
		}
	}

void CylinderNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	renderState.setFrontFace(GL_CCW);
	renderState.enableCulling(GL_BACK);
	
	/* Render the display list: */
	DisplayList::glRenderAction(renderState.contextData);
	}

}
