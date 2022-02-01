/***********************************************************************
SceneGraphManager - Class to manage a scene graph used to represent
renderable objects in physical and navigational space.
Copyright (c) 2021 Oliver Kreylos

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

#include <Vrui/SceneGraphManager.h>

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLClipPlaneTracker.h>
#include <SceneGraph/Node.h>
#include <SceneGraph/GraphNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>

namespace Vrui {

/****************************************************
Declaration of class SceneGraphManager::ClippedGroup:
****************************************************/

class SceneGraphManager::ClippedGroup:public SceneGraph::GroupNode
	{
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Constructors and destructors: */
	ClippedGroup(void);
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	
	/* Methods from class GraphNode: */
	virtual void glRenderAction(SceneGraph::GLRenderState& renderState) const;
	};

/********************************************************
Static elements of class SceneGraphManager::ClippedGroup:
********************************************************/

const char* SceneGraphManager::ClippedGroup::className="SceneGraphManager::ClippedGroup";

/************************************************
Methods of class SceneGraphManager::ClippedGroup:
************************************************/

SceneGraphManager::ClippedGroup::ClippedGroup(void)
	{
	}

const char* SceneGraphManager::ClippedGroup::getClassName(void) const
	{
	return className;
	}

void SceneGraphManager::ClippedGroup::glRenderAction(SceneGraph::GLRenderState& renderState) const
	{
	/* Resume clipping planes: */
	renderState.contextData.getClipPlaneTracker()->resume();
	
	/* Delegate to the base class method: */
	SceneGraph::GroupNode::glRenderAction(renderState);
	
	/* Pause clipping planes again: */
	renderState.contextData.getClipPlaneTracker()->pause();
	}

/**********************************
Methods of class SceneGraphManager:
**********************************/

void SceneGraphManager::setNavigationTransformation(const NavTransform& newNavigationTransformation)
	{
	/* Update the navigational-space scene graph's transformation: */
	navigationalRoot->setTransform(newNavigationTransformation);
	}

void SceneGraphManager::updateInputDevices(void)
	{
	/* Update the transformations of the device scene graphs of all enabled input devices: */
	for(DeviceSceneGraphMap::Iterator dsgmIt=deviceSceneGraphMap.begin();!dsgmIt.isFinished();++dsgmIt)
		{
		DeviceSceneGraph& dsg=dsgmIt->getDest();
		if(dsg.enabled)
			dsg.root->setTransform(dsgmIt->getSource()->getTransformation());
		}
	}

void SceneGraphManager::setInputDeviceState(InputDevice* device,bool newEnabled)
	{
	/* Check if the given device is represented in the scene graph: */
	DeviceSceneGraphMap::Iterator dsgmIt=deviceSceneGraphMap.findEntry(device);
	if(!dsgmIt.isFinished())
		{
		DeviceSceneGraph& dsg=dsgmIt->getDest();
		
		/* Add or remove the device's scene graph from the physical-space scene graph: */
		if(newEnabled)
			physicalRoot->addChild(*dsg.root);
		else
			physicalRoot->removeChild(*dsg.root);
		
		/* Update the device's state: */
		dsg.enabled=newEnabled;
		}
	}

void SceneGraphManager::removeInputDevice(InputDevice* device)
	{
	/* Check if the given device is represented in the scene graph: */
	DeviceSceneGraphMap::Iterator dsgmIt=deviceSceneGraphMap.findEntry(device);
	if(!dsgmIt.isFinished())
		{
		DeviceSceneGraph& dsg=dsgmIt->getDest();
		
		/* If the device is currently enabled, remove it from the physical-space scene graph: */
		if(dsg.enabled)
			physicalRoot->removeChild(*dsg.root);
		
		/* Remove the device from the device scene graph map: */
		deviceSceneGraphMap.removeEntry(dsgmIt);
		}
	}

SceneGraphManager::SceneGraphManager(void)
	:physicalRoot(new SceneGraph::GroupNode),navigationalRoot(new SceneGraph::DOGTransformNode),
	 clippedRoot(new ClippedGroup),
	 deviceSceneGraphMap(17)
	{
	/* Add the navigational-space scene graph to the physical-space scene graph: */
	addPhysicalNode(*navigationalRoot);
	
	/* Add the clipped navigational-space scene graph to the navigational-space scene graph: */
	addUnclippedNavigationalNode(*clippedRoot);
	}

void SceneGraphManager::addPhysicalNode(SceneGraph::GraphNode& node)
	{
	/* Add the given node to the physical-space scene graph: */
	physicalRoot->addChild(node);
	}

void SceneGraphManager::updatePhysicalNode(SceneGraph::GraphNode& node,unsigned int updateReason)
	{
	/* Notify the physical-space scene graph root node of a child's update: */
	physicalRoot->cascadingUpdate(node,updateReason);
	}

void SceneGraphManager::removePhysicalNode(SceneGraph::GraphNode& node)
	{
	/* Remove the given node from the physical-space scene graph: */
	physicalRoot->removeChild(node);
	}

void SceneGraphManager::addNavigationalNode(SceneGraph::GraphNode& node)
	{
	/* Add the given node to the clipped navigational-space scene graph and cascade the update towards the physical-space root if necessary: */
	unsigned int cnavUpdateResult=clippedRoot->addChild(node);
	if(cnavUpdateResult!=SceneGraph::Node::NoCascade)
		{
		unsigned int navUpdateResult=navigationalRoot->cascadingUpdate(*clippedRoot,cnavUpdateResult);
		if(navUpdateResult!=SceneGraph::Node::NoCascade)
			physicalRoot->cascadingUpdate(*navigationalRoot,navUpdateResult);
		}
	}

void SceneGraphManager::updateNavigationalNode(SceneGraph::GraphNode& node,unsigned int updateReason)
	{
	/* Notify the clipped navigational-space scene graph root node of a child's update and cascade the update towards the physical-space root if necessary: */
	unsigned int cnavUpdateResult=clippedRoot->cascadingUpdate(node,updateReason);
	if(cnavUpdateResult!=SceneGraph::Node::NoCascade)
		{
		unsigned int navUpdateResult=navigationalRoot->cascadingUpdate(*clippedRoot,cnavUpdateResult);
		if(navUpdateResult!=SceneGraph::Node::NoCascade)
			physicalRoot->cascadingUpdate(*navigationalRoot,navUpdateResult);
		}
	}

void SceneGraphManager::removeNavigationalNode(SceneGraph::GraphNode& node)
	{
	/* Remove the given node from the clipped navigational-space scene graph and cascade the update towards the physical-space root if necessary: */
	unsigned int cnavUpdateResult=clippedRoot->removeChild(node);
	if(cnavUpdateResult!=SceneGraph::Node::NoCascade)
		{
		unsigned int navUpdateResult=navigationalRoot->cascadingUpdate(*clippedRoot,cnavUpdateResult);
		if(navUpdateResult!=SceneGraph::Node::NoCascade)
			physicalRoot->cascadingUpdate(*navigationalRoot,navUpdateResult);
		}
	}

void SceneGraphManager::addUnclippedNavigationalNode(SceneGraph::GraphNode& node)
	{
	/* Add the given node to the navigational-space scene graph and cascade the update to the physical-space root if necessary: */
	unsigned int navUpdateResult=navigationalRoot->addChild(node);
	if(navUpdateResult!=SceneGraph::Node::NoCascade)
		physicalRoot->cascadingUpdate(*navigationalRoot,navUpdateResult);
	}

void SceneGraphManager::updateUnclippedNavigationalNode(SceneGraph::GraphNode& node,unsigned int updateReason)
	{
	/* Notify the navigational-space scene graph root node of a child's update and cascade the update to the physical-space root if necessary: */
	unsigned int navUpdateResult=navigationalRoot->cascadingUpdate(node,updateReason);
	if(navUpdateResult!=SceneGraph::Node::NoCascade)
		physicalRoot->cascadingUpdate(*navigationalRoot,navUpdateResult);
	}

void SceneGraphManager::removeUnclippedNavigationalNode(SceneGraph::GraphNode& node)
	{
	/* Remove the given node from the navigational-space scene graph and cascade the update to the physical-space root if necessary: */
	unsigned int navUpdateResult=navigationalRoot->removeChild(node);
	if(navUpdateResult!=SceneGraph::Node::NoCascade)
		physicalRoot->cascadingUpdate(*navigationalRoot,navUpdateResult);
	}

void SceneGraphManager::addDeviceNode(InputDevice* device,SceneGraph::GraphNode& node)
	{
	/* Search for the device in the device scene graph map: */
	DeviceSceneGraphMap::Iterator dsgmIt=deviceSceneGraphMap.findEntry(device);
	if(dsgmIt.isFinished())
		{
		/* Create a new scene graph for the requested device and add it to the device scene graph map: */
		SceneGraph::ONTransformNode* deviceRoot=new SceneGraph::ONTransformNode;
		bool enabled=getInputGraphManager()->isEnabled(device);
		dsgmIt=deviceSceneGraphMap.setAndFindEntry(DeviceSceneGraphMap::Entry(device,DeviceSceneGraph(*deviceRoot,enabled)));
		
		/* Add the new device scene graph to the physical-space scene graph if the device is enabled: */
		if(enabled)
			physicalRoot->addChild(*deviceRoot);
		}
	
	/* Add the given node to the device scene graph root and cascade the update to the physical-space root if necessary: */
	unsigned int devUpdateResult=dsgmIt->getDest().root->addChild(node);
	if(dsgmIt->getDest().enabled&&devUpdateResult!=SceneGraph::Node::NoCascade)
		physicalRoot->cascadingUpdate(*dsgmIt->getDest().root,devUpdateResult);
	}

void SceneGraphManager::updateDeviceNode(InputDevice* device,SceneGraph::GraphNode& node,unsigned int updateReason)
	{
	/* Search for the device in the device scene graph map: */
	DeviceSceneGraphMap::Iterator dsgmIt=deviceSceneGraphMap.findEntry(device);
	if(!dsgmIt.isFinished())
		{
		DeviceSceneGraph& dsg=deviceSceneGraphMap.getEntry(device).getDest();
		
		/* Notify the device scene graph root node of a child's update and cascade the update to the physical-space root if necessary: */
		unsigned int devUpdateResult=dsg.root->cascadingUpdate(node,updateReason);
		if(dsg.enabled&&devUpdateResult!=SceneGraph::Node::NoCascade)
			physicalRoot->cascadingUpdate(*dsg.root,devUpdateResult);
		}
	}

void SceneGraphManager::removeDeviceNode(InputDevice* device,SceneGraph::GraphNode& node)
	{
	/* Search for the device in the device scene graph map: */
	DeviceSceneGraphMap::Iterator dsgmIt=deviceSceneGraphMap.findEntry(device);
	if(!dsgmIt.isFinished())
		{
		DeviceSceneGraph& dsg=deviceSceneGraphMap.getEntry(device).getDest();
		
		/* Remove the given node from the device scene graph root: */
		unsigned int devUpdateResult=dsg.root->removeChild(node);
		
		/* Check if the device scene graph is now empty: */
		if(dsg.root->getChildren().empty())
			{
			/* Remove the device scene graph from the physical-space scene graph if the device is currently enabled: */
			if(dsg.enabled)
				physicalRoot->removeChild(*dsg.root);
			
			/* Remove the device from the device scene graph map: */
			deviceSceneGraphMap.removeEntry(dsgmIt);
			}
		else
			{
			/* Cascade the update to the physical-space root if necessary: */
			if(dsg.enabled&&devUpdateResult!=SceneGraph::Node::NoCascade)
				physicalRoot->cascadingUpdate(*dsg.root,devUpdateResult);
			}
		}
	}

SceneGraph::Box SceneGraphManager::calcNavigationalBoundingBox(void) const
	{
	/* Calculate the union of the bounding boxes of all children of the navigational-space scene graph root: */
	SceneGraph::Box result=SceneGraph::Box::empty;
	const SceneGraph::GroupNode::ChildList& children=navigationalRoot->getChildren();
	for(SceneGraph::GroupNode::ChildList::const_iterator cIt=children.begin();cIt!=children.end();++cIt)
		result.addBox((*cIt)->calcBoundingBox());
	return result;
	}

void SceneGraphManager::testNavigationalCollision(SceneGraph::SphereCollisionQuery& collisionQuery) const
	{
	/* Execute the collision query on all children of the navigational-space scene graph root: */
	const SceneGraph::GroupNode::ChildList& children=navigationalRoot->getChildren();
	for(SceneGraph::GroupNode::ChildList::const_iterator cIt=children.begin();cIt!=children.end();++cIt)
		if((*cIt)->participatesInPass(SceneGraph::GraphNode::CollisionPass))
			(*cIt)->testCollision(collisionQuery);
	}

}
