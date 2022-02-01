/***********************************************************************
LensDistortion - Classes representing different correction formulas for
non-linear lens distortion.
Copyright (c) 2015-2020 Oliver Kreylos

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

#ifndef VIDEO_LENSDISTORTION_INCLUDED
#define VIDEO_LENSDISTORTION_INCLUDED

#include <stdexcept>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

/**************************************************************
Different types of lens distortion formula for experimentation:
**************************************************************/

#define VIDEO_LENSDISTORTION_NUMKAPPAS 6 // Number of radial formula coefficients
#define VIDEO_LENSDISTORTION_INVERSERADIAL 0 // Apply inverse of radial formula as scaling factor
#define VIDEO_LENSDISTORTION_RATIONALRADIAL 1 // Radial function is rational, with half of coefficients in the denominator

namespace Video {

class LensDistortion
	{
	/* Embedded classes: */
	public:
	typedef double Scalar; // Scalar type for points and vectors
	typedef Geometry::Point<Scalar,2> Point; // Type for points in image space
	typedef Geometry::Vector<Scalar,2> Vector; // Type for vectors in image space
	static const int numKappas=VIDEO_LENSDISTORTION_NUMKAPPAS; // Number of radial distortion coefficients
	#if VIDEO_LENSDISTORTION_RATIONALRADIAL
	static const int numNumeratorKappas=(numKappas+1)/2; // Number of radial distortion coefficients in the numerator of the formula
	#endif
	static const int numRhos=2; // Number of tangential distortion coefficients
	static const int numParameters=2+numKappas+numRhos; // Number of scalar parameters in the lens distortion correction formula
	typedef Geometry::ComponentArray<Scalar,numParameters> ParameterVector; // A vector containing the scalar parameters of a lens distortion correction formula
	
	/* Elements: */
	private:
	Point center; // Center point for lens distortion
	Scalar kappas[numKappas]; // Radial distortion coefficients
	Scalar rhos[numRhos]; // Tangential distortion coefficients
	Scalar maxR2; // Maximum distance from the lens distortion center before distortion wraps back
	
	/* Constructors and destructors: */
	public:
	LensDistortion(void) // Creates an identity formula
		:center(Point::origin),
		 maxR2(Math::Constants<Scalar>::max)
		{
		for(int i=0;i<numKappas;++i)
			kappas[i]=Scalar(0);
		for(int i=0;i<numRhos;++i)
			rhos[i]=Scalar(0);
		}
	
	/* Methods: */
	bool isIdentity(void) const // Returns true if this is a no-op identity lens distortion correction
		{
		/* Test all radial and tangential distortion coefficients: */
		bool result=true;
		for(int i=0;i<numKappas&&result;++i)
			result=kappas[i]==Scalar(0);
		for(int i=0;i<numRhos&&result;++i)
			result=rhos[i]==Scalar(0);
		return result;
		}
	bool isRadial(void) const // Returns true if this is a radial-only lens distortion correction formula
		{
		/* Test all tangential distortion coefficients: */
		bool result=true;
		for(int i=0;i<numRhos&&result;++i)
			result=rhos[i]==Scalar(0);
		return result;
		}
	const Point& getCenter(void) const
		{
		return center;
		}
	void setCenter(const Point& newCenter) // Sets the distortion center point
		{
		center=newCenter;
		
		/* Update the maximum distortion radius: */
		maxR2=calcMaxR2();
		}
	Scalar getKappa(int index) const
		{
		if(index<0||index>=numKappas)
			throw std::runtime_error("Video::LensDistortion::getKappa: Index out of range");
		return kappas[index];
		}
	void setKappa(int index,Scalar newKappa) // Sets one radial distortion coefficient
		{
		if(index<0||index>=numKappas)
			throw std::runtime_error("Video::LensDistortion::setKappa: Index out of range");
		kappas[index]=newKappa;
		
		/* Update the maximum distortion radius: */
		maxR2=calcMaxR2();
		}
	Scalar getRho(int index) const
		{
		if(index<0||index>=numRhos)
			throw std::runtime_error("Video::LensDistortion::getRho: Index out of range");
		return rhos[index];
		}
	void setRho(int index,Scalar newRho) // Sets one tangential distortion coefficient
		{
		if(index<0||index>=numRhos)
			throw std::runtime_error("Video::LensDistortion::setRho: Index out of range");
		rhos[index]=newRho;
		
		/* Update the maximum distortion radius: */
		maxR2=calcMaxR2();
		}
	static ParameterVector getParameterScales(Scalar imageScale) // Returns a vector of parameter scaling factors for a given image scale
		{
		ParameterVector result;
		
		/* First two parameters are distortion center: */
		result[0]=imageScale;
		result[1]=imageScale;
		
		/* Next parameters are radial distortion coefficients: */
		#if VIDEO_LENSDISTORTION_RATIONALRADIAL
		Scalar is=imageScale;
		for(int i=0;i<numNumeratorKappas;++i)
			{
			result[2+i]=is;
			is*=imageScale;
			}
		is=imageScale;
		for(int i=numNumeratorKappas;i<numKappas;++i)
			{
			result[2+i]=is;
			is*=imageScale;
			}
		#else
		Scalar is=imageScale;
		for(int i=0;i<numKappas;++i)
			{
			result[2+i]=is;
			is*=imageScale;
			}
		#endif
		
		/* Next parameters are tangential distortion coefficients: */
		for(int i=0;i<numRhos;++i)
			result[2+numKappas+i]=Math::sqrt(imageScale);
		
		return result;
		}
	ParameterVector getParameterVector(void) const // Returns a parameter vector describing the current formula
		{
		/* Write parameters into a parameter vector: */
		ParameterVector result;
		for(int i=0;i<2;++i)
			result[0+i]=center[i];
		for(int i=0;i<numKappas;++i)
			result[2+i]=kappas[i];
		for(int i=0;i<numRhos;++i)
			result[2+numKappas+i]=rhos[i];
		return result;
		}
	void setParameterVector(const ParameterVector& parameterVector) // Sets the current formula based on the given parameter vector
		{
		/* Set parameters from the parameter vector: */
		for(int i=0;i<2;++i)
			center[i]=parameterVector[0+i];
		for(int i=0;i<numKappas;++i)
			kappas[i]=parameterVector[2+i];
		for(int i=0;i<numRhos;++i)
			rhos[i]=parameterVector[2+numKappas+i];
		
		/* Update the maximum distortion radius: */
		maxR2=calcMaxR2();
		}
	Scalar getMaxR2(void) const // Returns the cached maximum squared undistortion radius
		{
		return maxR2;
		}
	Scalar calcMaxR2(void) const; // Returns the maximum squared distance from the center any undistorted point can have before things to to shit
	bool canDistort(const Point& undistorted) const // Returns true if the given point can be fed into the distortion formula without causing wrap-around
		{
		Scalar r2=Math::sqr(undistorted[0]-center[0])+Math::sqr(undistorted[1]-center[1]);
		return r2<maxR2;
		}
	Point distort(const Point& undistorted) const // Calculates forward lens distortion correction formula for the given tangent-space point
		{
		/* Calculate the radial correction coefficient: */
		Vector d=undistorted-center;
		Scalar r2=d.sqr();
		#if VIDEO_LENSDISTORTION_RATIONALRADIAL
		Scalar radialn(0);
		for(int i=numNumeratorKappas-1;i>=0;--i)
			radialn=(radialn+kappas[i])*r2;
		Scalar radiald(0);
		for(int i=numKappas-1;i>=numNumeratorKappas;--i)
			radiald=(radiald+kappas[i])*r2;
		Scalar radial=(Scalar(1)+radialn)/(Scalar(1)+radiald);
		#else
		Scalar radial(0);
		for(int i=numKappas-1;i>=0;--i)
			radial=(radial+kappas[i])*r2;
		radial+=Scalar(1);
		#if VIDEO_LENSDISTORTION_INVERSERADIAL
		radial=Scalar(1)/radial;
		#endif
		#endif
		
		/* Apply radial and tangential distortion correction: */
		return Point(center[0]+d[0]*radial+Scalar(2)*rhos[0]*d[0]*d[1]+rhos[1]*(r2+Scalar(2)*d[0]*d[0]), // Tangential distortion formula in x
		             center[1]+d[1]*radial+rhos[0]*(r2+Scalar(2)*d[1]*d[1])+Scalar(2)*rhos[1]*d[0]*d[1]); // Tangential distortion formula in y
		}
	Point distort(int x,int y) const // Ditto, for center point of pixel with given index
		{
		return distort(Point(Scalar(x)+Scalar(0.5),Scalar(y)+Scalar(0.5)));
		}
	Point undistort(const Point& distorted) const; // Calculates inverse lens distortion correction formula via Newton-Raphson iteration for the given tangent-space point
	Point undistort(int x,int y) const // Ditto, for center point of pixel with given index
		{
		return undistort(Point(Scalar(x)+Scalar(0.5),Scalar(y)+Scalar(0.5)));
		}
	};

}

#endif
