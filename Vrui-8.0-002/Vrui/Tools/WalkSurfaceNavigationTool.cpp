/***********************************************************************
WalkSurfaceNavigationTool - Version of the WalkNavigationTool that lets
a user navigate along an application-defined surface.
Copyright (c) 2009-2020 Oliver Kreylos

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

#include <Vrui/Tools/WalkSurfaceNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLValueCoders.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/****************************************************************
Methods of class WalkSurfaceNavigationToolFactory::Configuration:
****************************************************************/

WalkSurfaceNavigationToolFactory::Configuration::Configuration(void)
	:centerOnActivation(false),
	 centerPoint(getDisplayCenter()),
	 moveSpeed(getDisplaySize()),
	 innerRadius(getDisplaySize()*Scalar(0.5)),outerRadius(getDisplaySize()*Scalar(0.75)),
	 centerViewDirection(getForwardDirection()),
	 rotateSpeed(Math::rad(Scalar(120))),
	 innerAngle(Math::rad(Scalar(30))),outerAngle(Math::rad(Scalar(120))),
	 fallAcceleration(getMeterFactor()*Scalar(9.81)),
	 jetpackAcceleration(fallAcceleration*Scalar(1.5)),
	 probeSize(getInchFactor()*Scalar(12)),
	 maxClimb(getInchFactor()*Scalar(12)),
	 fixAzimuth(false),
	 drawMovementCircles(true),
	 movementCircleColor(0.0f,1.0f,0.0f),
	 drawHud(true),
	 hudFontSize(getUiSize()*2.0f)
	{
	}

void WalkSurfaceNavigationToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	centerOnActivation=cfs.retrieveValue<bool>("./centerOnActivation",centerOnActivation);
	centerPoint=cfs.retrieveValue<Point>("./centerPoint",centerPoint);
	centerPoint=calcFloorPoint(centerPoint);
	moveSpeed=cfs.retrieveValue<Scalar>("./moveSpeed",moveSpeed);
	innerRadius=cfs.retrieveValue<Scalar>("./innerRadius",innerRadius);
	outerRadius=cfs.retrieveValue<Scalar>("./outerRadius",outerRadius);
	centerViewDirection=cfs.retrieveValue<Vector>("./centerViewDirection",centerViewDirection);
	centerViewDirection.orthogonalize(getUpDirection()).normalize();
	rotateSpeed=Math::rad(cfs.retrieveValue<Scalar>("./rotateSpeed",Math::deg(rotateSpeed)));
	innerAngle=Math::rad(cfs.retrieveValue<Scalar>("./innerAngle",Math::deg(innerAngle)));
	outerAngle=Math::rad(cfs.retrieveValue<Scalar>("./outerAngle",Math::deg(outerAngle)));
	fallAcceleration=cfs.retrieveValue<Scalar>("./fallAcceleration",fallAcceleration);
	jetpackAcceleration=cfs.retrieveValue<Scalar>("./jetpackAcceleration",fallAcceleration*Scalar(1.5));
	probeSize=cfs.retrieveValue<Scalar>("./probeSize",probeSize);
	maxClimb=cfs.retrieveValue<Scalar>("./maxClimb",maxClimb);
	fixAzimuth=cfs.retrieveValue<bool>("./fixAzimuth",fixAzimuth);
	drawMovementCircles=cfs.retrieveValue<bool>("./drawMovementCircles",drawMovementCircles);
	movementCircleColor=cfs.retrieveValue<Color>("./movementCircleColor",movementCircleColor);
	drawHud=cfs.retrieveValue<bool>("./drawHud",drawHud);
	hudFontSize=cfs.retrieveValue<float>("./hudFontSize",hudFontSize);
	}

void WalkSurfaceNavigationToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<bool>("./centerOnActivation",centerOnActivation);
	cfs.storeValue<Point>("./centerPoint",centerPoint);
	cfs.storeValue<Scalar>("./moveSpeed",moveSpeed);
	cfs.storeValue<Scalar>("./innerRadius",innerRadius);
	cfs.storeValue<Scalar>("./outerRadius",outerRadius);
	cfs.storeValue<Vector>("./centerViewDirection",centerViewDirection);
	cfs.storeValue<Scalar>("./rotateSpeed",Math::deg(rotateSpeed));
	cfs.storeValue<Scalar>("./innerAngle",Math::deg(innerAngle));
	cfs.storeValue<Scalar>("./outerAngle",Math::deg(outerAngle));
	cfs.storeValue<Scalar>("./fallAcceleration",fallAcceleration);
	cfs.storeValue<Scalar>("./jetpackAcceleration",fallAcceleration*Scalar(1.5));
	cfs.storeValue<Scalar>("./probeSize",probeSize);
	cfs.storeValue<Scalar>("./maxClimb",maxClimb);
	cfs.storeValue<bool>("./fixAzimuth",fixAzimuth);
	cfs.storeValue<bool>("./drawMovementCircles",drawMovementCircles);
	cfs.storeValue<Color>("./movementCircleColor",movementCircleColor);
	cfs.storeValue<bool>("./drawHud",drawHud);
	cfs.storeValue<float>("./hudFontSize",hudFontSize);
	}

/*************************************************
Methods of class WalkSurfaceNavigationToolFactory:
*************************************************/

WalkSurfaceNavigationToolFactory::WalkSurfaceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("WalkSurfaceNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	layout.setNumValuators(0,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	configuration.read(toolManager.getToolClassSection(getClassName()));
	
	/* Set tool class' factory pointer: */
	WalkSurfaceNavigationTool::factory=this;
	}

WalkSurfaceNavigationToolFactory::~WalkSurfaceNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	WalkSurfaceNavigationTool::factory=0;
	}

const char* WalkSurfaceNavigationToolFactory::getName(void) const
	{
	return "Walk";
	}

const char* WalkSurfaceNavigationToolFactory::getButtonFunction(int) const
	{
	return "Start / Stop";
	}

const char* WalkSurfaceNavigationToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Fire Jetpack";
		
		default:
			return "Unused";
		}
	}

Tool* WalkSurfaceNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new WalkSurfaceNavigationTool(this,inputAssignment);
	}

void WalkSurfaceNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveWalkSurfaceNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("SurfaceNavigationTool");
	}

extern "C" ToolFactory* createWalkSurfaceNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	WalkSurfaceNavigationToolFactory* walkSurfaceNavigationToolFactory=new WalkSurfaceNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return walkSurfaceNavigationToolFactory;
	}

extern "C" void destroyWalkSurfaceNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************************
Methods of class WalkSurfaceNavigationTool::DataItem:
****************************************************/

WalkSurfaceNavigationTool::DataItem::DataItem(void)
	{
	/* Create tools' model display list: */
	movementCircleListId=glGenLists(2);
	hudListId=movementCircleListId+1;
	}

WalkSurfaceNavigationTool::DataItem::~DataItem(void)
	{
	/* Destroy tools' model display list: */
	glDeleteLists(movementCircleListId,2);
	}

/**************************************************
Static elements of class WalkSurfaceNavigationTool:
**************************************************/

WalkSurfaceNavigationToolFactory* WalkSurfaceNavigationTool::factory=0;

/******************************************
Methods of class WalkSurfaceNavigationTool:
******************************************/

void WalkSurfaceNavigationTool::applyNavState(void) const
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotateAround(Point(0,0,headHeight),Rotation::rotateX(elevation));
	nav*=NavTransform::rotate(Rotation::rotateZ(azimuth));
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void WalkSurfaceNavigationTool::initNavState(void)
	{
	/* Calculate the main viewer's current head and foot positions: */
	Point headPos=getMainViewer()->getHeadPosition();
	footPos=calcFloorPoint(headPos);
	headHeight=Geometry::dist(headPos,footPos);
	
	/* Set up a physical navigation frame around the main viewer's current head position: */
	calcPhysicalFrame(headPos);
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	surfaceFrame=getInverseNavigationTransformation()*physicalFrame;
	NavTransform newSurfaceFrame=surfaceFrame;
	
	/* Reset the falling velocity: */
	fallVelocity=Scalar(0);
	
	/* Align the initial frame with the application's surface and calculate Euler angles: */
	AlignmentData ad(surfaceFrame,newSurfaceFrame,configuration.probeSize,configuration.maxClimb);
	Scalar roll;
	align(ad,azimuth,elevation,roll);
	
	/* Limit the elevation angle to the horizontal: */
	elevation=Scalar(0);
	
	/* If the initial surface frame was above the surface, lift it back up and start falling: */
	Scalar z=newSurfaceFrame.inverseTransform(surfaceFrame.getOrigin())[2];
	if(z>Scalar(0))
		{
		newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
		fallVelocity=-configuration.fallAcceleration*getFrameTime();
		}
	
	/* Move the physical frame to the foot position, and adjust the surface frame accordingly: */
	newSurfaceFrame*=Geometry::invert(physicalFrame)*NavTransform::translate(footPos-headPos)*physicalFrame;
	physicalFrame.leftMultiply(NavTransform::translate(footPos-headPos));
	
	/* Apply the initial navigation state: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

WalkSurfaceNavigationTool::WalkSurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(factory,inputAssignment),
	 configuration(WalkSurfaceNavigationTool::factory->configuration),
	 numberRenderer(configuration.hudFontSize,true),
	 centerPoint(configuration.centerPoint),
	 jetpack(0)
	{
	/* This object's GL state depends on the number renderer's GL state: */
	dependsOn(&numberRenderer);
	}

void WalkSurfaceNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	centerPoint=configuration.centerPoint;
	numberRenderer.setFont(configuration.hudFontSize,true);
	}

void WalkSurfaceNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

const ToolFactory* WalkSurfaceNavigationTool::getFactory(void) const
	{
	return factory;
	}

void WalkSurfaceNavigationTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Act depending on this tool's current state: */
		if(isActive())
			{
			/* Deactivate this tool: */
			deactivate();
			}
		else
			{
			/* Try activating this tool: */
			if(activate())
				{
				/* Store the center point for this navigation sequence: */
				if(configuration.centerOnActivation)
					centerPoint=calcFloorPoint(getMainViewer()->getHeadPosition());
				
				/* Initialize the navigation state: */
				initNavState();
				}
			}
		}
	}

void WalkSurfaceNavigationTool::valuatorCallback(int,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Update the jetpack acceleration value: */
	jetpack=Scalar(cbData->newValuatorValue)*configuration.jetpackAcceleration;
	}

void WalkSurfaceNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Calculate azimuth angle change based on the current viewing direction: */
		Vector viewDir=getMainViewer()->getViewDirection();
		viewDir-=getUpDirection()*((viewDir*getUpDirection())/Geometry::sqr(getUpDirection()));
		Scalar viewDir2=Geometry::sqr(viewDir);
		if(viewDir2!=Scalar(0))
			{
			/* Calculate the rotation speed: */
			Scalar viewAngleCos=(viewDir*configuration.centerViewDirection)/Math::sqrt(viewDir2);
			Scalar viewAngle;
			if(viewAngleCos>Scalar(1)-Math::Constants<Scalar>::epsilon)
				viewAngle=Scalar(0);
			else if(viewAngleCos<Scalar(-1)+Math::Constants<Scalar>::epsilon)
				viewAngle=Math::Constants<Scalar>::pi;
			else
				viewAngle=Math::acos(viewAngleCos);
			Scalar rotateSpeed=Scalar(0);
			if(viewAngle>=configuration.outerAngle)
				rotateSpeed=configuration.rotateSpeed;
			else if(viewAngle>configuration.innerAngle)
				rotateSpeed=configuration.rotateSpeed*(viewAngle-configuration.innerAngle)/(configuration.outerAngle-configuration.innerAngle);
			Vector x=configuration.centerViewDirection^getUpDirection();
			if(viewDir*x<Scalar(0))
				rotateSpeed=-rotateSpeed;
			
			/* Update the azimuth angle: */
			azimuth=wrapAngle(azimuth+rotateSpeed*getFrameTime());
			}
		
		/* Calculate the new head and foot positions: */
		Point newFootPos=calcFloorPoint(getMainViewer()->getHeadPosition());
		headHeight=Geometry::dist(getMainViewer()->getHeadPosition(),newFootPos);
		
		/* Create a physical navigation frame around the new foot position: */
		calcPhysicalFrame(newFootPos);
		
		/* Calculate the movement from walking: */
		Vector move=newFootPos-footPos;
		footPos=newFootPos;
		
		/* Calculate movement from virtual joystick: */
		Vector moveDir=footPos-centerPoint;
		Scalar moveDirLen=moveDir.mag();
		Scalar speed=Scalar(0);
		if(moveDirLen>=configuration.outerRadius)
			speed=configuration.moveSpeed;
		else if(moveDirLen>configuration.innerRadius)
			speed=configuration.moveSpeed*(moveDirLen-configuration.innerRadius)/(configuration.outerRadius-configuration.innerRadius);
		moveDir*=speed/moveDirLen;
		
		/* Add the current flying and falling velocities: */
		if(jetpack!=Scalar(0))
			moveDir+=getValuatorDeviceRayDirection(0)*jetpack;
		moveDir+=getUpDirection()*fallVelocity;
		
		/* Calculate the complete movement vector: */
		move+=moveDir*getFrameTime();
		
		/* Transform the movement vector from physical space to the physical navigation frame: */
		move=physicalFrame.inverseTransform(move);
		
		/* Rotate by the current azimuth angle: */
		move=Rotation::rotateZ(-azimuth).transform(move);
		
		/* Move the surface frame: */
		NavTransform newSurfaceFrame=surfaceFrame;
		newSurfaceFrame*=NavTransform::translate(move);
		
		/* Re-align the surface frame with the surface: */
		Point initialOrigin=newSurfaceFrame.getOrigin();
		Rotation initialOrientation=newSurfaceFrame.getRotation();
		AlignmentData ad(surfaceFrame,newSurfaceFrame,configuration.probeSize,configuration.maxClimb);
		align(ad);
		
		if(!configuration.fixAzimuth)
			{
			/* Have the azimuth angle track changes in the surface frame's rotation: */
			Rotation rot=Geometry::invert(initialOrientation)*newSurfaceFrame.getRotation();
			rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
			Vector x=rot.getDirection(0);
			azimuth=wrapAngle(azimuth+Math::atan2(x[1],x[0]));
			}
		
		/* Check if the initial surface frame is above the surface: */
		Scalar z=newSurfaceFrame.inverseTransform(initialOrigin)[2];
		if(z>Scalar(0))
			{
			/* Lift the aligned frame back up to the original altitude and continue flying: */
			newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
			fallVelocity-=configuration.fallAcceleration*getFrameTime();
			}
		else
			{
			/* Stop falling: */
			fallVelocity=Scalar(0);
			}
		
		/* Apply the newly aligned surface frame: */
		surfaceFrame=newSurfaceFrame;
		applyNavState();
		
		if(speed!=Scalar(0)||z>Scalar(0)||jetpack!=Scalar(0))
			{
			/* Request another frame: */
			scheduleUpdate(getNextAnimationTime());
			}
		}
	}

void WalkSurfaceNavigationTool::display(GLContextData& contextData) const
	{
	/* Get a pointer to the context data item and set up OpenGL state: */
	DataItem* dataItem=0;
	if(configuration.drawMovementCircles||(configuration.drawHud&&isActive()))
		{
		dataItem=contextData.retrieveDataItem<DataItem>(this);
		
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		}
	
	if(configuration.drawMovementCircles)
		{
		/* Translate to the center point: */
		glPushMatrix();
		glTranslate(centerPoint-Point::origin);
		
		/* Execute the movement circle display list: */
		glCallList(dataItem->movementCircleListId);
		
		glPopMatrix();
		}
	
	if(configuration.drawHud&&isActive())
		{
		/* Translate to the HUD's center point: */
		glPushMatrix();
		glMultMatrix(physicalFrame);
		glTranslate(0,0,headHeight);
		
		/* Rotate by the azimuth angle: */
		glRotate(Math::deg(azimuth),0,0,1);
		
		/* Execute the HUD display list: */
		glCallList(dataItem->hudListId);
		
		glPopMatrix();
		}
	
	/* Reset OpenGL state: */
	if(configuration.drawMovementCircles||(configuration.drawHud&&isActive()))
		glPopAttrib();
	}

void WalkSurfaceNavigationTool::initContext(GLContextData& contextData) const
	{
	DataItem* dataItem=0;
	if(configuration.drawMovementCircles||configuration.drawHud)
		{
		/* Create a new data item: */
		dataItem=new DataItem;
		contextData.addDataItem(this,dataItem);
		}
		
	if(configuration.drawMovementCircles)
		{
		/* Create the movement circle display list: */
		glNewList(dataItem->movementCircleListId,GL_COMPILE);
		
		/* Create a coordinate system for the floor plane: */
		Vector y=configuration.centerViewDirection;
		Vector x=y^getFloorPlane().getNormal();
		x.normalize();
		
		/* Draw the inner circle: */
		glColor(configuration.movementCircleColor);
		glBegin(GL_LINE_LOOP);
		for(int i=0;i<64;++i)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(64);
			glVertex(Point::origin-x*(Math::sin(angle)*configuration.innerRadius)+y*(Math::cos(angle)*configuration.innerRadius));
			}
		glEnd();
		
		/* Draw the outer circle: */
		glBegin(GL_LINE_LOOP);
		for(int i=0;i<64;++i)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(64);
			glVertex(Point::origin-x*(Math::sin(angle)*configuration.outerRadius)+y*(Math::cos(angle)*configuration.outerRadius));
			}
		glEnd();
		
		/* Draw the inner angle: */
		glBegin(GL_LINE_STRIP);
		glVertex(Point::origin-x*(Math::sin(configuration.innerAngle)*configuration.innerRadius)+y*(Math::cos(configuration.innerAngle)*configuration.innerRadius));
		glVertex(Point::origin);
		glVertex(Point::origin-x*(Math::sin(-configuration.innerAngle)*configuration.innerRadius)+y*(Math::cos(-configuration.innerAngle)*configuration.innerRadius));
		glEnd();
		
		/* Draw the outer angle: */
		glBegin(GL_LINE_STRIP);
		glVertex(Point::origin-x*(Math::sin(configuration.outerAngle)*configuration.outerRadius)+y*(Math::cos(configuration.outerAngle)*configuration.outerRadius));
		glVertex(Point::origin);
		glVertex(Point::origin-x*(Math::sin(-configuration.outerAngle)*configuration.outerRadius)+y*(Math::cos(-configuration.outerAngle)*configuration.outerRadius));
		glEnd();
		
		glEndList();
		}
	
	if(configuration.drawHud)
		{
		/* Create the HUD display list: */
		glNewList(dataItem->hudListId,GL_COMPILE);
		
		/* Calculate the HUD layout: */
		Scalar hudRadius=getDisplaySize()*Scalar(2);
		Scalar hudTickSize=configuration.hudFontSize;
		
		/* Draw the azimuth tick marks: */
		glColor(getForegroundColor());
		glBegin(GL_LINES);
		for(int az=0;az<360;az+=10)
			{
			Scalar angle=Math::rad(Scalar(az));
			Scalar c=Math::cos(angle)*hudRadius;
			Scalar s=Math::sin(angle)*hudRadius;
			glVertex(s,c,Scalar(0));
			glVertex(s,c,Scalar(0)+(az%30==0?hudTickSize*Scalar(2):hudTickSize));
			}
		glEnd();
		
		/* Draw the azimuth labels: */
		for(int az=0;az<360;az+=30)
			{
			/* Move to the label's coordinate system: */
			glPushMatrix();
			Scalar angle=Math::rad(Scalar(az));
			Scalar c=Math::cos(angle)*hudRadius;
			Scalar s=Math::sin(angle)*hudRadius;
			glTranslate(s,c,hudTickSize*Scalar(2.5));
			glRotate(-double(az),0.0,0.0,1.0);
			glRotate(90.0,1.0,0.0,0.0);
			double width=Scalar(numberRenderer.calcNumberWidth(az));
			glTranslate(-width*0.5,0.0,0.0);
			
			/* Draw the azimuth label: */
			numberRenderer.drawNumber(az,contextData);
			
			/* Go back to original coordinate system: */
			glPopMatrix();
			}
		
		glEndList();
		}
	}

}
