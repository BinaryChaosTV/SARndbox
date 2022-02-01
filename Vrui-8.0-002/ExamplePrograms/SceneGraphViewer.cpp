/***********************************************************************
SceneGraphViewer - Viewer for one or more scene graphs loaded from VRML
2.0 files.
Copyright (c) 2010-2021 Oliver Kreylos

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

#include "SceneGraphViewer.h"

#include <stdio.h>
#include <vector>
#include <Misc/FunctionCalls.h>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Geometry/Plane.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Button.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GraphNode.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <Vrui/Vrui.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/Viewer.h>

#include "SceneGraphViewerWalkNavigationTool.h"

// DEBUGGING
#include <iostream>
#include <Geometry/OutputOperators.h>

/*********************************
Methods of class SceneGraphViewer:
*********************************/

void SceneGraphViewer::goToPhysicalSpaceCallback(Misc::CallbackData* cbData)
	{
	Vrui::setNavigationTransformation(Vrui::NavTransform::identity);
	}

void SceneGraphViewer::sceneGraphToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& index)
	{
	/* Enable or disable the selected scene graph: */
	sceneGraphs[index].enabled=cbData->set;
	
	/* Add or remove the scene graph to/from its assigned central scene graph: */
	if(cbData->set)
		Vrui::getSceneGraphManager()->addNode(sceneGraphs[index].navigational,*sceneGraphs[index].root);
	else
		Vrui::getSceneGraphManager()->removeNode(sceneGraphs[index].navigational,*sceneGraphs[index].root);
	}

void SceneGraphViewer::reloadAllSceneGraphsCallback(Misc::CallbackData* cbData)
	{
	/* Remove all scene graphs from their assigned central scene graphs: */
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt)
		if(sgIt->enabled)
			Vrui::getSceneGraphManager()->removeNode(sgIt->navigational,*sgIt->root);
	
	/* Create a node creator to parse the VRML files: */
	SceneGraph::NodeCreator nodeCreator;
	
	int index=0;
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt,++index)
		{
		/* Delete the current scene graph: */
		bool enableOnLoad=sgIt->root==0||sgIt->enabled;
		sgIt->root=0;
		sgIt->enabled=false;
		
		/* Try reloading the scene graph: */
		try
			{
			/* Create the new scene graph's root node: */
			SceneGraph::GroupNodePointer root=new SceneGraph::GroupNode;
			
			/* Load and parse the VRML file: */
			SceneGraph::VRMLFile vrmlFile(sgIt->fileName.c_str(),nodeCreator);
			vrmlFile.parse(*root);
			
			/* Add the new scene graph to the list: */
			sgIt->root=root;
			sgIt->enabled=enableOnLoad;
			
			if(enableOnLoad)
				{
				/* Add the new scene graph to the appropriate root node: */
				Vrui::getSceneGraphManager()->addNode(sgIt->navigational,*sgIt->root);
				}
			}
		catch(const std::runtime_error& err)
			{
			/* Print an error message and try the next file: */
			Misc::formattedUserWarning("Scene Graph Viewer: Ignoring file %s due to exception %s",sgIt->fileName.c_str(),err.what());
			}
		
		/* Update the state of the menu button associated with this scene graph: */
		GLMotif::ToggleButton* sceneGraphToggle=dynamic_cast<GLMotif::ToggleButton*>(mainMenu->getEntry(index));
		if(sceneGraphToggle!=0)
			sceneGraphToggle->setToggle(sgIt->enabled);
		}
	}

void SceneGraphViewer::alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData)
	{
	/* Retrieve the user's current pose in physical space: */
	Vrui::Point headPosPhys=Vrui::getMainViewer()->getHeadPosition();
	Vrui::Point footPosPhys=Vrui::calcFloorPoint(headPosPhys);
	Vrui::Scalar heightPhys=Geometry::dist(headPosPhys,footPosPhys);
	
	/* Calculate the user's proposed pose in navigational space: */
	Point footPos=alignmentData.surfaceFrame.getOrigin();
	Vector up=alignmentData.surfaceFrame.getRotation().getDirection(2);
	Scalar height=heightPhys*alignmentData.surfaceFrame.getScaling();
	
	// DEBUGGING
	// std::cout<<"Foot: "<<footPos<<", head: "<<headPos<<std::endl;
	// std::cout<<"Up vector: "<<up<<", height: "<<height<<std::endl;
	// std::cout<<"Proposed move: "<<alignmentData.prevSurfaceFrame.getOrigin()<<" to "<<footPos<<std::endl;
	
	/* Retrieve alignment parameters: */
	Scalar ps(alignmentData.probeSize);
	Vector psvec=up*ps;
	Scalar mc(alignmentData.maxClimb);
	
	/* Check whether this is a continuing navigation sequence: */
	AlignmentState* as=static_cast<AlignmentState*>(alignmentData.alignmentState);
	if(as!=0)
		{
		/* Retrieve the user's previous head and foot positions: */
		Point prevFootPos=alignmentData.prevSurfaceFrame.getOrigin();
		Point prevHeadPos=prevFootPos+up*as->height;
		
		/* Calculate the foot's movement vector: */
		Vector moveVec=footPos-prevFootPos;
		
		/* Raise the user's foot to step over low obstacles: */
		prevFootPos+=up*(ps+Math::min(mc,Math::div2(as->height))); // Step by the tool's max climb, but not more than half the avatar's height
		
		/* Trace the head and foot from the previous to the intended positions along a common movement vector: */
		Point headPos;
		if(moveVec!=Vector::zero)
			{
			SceneGraph::SphereCollisionQuery headQuery(prevHeadPos,moveVec,ps);
			Vrui::getSceneGraphManager()->testNavigationalCollision(headQuery);
			SceneGraph::SphereCollisionQuery footQuery(prevFootPos,moveVec,ps);
			Vrui::getSceneGraphManager()->testNavigationalCollision(footQuery);
			
			/* Check if there was a collision: */
			if(headQuery.isHit()||footQuery.isHit())
				{
				/* Move head and foot to the collision position and get the collision plane normal: */
				Vector step,collisionNormal;
				if(headQuery.getHitLambda()<=footQuery.getHitLambda())
					{
					step=moveVec*headQuery.getHitLambda();
					collisionNormal=headQuery.getHitNormal();
					}
				else
					{
					step=moveVec*footQuery.getHitLambda();
					collisionNormal=footQuery.getHitNormal();
					}
				headPos=prevHeadPos+step;
				footPos=prevFootPos+step;
				
				/* Align the remaining movement vector with the collision plane: */
				Vector n1=up;
				n1.orthogonalize(moveVec);
				if(n1!=Vector::zero)
					{
					/* Find a sliding vector that is in both the collision plane and the current movement plane: */
					Vector slide=n1^collisionNormal;
					Vector moveVec2=slide*(((moveVec-step)*slide)/slide.sqr());
					
					/* Add a fudge factor away from the collision plane to avoid numerical errors: */
					moveVec2.addScaled(collisionNormal,Scalar(1.0e-5)/collisionNormal.mag());
					
					/* Trace the head and foot again: */
					SceneGraph::SphereCollisionQuery headQuery2(headPos,moveVec2,ps);
					Vrui::getSceneGraphManager()->testNavigationalCollision(headQuery2);
					SceneGraph::SphereCollisionQuery footQuery2(footPos,moveVec2,ps);
					Vrui::getSceneGraphManager()->testNavigationalCollision(footQuery2);
					Scalar stepLambda=Math::min(headQuery2.getHitLambda(),footQuery2.getHitLambda());
					step=moveVec2*stepLambda;
					headPos+=step;
					footPos+=step;
					
					// DEBUGGING
					// std::cout<<collisionNormal*moveVec2<<", "<<stepLambda<<std::endl;
					}
				}
			else
				{
				headPos=prevHeadPos+moveVec;
				footPos=prevFootPos+moveVec;
				}
			}
		else
			{
			headPos=prevHeadPos;
			footPos=prevFootPos;
			}
		
		/* Drop the foot down to extend the avatar to its new height and a maxClimb beyond to test for falling: */
		Point dropFootPos=Geometry::subtractScaled(headPos,up,height+mc-ps);
		SceneGraph::SphereCollisionQuery footDropQuery(footPos,dropFootPos-footPos,ps);
		Vrui::getSceneGraphManager()->testNavigationalCollision(footDropQuery);
		
		/* Check if the avatar could not be extended to its full height: */
		Scalar extendHeight=(height+mc-ps)*footDropQuery.getHitLambda()+ps;
		if(extendHeight<height)
			{
			footPos=footDropQuery.getHitPoint();
			
			#if 0
			
			/* Check if the foot landed on a surface that is too steep to stand: */
			const Vector& hitNormal=footDropQuery.getHitNormal();
			if(hitNormal*up<Math::cos(Math::rad(Scalar(45)))*hitNormal.mag())
				{
				/* Find a slide vector that is in the plane of the current movement, to avoid having to re-trace the head position: */
				Vector slideVec=(moveVec^up)^hitNormal;
				Scalar svup=slideVec*up;
				if(svup<Scalar(0))
					{
					// DEBUGGING
					// std::cout<<"Sliding!"<<std::endl;
					
					/* Slide back until the avatar is fully extended or the current motion is undone: */
					Vector horMoveVec=moveVec;
					horMoveVec.orthogonalize(up);
					Scalar slideLambda=Math::min((extendHeight-height)/svup,-((footPos-prevHeadPos)*horMoveVec)/(slideVec*horMoveVec));
					SceneGraph::SphereCollisionQuery footSlideQuery(footPos,slideVec*slideLambda,ps);
					Vrui::getSceneGraphManager()->testNavigationalCollision(footSlideQuery);
					footPos=footSlideQuery.getHitPoint();
					}
				}
			
			#endif
			}
		else
			footPos=dropFootPos;
		
		// DEBUGGING
		// Vector step=footPos-prevFootPos;
		// step[2]=Scalar(0);
		// moveVec[2]=Scalar(0);
		// std::cout<<moveVec.mag()<<", "<<step.mag()<<std::endl;
		}
	else
		{
		/* Initialize a new navigation sequence: */
		alignmentData.alignmentState=as=new AlignmentState;
		as->floorLift=Scalar(0);
		
		/* Drop down from the proposed foot position to let the navigation tool implement falling: */
		footPos+=psvec;
		SceneGraph::SphereCollisionQuery footDropQuery(footPos,-up*mc,ps);
		Vrui::getSceneGraphManager()->testNavigationalCollision(footDropQuery);
		footPos=footDropQuery.getHitPoint();
		}
	
	/* Position the proposed frame at the final foot position: */
	alignmentData.surfaceFrame=Vrui::NavTransform(Vrui::Point(footPos-psvec)-Vrui::Point::origin,alignmentData.surfaceFrame.getRotation(),alignmentData.surfaceFrame.getScaling());
	
	/* Update the alignment state: */
	as->height=height;
	}

SceneGraphViewer::SceneGraphViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 mainMenu(0)
	{
	/* Create a node creator to parse the VRML files: */
	SceneGraph::NodeCreator nodeCreator;
	
	/* Keep track if any of the loaded scene graphs require an audio processing pass: */
	bool requireAudio=false;
	
	/* Parse the command line: */
	bool navigational=true;
	bool enable=true;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"navigational")==0||strcasecmp(argv[i]+1,"n")==0)
				navigational=true;
			else if(strcasecmp(argv[i]+1,"physical")==0||strcasecmp(argv[i]+1,"p")==0)
				navigational=false;
			else if(strcasecmp(argv[i]+1,"enable")==0||strcasecmp(argv[i]+1,"e")==0)
				enable=true;
			else if(strcasecmp(argv[i]+1,"disable")==0||strcasecmp(argv[i]+1,"d")==0)
				enable=false;
			}
		else
			{
			/* Create a scene graph item for the given file name: */
			sceneGraphs.push_back(SGItem());
			SGItem& sg=sceneGraphs.back();
			sg.fileName=argv[i];
			sg.navigational=navigational;
			sg.enabled=false;
			
			/* Try loading the scene graph: */
			try
				{
				/* Create the new scene graph's root node: */
				SceneGraph::GroupNodePointer root=new SceneGraph::GroupNode;
				
				/* Load and parse the VRML file: */
				SceneGraph::VRMLFile vrmlFile(sg.fileName.c_str(),nodeCreator);
				vrmlFile.parse(*root);
				
				/* Add the new scene graph to the list: */
				sg.root=root;
				sg.enabled=enable;
				
				if(enable)
					{
					/* Add the new scene graph to the appropriate root node: */
					Vrui::getSceneGraphManager()->addNode(navigational,*root);
					}
				
				/* Check if the scene graph requires an audio pass: */
				requireAudio=requireAudio||(root->getPassMask()&SceneGraph::GraphNode::ALRenderPass)!=0x0U;
				}
			catch(const std::runtime_error& err)
				{
				/* Print an error message and try the next file: */
				Misc::formattedUserWarning("Scene Graph Viewer: Ignoring file %s due to exception %s",sg.fileName.c_str(),err.what());
				
				/* Remove the tentatively-added scene graph again: */
				sceneGraphs.pop_back();
				}
			}
		}
	
	/* Request audio processing if needed: */
	if(requireAudio)
		Vrui::requestSound();
	
	/* Create the main menu shell: */
	mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Scene Graph Viewer");
	
	/* Add a button to go to physical space: */
	GLMotif::Button* goToPhysicalSpaceButton=new GLMotif::Button("GoToPhysicalSpaceButton",mainMenu,"Go To Physical Space");
	goToPhysicalSpaceButton->getSelectCallbacks().add(this,&SceneGraphViewer::goToPhysicalSpaceCallback);
	
	/* Add a toggle button for each scene graph: */
	mainMenu->addSeparator();
	int index=0;
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt,++index)
		{
		/* Generate a widget name: */
		char toggleName[40];
		snprintf(toggleName,sizeof(toggleName),"SceneGraphToggle%d",index);
		
		/* Extract a short name from the scene graph file name: */
		std::string::iterator start=sgIt->fileName.begin();
		std::string::iterator end=sgIt->fileName.end();
		for(std::string::iterator fnIt=sgIt->fileName.begin();fnIt!=sgIt->fileName.end();++fnIt)
			{
			if(*fnIt=='/')
				{
				start=fnIt+1;
				end=sgIt->fileName.end();
				}
			else if(*fnIt=='.')
				end=fnIt;
			}
		std::string name(start,end);
		
		/* Create a toggle button: */
		GLMotif::ToggleButton* sceneGraphToggle=new GLMotif::ToggleButton(toggleName,mainMenu,name.c_str());
		sceneGraphToggle->setToggle(sgIt->enabled);
		sceneGraphToggle->getValueChangedCallbacks().add(this,&SceneGraphViewer::sceneGraphToggleCallback,index);
		}
	mainMenu->addSeparator();
	
	/* Add a button to force a reload on all scene graphs: */
	GLMotif::Button* reloadAllSceneGraphsButton=new GLMotif::Button("ReloadAllSceneGraphsButton",mainMenu,"Reload All");
	reloadAllSceneGraphsButton->getSelectCallbacks().add(this,&SceneGraphViewer::reloadAllSceneGraphsCallback);
	
	/* Finish and install the main menu: */
	mainMenu->manageMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Initialize the custom tool classes: */
	WalkNavigationToolFactory* walkNavigationToolFactory=new WalkNavigationToolFactory(*Vrui::getToolManager());
	Vrui::getToolManager()->addClass(walkNavigationToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

SceneGraphViewer::~SceneGraphViewer(void)
	{
	delete mainMenu;
	}

void SceneGraphViewer::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Let the base class at it first: */
	Vrui::Application::toolCreationCallback(cbData);
	
	/* Check if the new tool is a surface navigation tool: */
	Vrui::SurfaceNavigationTool* surfaceNavigationTool=dynamic_cast<Vrui::SurfaceNavigationTool*>(cbData->tool);
	if(surfaceNavigationTool!=0)
		{
		/* Set the new tool's alignment function: */
		surfaceNavigationTool->setAlignFunction(Misc::createFunctionCall(this,&SceneGraphViewer::alignSurfaceFrame));
		}
	}

void SceneGraphViewer::display(GLContextData& contextData) const
	{
	/* Actually, got nothing to do! */
	}

void SceneGraphViewer::resetNavigation(void)
	{
	/* Calculate the bounding box of all enabled navigational-space scene graphs: */
	SceneGraph::Box bbox=Vrui::getSceneGraphManager()->calcNavigationalBoundingBox();
	
	/* Show the entire bounding box: */
	Vrui::setNavigationTransformation(Geometry::mid(bbox.min,bbox.max),Math::div2(Geometry::dist(bbox.min,bbox.max)),Vrui::Vector(0,1,0));
	}

VRUI_APPLICATION_RUN(SceneGraphViewer)
