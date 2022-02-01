/***********************************************************************
BackgroundViewer - Utility to view (and edit) a 3D camera's background
removal frame.
Copyright (c) 2018 Oliver Kreylos

This file is part of the Kinect 3D Video Capture Project (Kinect).

The Kinect 3D Video Capture Project is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Kinect 3D Video Capture Project is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Kinect 3D Video Capture Project; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <string.h>
#include <string>
#include <iostream>
#include <Misc/SizedTypes.h>
#include <Threads/Thread.h>
#include <Threads/MutexCond.h>
#include <Threads/TripleBuffer.h>
#include <IO/File.h>
#include <IO/SeekableFile.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Geometry/ProjectiveTransformation.h>
#include <GL/gl.h>
#include <GL/GLModels.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>
#include <Vrui/ToolManager.h>
#include <Kinect/Config.h>
#include <Kinect/Internal/Config.h>
#include <Kinect/FrameBuffer.h>
#include <Kinect/MeshBuffer.h>
#include <Kinect/LensDistortion.h>
#include <Kinect/FrameSource.h>
#include <Kinect/ProjectorHeader.h>

class BackgroundViewer:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef Kinect::FrameSource::DepthPixel DepthPixel;
	typedef Kinect::ProjectorType::Point Point;
	typedef Point::Vector Vector;
	typedef Point::Scalar Scalar;
	
	class PaintTool; // Forward declaration
	typedef Vrui::GenericToolFactory<PaintTool> PaintToolFactory; // Painting tool class uses the generic factory class
	
	class PaintTool:public Vrui::Tool,public Vrui::Application::Tool<BackgroundViewer> // Tool class to paint background images
		{
		friend class Vrui::GenericToolFactory<PaintTool>;
		
		/* Elements: */
		private:
		static PaintToolFactory* factory; // Pointer to the factory object for this class
		bool active; // Flag if the tool is currently painting
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the painting tool's factory class
		PaintTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	struct Sphere
		{
		/* Elements: */
		public:
		Point center;
		Scalar radius;
		};
	
	/* Elements: */
	unsigned int backgroundSize[2]; // Width and height of background frame
	Kinect::FrameBuffer background; // Frame buffer holding the background frame
	Kinect::ProjectorType projector; // A projector to convert the background frame into a 3D model
	Kinect::MeshBuffer backgroundMesh; // Mesh buffer holding the reconstructed background frame
	Threads::TripleBuffer<Sphere> editRequest; // Triple buffer holding the effect volume of the most recent editing operation
	Threads::MutexCond editRequestCond; // Condition variable to signal a new editing request
	volatile bool shutdownEditingThread; // Signal to shut down the editing thread
	Threads::Thread editingThread; // Background thread to edit the background frame
	
	/* Private methods: */
	void* editingThreadMethod(void); // Method running in the background editing thread
	
	/* Constructors and destructors: */
	public:
	BackgroundViewer(int& argc,char**& argv);
	virtual ~BackgroundViewer(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

/****************************************************
Static elements of class BackgroundViewer::PaintTool:
****************************************************/

BackgroundViewer::PaintToolFactory* BackgroundViewer::PaintTool::factory=0;

/********************************************
Methods of class BackgroundViewer::PaintTool:
********************************************/

void BackgroundViewer::PaintTool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new PaintToolFactory("PaintTool","Paint Background Image",0,*Vrui::getToolManager());
	
	/* Set the custom tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Paint");
	
	/* Register the custom tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

BackgroundViewer::PaintTool::PaintTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 active(false)
	{
	}

const Vrui::ToolFactory* BackgroundViewer::PaintTool::getFactory(void) const
	{
	return factory;
	}

void BackgroundViewer::PaintTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Activate or deactivate the tool: */
	active=cbData->newButtonState;
	}

void BackgroundViewer::PaintTool::frame(void)
	{
	if(active)
		{
		/* Calculate the center and radius of the tool's effect volume into world space: */
		Sphere sphere;
		sphere.center=Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0));
		sphere.radius=Vrui::getInverseNavigationTransformation().getScaling()*Vrui::getUiSize()*Vrui::Scalar(5);
		
		/* Post an edit request to the editing thread: */
		{
		Threads::MutexCond::Lock editRequestLock(application->editRequestCond);
		application->editRequest.postNewValue(sphere);
		application->editRequestCond.signal();
		}
		}
	}

void BackgroundViewer::PaintTool::display(GLContextData& contextData) const
	{
	/* Draw the effect volume: */
	glPushMatrix();
	glMultMatrix(getButtonDeviceTransformation(0));
	
	glDrawSphereIcosahedron(Vrui::getUiSize()*Vrui::Scalar(5),5);
	
	glPopMatrix();
	}

/*********************************
Methods of class BackgroundViewer:
*********************************/

void* BackgroundViewer::editingThreadMethod(void)
	{
	while(true)
		{
		/* Wait for a new editing request: */
		bool haveEdit;
		{
		Threads::MutexCond::Lock editRequestLock(editRequestCond);
		editRequestCond.wait(editRequestLock);
		haveEdit=editRequest.lockNewValue();
		}
		
		/* Check if it's time to quit: */
		if(shutdownEditingThread)
			break;
		
		/* If there's no edit request, go back to sleep: */
		if(!haveEdit)
			continue;
		
		/* Process the edit request: */
		Point center=editRequest.getLockedValue().center;
		Scalar radius=editRequest.getLockedValue().radius;
		
		/* Check if any background frame pixels are projected into the tool's effect volume: */
		bool edited=false;
		DepthPixel* bPtr=background.getData<DepthPixel>();
		for(unsigned int y=0;y<backgroundSize[1];++y)
			for(unsigned int x=0;x<backgroundSize[0];++x,++bPtr)
				{
				/* Unproject the pixel into world space: */
				Point p=projector.unprojectPixel(x,y);
				
				/* Check if the pixel is inside the effect volume: */
				if(Geometry::sqrDist(p,center)<=Math::sqr(radius))
					{
					/* Intersect a ray from the projector's focal point through the unprojected pixel with the effect volume: */
					Point start=Point::origin;
					Vector dir=p-start;
					
					Scalar d2=Geometry::sqr(dir);
					Vector oc=start-center;
					Scalar ph=(oc*dir);
					Scalar det=Math::sqr(ph)-(Geometry::sqr(oc)-Math::sqr(radius))*d2;
					if(det>=Scalar(0)) // Should always be the case, but better to test
						{
						det=Math::sqrt(det);
						Scalar lambda=(-ph+det)/d2; // Second intersection, where ray leaves the sphere
						
						/* Project the ray's exit point into the depth image to get its adjusted depth: */
						Point newDepth=projector.projectPoint(start+dir*lambda);
						*bPtr=DepthPixel(Math::floor(newDepth[2]+Scalar(0.5)));
						
						edited=true;
						}
					}
				}
		
		if(edited)
			{
			/* Update the background frame: */
			projector.processDepthFrame(background,backgroundMesh);
			projector.setMesh(background,backgroundMesh);
			Vrui::requestUpdate();
			}
		}
	
	return 0;
	}

BackgroundViewer::BackgroundViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv)
	{
	/* Parse the command line: */
	const char* backgroundFileName=0; // Name of the background frame file to load
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			std::cerr<<"Ignoring unrecognized command line parameter "<<argv[i]<<std::endl;
			}
		else if(backgroundFileName==0)
			backgroundFileName=argv[i];
		else
			std::cerr<<"Ignoring command line argument "<<argv[i]<<std::endl;
		}
	if(backgroundFileName==0)
		{
		std::cerr<<"No background file name provided; exiting"<<std::endl;
		Vrui::shutdown();
		return;
		}
	
	try
		{
		/* Open the background file: */
		IO::FilePtr backgroundFile=IO::openFile(backgroundFileName);
		backgroundFile->setEndianness(Misc::LittleEndian);
		
		/* Read the file header: */
		Misc::UInt32 fileSize[2];
		backgroundFile->read(fileSize,2);
		for(int i=0;i<2;++i)
			backgroundSize[i]=fileSize[i];
		
		/* Initialize the background frame buffer: */
		background=Kinect::FrameBuffer(fileSize[0],fileSize[1],size_t(fileSize[1])*size_t(fileSize[0])*sizeof(DepthPixel));
		DepthPixel* backgroundPixels=background.getData<DepthPixel>();
		
		/* Read the background frame contents: */
		backgroundFile->read(backgroundPixels,size_t(fileSize[1])*size_t(fileSize[0]));
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"Caught exception "<<err.what()<<" while reading background file "<<backgroundFileName<<"; exiting"<<std::endl;
		Vrui::shutdown();
		return;
		}
	
	/* Extract the camera serial number from the background file name: */
	std::string serialNumber;
	const char* fileNameStart=backgroundFileName;
	for(const char* bfnPtr=backgroundFileName;*bfnPtr!='\0';++bfnPtr)
		if(*bfnPtr=='/')
			fileNameStart=bfnPtr+1;
	if(strncmp(fileNameStart,"Background-",11)==0)
		{
		const char* serialStart=fileNameStart+11;
		const char* serialEnd;
		for(serialEnd=serialStart;*serialEnd!='\0'&&*serialEnd!='.';++serialEnd)
			;
		if(serialEnd!=serialStart&&strcmp(serialEnd,".background")==0)
			serialNumber=std::string(serialStart,serialEnd);
		}
	
	/* Initialize the depth frame projector: */
	projector.setDepthFrameSize(backgroundSize);
	
	/* Load a depth correction file if there is one matching the background file's serial number: */
	if(!serialNumber.empty())
		{
		try
			{
			/* Construct the depth correction file name: */
			std::string depthCorrectionFileName=std::string(backgroundFileName,fileNameStart);
			depthCorrectionFileName.append("DepthCorrection-");
			depthCorrectionFileName.append(serialNumber);
			depthCorrectionFileName.append(".dat");
			
			/* Load the depth correction file: */
			IO::FilePtr depthCorrectionFile=IO::openFile(depthCorrectionFileName.c_str());
			depthCorrectionFile->setEndianness(Misc::LittleEndian);
			Kinect::FrameSource::DepthCorrection dc(*depthCorrectionFile);
			
			/* Set the projector's per-pixel depth correction: */
			projector.setDepthCorrection(&dc);
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<"Unable to load per-pixel depth correction due to exception "<<err.what()<<std::endl;
			}
		}
	
	/* Load an intrinsic parameter file if there is one matching the background file's serial number: */
	if(!serialNumber.empty())
		{
		try
			{
			/* Construct the intrinsic parameter file name: */
			std::string intrinsicParameterFileName=std::string(backgroundFileName,fileNameStart);
			intrinsicParameterFileName.append("IntrinsicParameters-");
			intrinsicParameterFileName.append(serialNumber);
			intrinsicParameterFileName.append(".dat");
			
			/* Load the intrinsic parameter file: */
			IO::SeekableFilePtr intrinsicParameterFile=IO::openSeekableFile(intrinsicParameterFileName.c_str());
			intrinsicParameterFile->setEndianness(Misc::LittleEndian);
			
			Kinect::FrameSource::IntrinsicParameters ips;
			if(intrinsicParameterFile->getSize()==296)
				{
				/* Read depth lens distortion correction parameters: */
				Misc::Float64 depthLensDistortionParameters[5];
				intrinsicParameterFile->read(depthLensDistortionParameters,5);
				for(int i=0;i<3;++i)
					ips.depthLensDistortion.setKappa(i,depthLensDistortionParameters[i]);
				for(int i=0;i<2;++i)
					ips.depthLensDistortion.setRho(i,depthLensDistortionParameters[3+i]);
				}
			
			/* Read depth and color projection matrices: */
			Misc::Float64 depthMatrix[16];
			intrinsicParameterFile->read(depthMatrix,4*4);
			ips.depthProjection=Kinect::FrameSource::IntrinsicParameters::PTransform::fromRowMajor(depthMatrix);
			Misc::Float64 colorMatrix[16];
			intrinsicParameterFile->read(colorMatrix,4*4);
			ips.colorProjection=Kinect::FrameSource::IntrinsicParameters::PTransform::fromRowMajor(colorMatrix);
			
			/* Set projection parameters for the lens distortion corrector: */
			ips.depthLensDistortion.setProjection(ips.depthProjection);
			
			/* Set the projector's intrinsic parameters: */
			projector.setIntrinsicParameters(ips);
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<"Unable to load projector intrinsic parameters due to exception "<<err.what()<<std::endl;
			}
		}
	
	/* Set projection parameters: */
	projector.setMapTexture(false);
	projector.setIlluminate(true);
	projector.setTriangleDepthRange(2047);
	
	/* Let the projector process the background frame: */
	projector.processDepthFrame(background,backgroundMesh);
	projector.setMesh(background,backgroundMesh);
	
	/* Initialize the custom tool class: */
	PaintTool::initClass();
	
	/* Start the background editing thread: */
	shutdownEditingThread=false;
	editingThread.start(this,&BackgroundViewer::editingThreadMethod);
	}

BackgroundViewer::~BackgroundViewer(void)
	{
	/* Shut down the editing thread: */
	{
	Threads::MutexCond::Lock editRequestLock(editRequestCond);
	shutdownEditingThread=true;
	editRequestCond.signal();
	}
	editingThread.join();
	
	/* Save the edited background frame to a new file: */
	IO::FilePtr newBackgroundFile=IO::openFile("Background.background",IO::File::WriteOnly);
	newBackgroundFile->setEndianness(Misc::LittleEndian);
	for(int i=0;i<2;++i)
		newBackgroundFile->write<Misc::UInt32>(backgroundSize[i]);
	newBackgroundFile->write(background.getData<DepthPixel>(),size_t(backgroundSize[1])*size_t(backgroundSize[0]));
	}

void BackgroundViewer::frame(void)
	{
	/* Notify the projector of any changes to the background frame: */
	projector.updateFrames();
	}

void BackgroundViewer::display(GLContextData& contextData) const
	{
	/* Let the projector render the reconstructed background frame: */
	projector.glRenderAction(contextData);
	}

void BackgroundViewer::resetNavigation(void)
	{
	}

VRUI_APPLICATION_RUN(BackgroundViewer)
