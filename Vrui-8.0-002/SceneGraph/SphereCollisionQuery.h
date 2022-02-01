/***********************************************************************
SphereCollisionQuery - Class to query the collision of a sliding sphere
with a scene graph.
Copyright (c) 2020 Oliver Kreylos

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

#ifndef SCENEGRAPH_SPHERECOLLISIONQUERY_INCLUDED
#define SCENEGRAPH_SPHERECOLLISIONQUERY_INCLUDED

#include <Math/Interval.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <SceneGraph/Geometry.h>

namespace SceneGraph {

class SphereCollisionQuery
	{
	/* Elements: */
	private:
	Point c0; // Initial center point of sphere
	Vector c0c1; // Vector from initial to final point
	Scalar radius; // Sphere's radius
	Scalar c0c1Sqr; // Squared length of vector from initial to final point
	Scalar radiusSqr; // Sphere's squared radius
	
	/* Transient processing state: */
	Scalar hitLambda; // Ratio of sliding vector at which the sphere hit geometry
	Vector hitNormal; // Normal vector of hit plane (not normalized)
	
	/* Constructors and destructors: */
	public:
	SphereCollisionQuery(const Point& sC0,const Vector& sC0c1,Scalar sRadius); // Elementwise constructor
	
	/* Methods: */
	const Point& getC0(void) const
		{
		return c0;
		}
	Scalar getRadius(void) const
		{
		return radius;
		}
	const Vector& getC0c1(void) const
		{
		return c0c1;
		}
	Scalar getC0c1Sqr(void) const
		{
		return c0c1Sqr;
		}
	Scalar getRadiusSqr(void) const
		{
		return radiusSqr;
		}
	
	/* Helper methods to test against primitives: */
	Math::Interval<Scalar> calcBoxInterval(const Box& box) const; // Returns the trace parameter interval for which the sphere hits the given box based on current collision state
	bool doesHitBox(const Box& box) const; // Returns true if the sphere hits the given box based on current collision state
	bool testVertexAndUpdate(const Point& vertex); // Tests sphere against a vertex; returns true if previous hit result changed
	bool testEdgeAndUpdate(const Point& vertex0,const Point& vertex1); // Tests sphere against an edge defined by two vertices; returns true if previous hit result changed
	bool testPlaneAndUpdate(const Point& center,const Vector& normal); // Tests sphere against a plane; returns true if previous hit result changed
	
	/* Transformation methods: */
	SphereCollisionQuery transform(const OGTransform& transform) const; // Returns a transformed copy of the collision query
	SphereCollisionQuery& updateFromTransform(const OGTransform& transform,const SphereCollisionQuery& transformedQuery); // Updates a collision query from a transformed copy
	
	/* Update methods: */
	bool doesUpdate(Scalar lambda) const // Returns true if the given collision parameter will update the collision result
		{
		return lambda>=Scalar(0)&&lambda<hitLambda;
		}
	void update(Scalar newHitLambda,const Vector& newHitNormal) // Updates the collision result; assumes that doesUpdate() returned true
		{
		/* Update the result: */
		hitLambda=newHitLambda;
		hitNormal=newHitNormal;
		}
	
	/* Result access methods: */
	bool isHit(void) const // Returns true if the collision query resulted in a hit and the hit normal is valid
		{
		return hitLambda<Scalar(1);
		}
	Scalar getHitLambda(void) const
		{
		return hitLambda;
		}
	Point getHitPoint(void) const
		{
		return c0+c0c1*hitLambda;
		}
	const Vector& getHitNormal(void) const
		{
		return hitNormal;
		}
	};

}

#endif
