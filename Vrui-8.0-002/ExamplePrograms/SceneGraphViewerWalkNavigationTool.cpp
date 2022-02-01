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

#include "SceneGraphViewerWalkNavigationTool.h"

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLMaterialTemplates.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <Vrui/Vrui.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>

// DEBUGGING
#include <iostream>
#include <Geometry/OutputOperators.h>

/***************************************************************************
Methods of class SceneGraphViewer::WalkNavigationToolFactory::Configuration:
***************************************************************************/

SceneGraphViewer::WalkNavigationToolFactory::Configuration::Configuration(void)
	:fallAcceleration(Vrui::getMeterFactor()*Scalar(9.81)),
	 probeSize(Vrui::getInchFactor()*Scalar(12)),
	 maxClimb(Vrui::getInchFactor()*Scalar(12)),
	 orbRadius(Vrui::getInchFactor()*Scalar(1)),
	 orbVelocity(Vrui::getMeterFactor()*Scalar(7.5)),
	 startActive(false)
	{
	}

void SceneGraphViewer::WalkNavigationToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	fallAcceleration=cfs.retrieveValue<Scalar>("./fallAcceleration",fallAcceleration);
	probeSize=cfs.retrieveValue<Scalar>("./probeSize",probeSize);
	maxClimb=cfs.retrieveValue<Scalar>("./maxClimb",maxClimb);
	orbRadius=cfs.retrieveValue<Scalar>("./orbRadius",orbRadius);
	orbVelocity=cfs.retrieveValue<Scalar>("./orbVelocity",orbVelocity);
	startActive=cfs.retrieveValue<bool>("./startActive",startActive);
	}

void SceneGraphViewer::WalkNavigationToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<Scalar>("./fallAcceleration",fallAcceleration);
	cfs.storeValue<Scalar>("./probeSize",probeSize);
	cfs.storeValue<Scalar>("./maxClimb",maxClimb);
	cfs.storeValue<Scalar>("./orbRadius",orbRadius);
	cfs.storeValue<Scalar>("./orbVelocity",orbVelocity);
	cfs.storeValue<bool>("./startActive",startActive);
	}

/************************************************************
Methods of class SceneGraphViewer::WalkNavigationToolFactory:
************************************************************/

SceneGraphViewer::WalkNavigationToolFactory::WalkNavigationToolFactory(Vrui::ToolManager& toolManager)
	:Vrui::ToolFactory("SceneGraphViewerWalkNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(5);
	layout.setNumValuators(0);
	
	/* Insert class into class hierarchy: */
	Vrui::ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	configuration.read(toolManager.getToolClassSection(getClassName()));
	
	/* Set tool class' factory pointer: */
	WalkNavigationTool::factory=this;
	}

SceneGraphViewer::WalkNavigationToolFactory::~WalkNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	WalkNavigationTool::factory=0;
	}

const char* SceneGraphViewer::WalkNavigationToolFactory::getName(void) const
	{
	return "Walk on Scene Graph";
	}

const char* SceneGraphViewer::WalkNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Start / Stop";
			break;
		
		case 1:
			return "Teleport";
			break;
		
		case 2:
			return "Turn Left";
			break;
		
		case 3:
			return "Turn Right";
			break;
		
		case 4:
			return "Turn Around";
			break;
		
		default:
			return 0;
		}
	}

Vrui::Tool* SceneGraphViewer::WalkNavigationToolFactory::createTool(const Vrui::ToolInputAssignment& inputAssignment) const
	{
	return new WalkNavigationTool(this,inputAssignment);
	}

void SceneGraphViewer::WalkNavigationToolFactory::destroyTool(Vrui::Tool* tool) const
	{
	delete tool;
	}

/*************************************************************
Static elements of class SceneGraphViewer::WalkNavigationTool:
*************************************************************/

SceneGraphViewer::WalkNavigationToolFactory* SceneGraphViewer::WalkNavigationTool::factory=0;

/*****************************************************
Methods of class SceneGraphViewer::WalkNavigationTool:
*****************************************************/

void SceneGraphViewer::WalkNavigationTool::applyNavState(void) const
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotateAround(Point(0,0,headHeight),Rotation::rotateX(elevation));
	nav*=NavTransform::rotate(Rotation::rotateZ(azimuth));
	nav*=Geometry::invert(surfaceFrame);
	Vrui::setNavigationTransformation(nav);
	}

void SceneGraphViewer::WalkNavigationTool::initNavState(void)
	{
	/* Calculate the main viewer's current head and foot positions: */
	Point headPos=Vrui::getMainViewer()->getHeadPosition();
	footPos=Vrui::calcFloorPoint(headPos);
	headHeight=Geometry::dist(headPos,footPos);
	
	/* Set up a physical navigation frame around the main viewer's current head position: */
	calcPhysicalFrame(headPos);
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	surfaceFrame=Vrui::getInverseNavigationTransformation()*physicalFrame;
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
		fallVelocity=-configuration.fallAcceleration*Vrui::getFrameTime();
		}
	
	/* Move the physical frame to the foot position, and adjust the surface frame accordingly: */
	newSurfaceFrame*=Geometry::invert(physicalFrame)*NavTransform::translate(footPos-headPos)*physicalFrame;
	physicalFrame.leftMultiply(NavTransform::translate(footPos-headPos));
	
	/* Apply the initial navigation state: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

SceneGraphViewer::WalkNavigationTool::WalkNavigationTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::SurfaceNavigationTool(factory,inputAssignment),
	 configuration(WalkNavigationTool::factory->configuration),
	 teleport(false),orbPath(Scalar(0),0,0)
	{
	}

void SceneGraphViewer::WalkNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void SceneGraphViewer::WalkNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void SceneGraphViewer::WalkNavigationTool::initialize(void)
	{
	/* Try activating this tool if requested: */
	if(configuration.startActive&&activate())
		{
		/* Initialize the navigation state: */
		initNavState();
		}
	}

const Vrui::ToolFactory* SceneGraphViewer::WalkNavigationTool::getFactory(void) const
	{
	return factory;
	}

void SceneGraphViewer::WalkNavigationTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		switch(buttonSlotIndex)
			{
			case 0:
				/* Activate or deactivate the tool: */
				if(isActive())
					{
					/* Deactivate this tool: */
					teleport=false;
					deactivate();
					}
				else
					{
					/* Try activating this tool: */
					if(activate())
						{
						/* Initialize the navigation state: */
						initNavState();
						}
					}
				break;
			
			case 1:
				if(isActive())
					{
					/* Activate the teleportation orb: */
					teleport=true;
					}
				break;
			
			case 2:
				if(isActive())
					{
					/* Turn left by 90 degrees: */
					azimuth=wrapAngle(azimuth-Math::div2(Math::Constants<Scalar>::pi));
					}
				break;
			
			case 3:
				if(isActive())
					{
					/* Turn right by 90 degrees: */
					azimuth=wrapAngle(azimuth+Math::div2(Math::Constants<Scalar>::pi));
					}
				break;
			
			case 4:
				if(isActive())
					{
					/* Turn 180 degrees: */
					azimuth=wrapAngle(azimuth+Math::Constants<Scalar>::pi);
					}
				break;
			
			default:
				; // Do nothing
			}
		}
	}

void SceneGraphViewer::WalkNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Calculate the new head and foot positions: */
		Point newFootPos=Vrui::calcFloorPoint(Vrui::getMainViewer()->getHeadPosition());
		headHeight=Geometry::dist(Vrui::getMainViewer()->getHeadPosition(),newFootPos);
		
		if(teleport)
			{
			/* Get the teleportation orb's initial position and velocity vector in navigational space: */
			Scalar scale=Vrui::getInverseNavigationTransformation().getScaling();
			SceneGraph::Point orbPos(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
			SceneGraph::Vector orbVel(Vrui::getInverseNavigationTransformation().transform(getButtonDeviceRayDirection(0)));
			orbVel*=SceneGraph::Scalar(scale*configuration.orbVelocity/orbVel.mag());
			
			/* Get the up direction in navigational space: */
			SceneGraph::Vector up(Vrui::getInverseNavigationTransformation().transform(Vrui::getUpDirection()));
			up.normalize();
			
			/* Re-initialize the orb path: */
			SceneGraph::Scalar orbRadius(scale*configuration.orbRadius);
			orbPath.setTubeRadius(orbRadius);
			orbPath.clear();
			orbPath.addVertex(orbPos);
			
			/* Trace the orb until it hits a flat enough surface: */
			SceneGraph::Scalar time(0);
			SceneGraph::Scalar timeStep(0.05);
			SceneGraph::Scalar maxTime(2);
			while(time<=maxTime)
				{
				SceneGraph::Scalar ts=timeStep;
				SceneGraph::Vector orbStep=orbVel*ts;
				SceneGraph::SphereCollisionQuery orbQuery(orbPos,orbStep,orbRadius);
				Vrui::getSceneGraphManager()->testNavigationalCollision(orbQuery);
				if(orbQuery.isHit())
					{
					if(orbQuery.getHitLambda()>Scalar(0))
						{
						/* Move the orb to the contact point: */
						orbPos.addScaled(orbStep,orbQuery.getHitLambda());
						orbPath.addVertex(orbPos);
						}
					
					/* Stop tracing if the contact surface is flat enough to stand on: */
					if(orbQuery.getHitNormal()*up>=Math::cos(Math::rad(SceneGraph::Scalar(22.5)))*orbQuery.getHitNormal().mag())
						break;
					
					/* Bounce the orb off the surface and keep tracing: */
					orbVel.reflect(orbQuery.getHitNormal());
					orbVel*=SceneGraph::Scalar(-0.5);
					ts*=orbQuery.getHitLambda();
					}
				else
					{
					orbPos+=orbStep;
					orbPath.addVertex(orbPos);
					}
				orbVel.subtractScaled(up,SceneGraph::Scalar(scale*configuration.fallAcceleration*ts));
				time+=ts;
				}
			
			/* Check if the teleportation orb is valid: */
			orbValid=time<=maxTime;
			
			/* Check if the teleportation button was released: */
			if(!getButtonState(1))
				{
				if(orbValid)
					{
					/* Teleport by moving the surface frame to the orb's position: */
					surfaceFrame.leftMultiply(Vrui::NavTransform::translate(Vrui::Point(orbPos)-surfaceFrame.getOrigin()));
					}
				
				/* Deactivate the teleportation orb: */
				teleport=false;
				}
			}
		
		/* Create a physical navigation frame around the new foot position: */
		calcPhysicalFrame(newFootPos);
		
		/* Calculate the movement from walking: */
		Vector move=newFootPos-footPos;
		footPos=newFootPos;
		
		/* Add the current falling velocity: */
		Vector moveDir=Vrui::getUpDirection()*fallVelocity;
		
		/* Calculate the complete movement vector: */
		move+=moveDir*Vrui::getFrameTime();
		
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
		
		/* Check if the initial surface frame is above the surface: */
		Scalar z=newSurfaceFrame.inverseTransform(initialOrigin)[2];
		if(z>Scalar(0))
			{
			/* Lift the aligned frame back up to the original altitude and continue flying: */
			newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
			fallVelocity-=configuration.fallAcceleration*Vrui::getFrameTime();
			}
		else
			{
			/* Stop falling: */
			fallVelocity=Scalar(0);
			}
		
		/* Apply the newly aligned surface frame: */
		surfaceFrame=newSurfaceFrame;
		applyNavState();
		}
	}

void SceneGraphViewer::WalkNavigationTool::display(GLContextData& contextData) const
	{
	if(isActive()&&teleport)
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT);
		
		/* Go to navigational space: */
		Vrui::goToNavigationalSpace(contextData);
		
		/* Indicate whether the orb can be teleported to: */
		if(orbValid)
			{
			glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.0f,0.5f,1.0f));
			glMaterialSpecular(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.333f,0.333f,0.333f));
			glMaterialShininess(GLMaterialEnums::FRONT,32.0f);
			glMaterialEmission(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.0f,0.5f,0.0f));
			}
		else
			{
			glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.5f,1.0f,0.0f));
			glMaterialSpecular(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.333f,0.333f,0.333f));
			glMaterialShininess(GLMaterialEnums::FRONT,32.0f);
			glMaterialEmission(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.5f,0.0f,0.0f));
			}
		
		/* Draw the teleportation orb's path: */
		orbPath.glRenderAction(contextData);
		
		/* Return to physical space: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}
