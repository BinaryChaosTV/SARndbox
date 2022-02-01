/***********************************************************************
IntrinsicParameters - Class to represent camera intrinsic parameters.
Copyright (c) 2019-2020 Oliver Kreylos

This file is part of the Basic Video Library (Video).

The Basic Video Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Video Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Video Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef VIDEO_INTRINSICPARAMETERS_INCLUDED
#define VIDEO_INTRINSICPARAMETERS_INCLUDED

#include <Geometry/ComponentArray.h>
#include <Geometry/Matrix.h>
#include <Video/LensDistortion.h>

/* Forward declarations: */
namespace IO {
class File;
}

namespace Video {

class IntrinsicParameters
	{
	/* Embedded classes: */
	public:
	typedef LensDistortion::Scalar Scalar;
	typedef LensDistortion::Point ImagePoint;
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::Vector<Scalar,3> Vector;
	private:
	typedef Geometry::Matrix<Scalar,3,3> Matrix;
	typedef Geometry::ComponentArray<Scalar,3> CA;
	
	/* Elements: */
	unsigned int imageSize[2]; // Size of the video frame for which these parameters were computed
	Matrix m,mInv; // Intrinsic projection matrix and its inverse
	LensDistortion ld; // Lens distortion parameters
	
	/* Constructors and destructors: */
	public:
	IntrinsicParameters(void); // Creates default parameters
	IntrinsicParameters(unsigned int imageWidth,unsigned int imageHeight,Scalar focalLength); // Creates default parameters for the given image size and focal length
	
	/* Methods: */
	const unsigned int* getImageSize(void) const // Returns the video frame size
		{
		return imageSize;
		}
	const LensDistortion& getLensDistortion(void) const // Returns the lens distortion component
		{
		return ld;
		}
	bool matches(const unsigned int otherImageSize[2]) const // Returns true if the intrinsic parameters match the given video frame size
		{
		return imageSize[0]==otherImageSize[0]&&imageSize[1]==otherImageSize[1];
		}
	bool canUnproject(const ImagePoint& imagePoint) const
		{
		return ld.canDistort(imagePoint);
		}
	Vector unproject(const ImagePoint& imagePoint) const // Returns a tangent-space vector for the given point in image coordinates
		{
		/* Apply lens distortion correction: */
		ImagePoint cp=ld.distort(imagePoint);
		
		/* Unproject the point: */
		CA up=mInv*CA(cp[0],cp[1],Scalar(1));
		return Vector(up[0],up[1],Scalar(-1));
		}
	ImagePoint project(const Point& point) const // Returns a distortion-corrected image point for the given tangent-space point
		{
		/* Project the point: */
		CA p=m*CA(point[0],point[1],-point[2]);
		return ImagePoint(p[0]/p[2],p[1]/p[2]);
		}
	void read(IO::File& file); // Reads intrinsic camera parameters from a file
	void write(IO::File& file) const; // Writes intrinsic camera parameters to a file
	};

}

#endif
