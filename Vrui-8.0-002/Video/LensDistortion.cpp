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

#include <Video/LensDistortion.h>

namespace Video {

/*******************************
Methods of class LensDistortion:
*******************************/

LensDistortion::Scalar LensDistortion::calcMaxR2(void) const
	{
	/* Step through valid values of r until corrected radius decreases with an increase in undistorted radius: */
	Scalar maxR=Math::sqrt(Math::sqr(center[0])+Math::sqr(center[1]));
	Scalar lastCorrectedR(0);
	for(Scalar r=Scalar(1);r<maxR;r+=Scalar(1))
		{
		/* Calculate the radial correction coefficient: */
		Scalar r2=r*r;
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
		
		/* Calculate the corrected radius and bail out if it got smaller than the last corrected radius: */
		Scalar correctedR=r*radial;
		if(correctedR<=lastCorrectedR)
			maxR=r-1;
		lastCorrectedR=correctedR;
		}
	
	return Math::sqr(maxR);
	}

LensDistortion::Point LensDistortion::undistort(const LensDistortion::Point& distorted) const
	{
	/*******************************************************************
	Must run two-dimensional Newton-Raphson iteration to invert the full
	distortion correction formula.
	*******************************************************************/
	
	/* Start iteration at the distorted point: */
	Point p=distorted;
	for(int iteration=0;iteration<20;++iteration) // 20 is a ridiculous number
		{
		/* Calculate the radial correction coefficient: */
		Vector d=p-center;
		Scalar r2=d.sqr();
		if(r2>maxR2)
			{
			/* Bring the current point back into the fold: */
			d*=Math::sqrt(maxR2/r2);
			p=center+d;
			r2=maxR2;
			}
		
		#if VIDEO_LENSDISTORTION_RATIONALRADIAL
		Scalar radialn(0);
		for(int i=numNumeratorKappas-1;i>=0;--i)
			radialn=(radialn+kappas[i])*r2;
		radialn+=Scalar(1);
		Scalar radiald(0);
		for(int i=numKappas-1;i>=numNumeratorKappas;--i)
			radiald=(radiald+kappas[i])*r2;
		radiald+=Scalar(1);
		Scalar radial=radialn/radiald;
		#else
		Scalar radial(0);
		for(int i=numKappas-1;i>=0;--i)
			radial=(radial+kappas[i])*r2;
		radial+=Scalar(1);
		#if VIDEO_LENSDISTORTION_INVERSERADIAL
		radial=Scalar(1)/radial;
		#endif
		#endif
		
		/* Calculate the function value at p: */
		Point fp;
		fp[0]=center[0]+d[0]*radial+Scalar(2)*rhos[0]*d[0]*d[1]+rhos[1]*(r2+Scalar(2)*d[0]*d[0])-distorted[0];
		fp[1]=center[1]+d[1]*radial+rhos[0]*(r2+Scalar(2)*d[1]*d[1])+Scalar(2)*rhos[1]*d[0]*d[1]-distorted[1];
		
		/* Bail out if close enough: */
		if(fp.sqr()<Scalar(1.0e-32)) // 1e-32 is a ridiculous number
			break;
		
		/* Calculate the derivative of the radial correction coefficient: */
		#if VIDEO_LENSDISTORTION_RATIONALRADIAL
		Scalar dRadialn(0);
		for(int i=numNumeratorKappas-1;i>=0;--i)
			dRadialn=dRadialn*r2+Scalar(i+1)*kappas[i];
		Scalar dRadiald(0);
		for(int i=numKappas-1;i>=numNumeratorKappas;--i)
			dRadiald=dRadiald*r2+Scalar(i-numNumeratorKappas+1)*kappas[i];
		Scalar dRadial=(dRadialn*radiald-radialn*dRadiald)/Math::sqr(radiald);
		#else
		Scalar dRadial(0);
		for(int i=numKappas-1;i>=0;--i)
			dRadial=dRadial*r2+Scalar(i+1)*kappas[i];
		#if VIDEO_LENSDISTORTION_INVERSERADIAL
		dRadial=-dRadial*Math::sqr(radial);
		#endif
		#endif
		
		/* Calculate the function derivative at p: */
		dRadial*=Scalar(2);
		Scalar fpd[2][2];
		fpd[0][0]=radial+d[0]*dRadial*d[0]+Scalar(2)*rhos[0]*d[1]+Scalar(6)*rhos[1]*d[0]; // d fp[0] / d p[0]
		fpd[0][1]=d[0]*dRadial*d[1]+Scalar(2)*rhos[0]*d[0]+Scalar(2)*rhos[1]*d[1]; // d fp[0] / d p[1]
		fpd[1][0]=d[1]*dRadial*d[0]+Scalar(2)*rhos[0]*d[0]+Scalar(2)*rhos[1]*d[1]; // d fp[1] / d p[0]
		fpd[1][1]=radial+d[1]*dRadial*d[1]+Scalar(2)*rhos[1]*d[0]+Scalar(6)*rhos[0]*d[1]; // d fp[0] / d p[0]
		
		/* Perform the Newton-Raphson step: */
		Scalar det=fpd[0][0]*fpd[1][1]-fpd[0][1]*fpd[1][0];
		p[0]-=(fpd[1][1]*fp[0]-fpd[0][1]*fp[1])/det;
		p[1]-=(fpd[0][0]*fp[1]-fpd[1][0]*fp[0])/det;
		}
	
	return p;
	}

}
