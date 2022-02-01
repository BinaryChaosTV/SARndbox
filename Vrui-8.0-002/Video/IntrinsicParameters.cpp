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

#include <Video/IntrinsicParameters.h>

#include <Misc/SizedTypes.h>
#include <IO/File.h>
#include <Math/Math.h>

// DEBUGGING
// #include <iostream>

namespace Video {

/************************************
Methods of class IntrinsicParameters:
************************************/

IntrinsicParameters::IntrinsicParameters(void)
	:m(Scalar(1)),mInv(Scalar(1))
	{
	imageSize[1]=imageSize[0]=0;
	}

IntrinsicParameters::IntrinsicParameters(unsigned int imageWidth,unsigned int imageHeight,Scalar focalLength)
	:m(Scalar(1)),mInv(Scalar(1))
	{
	/* Copy the video frame size: */
	imageSize[0]=imageWidth;
	imageSize[1]=imageHeight;
	
	/* Create the camera projection matrix and its inverse: */
	m(0,0)=focalLength;
	m(0,2)=Math::div2(Scalar(imageSize[0]));
	m(1,1)=focalLength;
	m(1,2)=Math::div2(Scalar(imageSize[1]));
	mInv(0,0)=Scalar(1)/focalLength;
	mInv(0,2)=-m(0,2)/focalLength;
	mInv(1,1)=Scalar(1)/focalLength;
	mInv(1,2)=-m(1,2)/focalLength;
	
	/* Create identity lens distortion parameters: */
	LensDistortion::ParameterVector parv;
	for(int i=0;i<2;++i)
		parv[i]=m(i,2);
	for(int i=0;i<LensDistortion::numKappas;++i)
		parv[2+i]=Scalar(0);
	for(int i=0;i<LensDistortion::numRhos;++i)
		parv[2+LensDistortion::numKappas+i]=Scalar(0);
	ld.setParameterVector(parv);
	}

void IntrinsicParameters::read(IO::File& file)
	{
	/* Read the video frame size: */
	for(int i=0;i<2;++i)
		imageSize[i]=file.read<Misc::UInt32>();
	
	/* Read the camera projection matrix: */
	Scalar cx=file.read<Misc::Float64>();
	Scalar cy=file.read<Misc::Float64>();
	Scalar fx=file.read<Misc::Float64>();
	Scalar fy=file.read<Misc::Float64>();
	Scalar sk=file.read<Misc::Float64>();
	
	/* Construct the projection matrix and its inverse: */
	m(0,0)=fx;
	m(0,1)=sk;
	m(0,2)=cx;
	m(1,1)=fy;
	m(1,2)=cy;
	Scalar fxfy=fx*fy;
	mInv(0,0)=Scalar(1)/fx;
	mInv(0,1)=-sk/fxfy;
	mInv(0,2)=-cx/fx+cy*sk/fxfy;
	mInv(1,1)=Scalar(1)/fy;
	mInv(1,2)=-cy/fy;
	
	/* Read the lens distortion coefficients: */
	LensDistortion::ParameterVector parv;
	for(int i=0;i<2;++i)
		parv[i]=file.read<Misc::Float64>();
	int numKappas=file.read<Misc::SInt32>();
	if(numKappas!=LensDistortion::numKappas)
		throw std::runtime_error("Video::IntrinsicParameters::read: Mismatching number of radial lens distortion parameters");
	for(int i=0;i<LensDistortion::numKappas;++i)
		parv[2+i]=file.read<Misc::Float64>();
	int numRhos=file.read<Misc::SInt32>();
	if(numRhos!=LensDistortion::numRhos)
		throw std::runtime_error("Video::IntrinsicParameters::read: Mismatching number of tangential lens distortion parameters");
	for(int i=0;i<LensDistortion::numRhos;++i)
		parv[2+LensDistortion::numKappas+i]=file.read<Misc::Float64>();
	
	ld.setParameterVector(parv);
	
	#if 0
	// DEBUGGING
	std::cout<<"Image size: "<<imageSize[0]<<", "<<imageSize[1]<<std::endl;
	std::cout<<"Camera projection: fx="<<fx<<", fy="<<fy<<", cx="<<cx<<", cy="<<cy<<", sk="<<sk<<std::endl;
	std::cout<<"LD principal point: "<<parv[0]<<", "<<parv[1]<<std::endl;
	std::cout<<"Radial parameters: "<<parv[2+0]<<", "<<parv[2+1]<<", "<<parv[2+2]<<std::endl;
	std::cout<<"Tangential parameters: "<<parv[2+LensDistortion::numKappas+0]<<", "<<parv[2+LensDistortion::numKappas+1]<<std::endl;
	#endif
	}

void IntrinsicParameters::write(IO::File& file) const
	{
	/* Write the video frame size: */
	for(int i=0;i<2;++i)
		file.write(Misc::UInt32(imageSize[i]));
	
	/* Write the camera projection matrix: */
	file.write(Misc::Float64(m(0,2)));
	file.write(Misc::Float64(m(1,2)));
	file.write(Misc::Float64(m(0,0)));
	file.write(Misc::Float64(m(1,1)));
	file.write(Misc::Float64(m(0,1)));
	
	/* Write the lens distortion coefficients: */
	LensDistortion::ParameterVector parv=ld.getParameterVector();
	for(int i=0;i<2;++i)
		file.write(Misc::Float64(parv[i]));
	file.write(Misc::SInt32(LensDistortion::numKappas));
	for(int i=0;i<LensDistortion::numKappas;++i)
		file.write(Misc::Float64(parv[2+i]));
	file.write(Misc::SInt32(LensDistortion::numRhos));
	for(int i=0;i<LensDistortion::numRhos;++i)
		file.write(Misc::Float64(parv[2+LensDistortion::numKappas+i]));
	}

}
