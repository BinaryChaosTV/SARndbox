/***********************************************************************
PrimaryPlaneProjector - Helper class to project 3D points and vectors
into the 2D plane most closely aligned with a 3D plane.
Copyright (c) 2020 Oliver Kreylos

This file is part of the Templatized Geometry Library (TGL).

The Templatized Geometry Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Geometry Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Geometry Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef GEOMETRY_PRIMARYPLANEPROJECTOR_INCLUDED
#define GEOMETRY_PRIMARYPLANEPROJECTOR_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>

namespace Geometry {

template <class ScalarParam>
class PrimaryPlaneProjector
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type for points and vectors
	typedef Point<ScalarParam,3> Point3; // Type for 3D points
	typedef Vector<ScalarParam,3> Vector3; // Type for 3D vectors
	typedef Point<ScalarParam,2> Point2; // Type for 2D points
	typedef Vector<ScalarParam,2> Vector2; // Type for 2D vectors
	
	/* Elements: */
	private:
	int a0,a1; // The indices of the primary axes spanning the primary plane, ordered to retain handedness
	
	/* Constructors and destructors: */
	public:
	PrimaryPlaneProjector(const Vector3& planeNormal) // Creates a primary plane projector for a plane with the given normal vector
		{
		/* Find the primary plane best aligned with the given plane: */
		int pAxis=Geometry::findParallelAxis(planeNormal);
		
		/* Find the primary axes and projection order that projects while maintaining handedness: */
		if(planeNormal[pAxis]>=Scalar(0))
			{
			a0=(pAxis+1)%3;
			a1=(pAxis+2)%3;
			}
		else
			{
			a0=(pAxis+2)%3;
			a1=(pAxis+1)%3;
			}
		}
	
	/* Methods: */
	Point2 project(const Point3& point) const // Projects a point
		{
		return Point2(point[a0],point[a1]);
		}
	Vector2 project(const Vector3& vector) const // Projects a vector
		{
		return Vector2(vector[a0],vector[a1]);
		}
	};

}

#endif
