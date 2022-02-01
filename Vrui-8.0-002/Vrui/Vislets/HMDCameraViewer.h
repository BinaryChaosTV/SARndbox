/***********************************************************************
HMDCameraViewer - Vislet class to show a live pass-through video feed
from a mono or stereo camera attached to a head-mounted display.
Copyright (c) 2020-2021 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_VISLETS_HMDCAMERAVIEWER_INCLUDED
#define VRUI_VISLETS_HMDCAMERAVIEWER_INCLUDED

#include <string>
#include <Misc/RingBuffer.h>
#include <Realtime/TimeStamp.h>
#include <Threads/Spinlock.h>
#include <Threads/MutexCond.h>
#include <Threads/Thread.h>
#include <Threads/TripleBuffer.h>
#include <Geometry/Rotation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Images/BaseImage.h>
#include <Video/VideoDataFormat.h>
#include <Video/IntrinsicParameters.h>
#include <Vrui/Geometry.h>
#include <Vrui/Vislet.h>

/* Forward declarations: */
namespace Video {
class FrameBuffer;
class VideoDevice;
class ImageExtractor;
}
namespace Vrui {
class Viewer;
class VisletManager;
}

namespace Vrui {

namespace Vislets {

class HMDCameraViewer;

class HMDCameraViewerFactory:public VisletFactory
	{
	friend class HMDCameraViewer;
	
	/* Elements: */
	private:
	
	/* Configuration options: */
	Viewer* viewer; // The viewer representing the head-mounted display to which the camera is mounted
	std::string videoDeviceName; // Name of the camera's video device
	unsigned int videoDeviceIndex; // Index of the camera's video device among devices of the same name
	Video::VideoDataFormat videoFormat; // Camera's video format
	bool stereo; // Flag whether the camera's frames are stereo pairs
	unsigned int subFrames[2][4]; // Positions (origin and size) of mono or left and right sub-frames
	std::string intrinsicsNames[2]; // Names of mono or left and right intrinsic camera parameter files, relative to Vrui's Resource directory
	Rotation extrinsics; // Rotation from normalized camera space (looking along -z axis) to head tracker space
	Video::IntrinsicParameters::Scalar sphereRadius; // Radius of the video projection sphere in physical-space units
	Realtime::TimeStamp cameraLatency; // Estimated time from exposure to delivery of a new video frame
	
	/* Constructors and destructors: */
	public:
	HMDCameraViewerFactory(VisletManager& visletManager);
	virtual ~HMDCameraViewerFactory(void);
	
	/* Methods: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class HMDCameraViewer:public Vislet,public GLObject
	{
	friend class HMDCameraViewerFactory;
	
	/* Embedded classes: */
	private:
	typedef Video::IntrinsicParameters::Scalar Scalar;
	typedef Video::IntrinsicParameters::ImagePoint ImagePoint;
	typedef Video::IntrinsicParameters::Point Point;
	typedef Video::IntrinsicParameters::Vector Vector;
	
	struct Frame // Structure holding a video frame and its associated head transformation
		{
		/* Elements: */
		public:
		bool valid; // Flag if the frame is valid (has a video frame and a head orientation)
		Images::BaseImage frame; // The captured video frame
		Rotation headOrientation; // Orientation of the HMD in physical space at the time the frame was captured
		};
	
	struct OrientationSample // Structure for a head orientation sample
		{
		/* Elements: */
		public:
		Rotation orientation; // Sampled head orientation
		Realtime::TimeStamp timeStamp; // Time at which the orientation was sampled
		};
	
	struct DataItem:public GLObject::DataItem // Structure to store per-context OpenGL state
		{
		/* Elements: */
		public:
		bool haveNpotdt; // Flag whether OpenGL supports non-power-of-two dimension textures
		GLuint videoTextureId; // ID of image texture object
		GLfloat texMin[2][2],texMax[2][2]; // Texture coordinate rectangle to render the image texture for the left and right eye, respectively (to account for power-of-two only textures)
		unsigned int videoTextureVersion; // Version number of the video frame in the video texture
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	static HMDCameraViewerFactory* factory; // Pointer to the factory object for this class
	
	/* Video device streaming state: */
	volatile bool runStreamingThread; // Flag to keep the background video capture thread running
	Threads::Thread streamingThread; // Background thread handling video capture from the camera device
	Video::VideoDevice* videoDevice; // Pointer to the video recording device
	Video::ImageExtractor* videoExtractor; // Helper object to convert video frames to RGB
	Threads::MutexCond activationCond; // Condition variable to signal activation of the vislet to the background thread
	Threads::TripleBuffer<Frame> videoFrames; // Triple buffer to pass video frames from the video callback to the main loop
	unsigned int videoFrameVersion; // Version number of the most recent video frame in the triple buffer
	
	/* Video display state: */
	Video::IntrinsicParameters intrinsics[2]; // Set of intrinsic camera parameters to apply to the left and right video streams
	Threads::Spinlock orientationsMutex; // Mutex protecting the head orientation ring buffer
	Misc::RingBuffer<OrientationSample> orientationSamples; // Ring buffer of recent head orientations
	
	/* Private methods: */
	void* streamingThreadMethod(void); // Method running the video capture thread
	Point projectImagePoint(int eyeIndex,const ImagePoint& ip) const // Projects an image point into display space
		{
		/* Transform the image point to 3D direction space: */
		Vector d=intrinsics[eyeIndex].unproject(ip);
		
		/* Return the scaled direction vector: */
		return Point::origin+d*(factory->sphereRadius/d.mag());
		}
	
	/* Constructors and destructors: */
	public:
	HMDCameraViewer(int numArguments,const char* const arguments[]);
	virtual ~HMDCameraViewer(void);
	
	/* Methods from class Vislet: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void enable(bool startup);
	virtual void disable(bool shutdown);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

}

}

#endif
