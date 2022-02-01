/***********************************************************************
SceneGraphViewerWalkNavigationTool - Navigation tool to walk and
teleport through a scene graph.
Copyright (c) 2020-2021 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPHVIEWERWALKNAVIGATIONTOOL_INCLUDED
#define SCENEGRAPHVIEWERWALKNAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Plane.h>
#include <GL/GLPolylineTube.h>
#include <Vrui/Geometry.h>
#include <Vrui/SurfaceNavigationTool.h>

#include "SceneGraphViewer.h"

/* Forward declarations: */
class GLContextData;

/****************
Embedded classes:
****************/

class SceneGraphViewer::WalkNavigationToolFactory:public Vrui::ToolFactory
	{
	friend class WalkNavigationTool;
	
	/* Embedded classes: */
	private:
	typedef Vrui::Scalar Scalar;
	typedef Vrui::Point Point;
	typedef Vrui::Vector Vector;
	typedef Vrui::Rotation Rotation;
	typedef Vrui::NavTransform NavTransform;
	
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		Scalar fallAcceleration; // Acceleration when falling in physical space units per second^2, defaults to g
		Scalar probeSize; // Size of probe to use when aligning surface frames
		Scalar maxClimb; // Maximum amount of climb per frame
		Scalar orbRadius; // Radius of teleportation orb in physical space units
		Scalar orbVelocity; // Initial velocity of teleportation orb in physical space units per second
		bool startActive; // Flag whether a new navigation tool activates itself immediately after creation
		
		/* Constructors and destructors: */
		Configuration(void); // Creates default configuration
		
		/* Methods: */
		void read(const Misc::ConfigurationFileSection& cfs); // Overrides configuration from configuration file section
		void write(Misc::ConfigurationFileSection& cfs) const; // Writes configuration to configuration file section
		};
	
	/* Elements: */
	Configuration configuration; // Default configuration for all tools
	
	/* Constructors and destructors: */
	public:
	WalkNavigationToolFactory(Vrui::ToolManager& toolManager);
	virtual ~WalkNavigationToolFactory(void);
	
	/* Methods from Vrui::ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Vrui::Tool* createTool(const Vrui::ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Vrui::Tool* tool) const;
	};

class SceneGraphViewer::WalkNavigationTool:public Vrui::SurfaceNavigationTool
	{
	friend class WalkNavigationToolFactory;
	
	/* Embedded classes: */
	private:
	typedef Vrui::Scalar Scalar;
	typedef Vrui::Point Point;
	typedef Vrui::Vector Vector;
	typedef Vrui::Rotation Rotation;
	typedef Vrui::NavTransform NavTransform;
	
	/* Elements: */
	private:
	static WalkNavigationToolFactory* factory; // Pointer to the factory object for this class
	WalkNavigationToolFactory::Configuration configuration; // Private configuration of this tool
	
	/* Transient navigation state: */
	Point footPos; // Position of the main viewer's foot on the last frame
	Scalar headHeight; // Height of viewer's head above the foot point
	NavTransform surfaceFrame; // Current local coordinate frame aligned to the surface in navigation coordinates
	Scalar azimuth; // Current azimuth of view relative to local coordinate frame
	Scalar elevation; // Current elevation of view relative to local coordinate frame
	bool teleport; // Flag whether the teleportation orb is active
	GLPolylineTube orbPath; // Teleportation orb's current path through the environment
	bool orbValid; // Flag whether the teleportation orb is in a valid position
	Scalar fallVelocity; // Current falling velocity while airborne in units per second^2
	
	/* Private methods: */
	void applyNavState(void) const; // Sets the navigation transformation based on the tool's current navigation state
	void initNavState(void); // Initializes the tool's navigation state when it is activated
	
	/* Constructors and destructors: */
	public:
	WalkNavigationTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const Vrui::ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

#endif
