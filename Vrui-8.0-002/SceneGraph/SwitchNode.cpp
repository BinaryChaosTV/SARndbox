/***********************************************************************
SwitchNode - Class for group nodes that traverse zero or one of their
children based on a selection field.
Copyright (c) 2018-2021 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/SwitchNode.h>

#include <string.h>
#include <Geometry/Box.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

/***********************************
Static elements of class SwitchNode:
***********************************/

const char* SwitchNode::className="Switch";

/***************************
Methods of class SwitchNode:
***************************/

SwitchNode::SwitchNode(void)
	:whichChoice(-1)
	{
	}

const char* SwitchNode::getClassName(void) const
	{
	return className;
	}

EventOut* SwitchNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"choice")==0)
		return makeEventOut(this,choice);
	else if(strcmp(fieldName,"whichChoice")==0)
		return makeEventOut(this,whichChoice);
	else
		return GraphNode::getEventOut(fieldName);
	}

EventIn* SwitchNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"choice")==0)
		return makeEventIn(this,choice);
	else if(strcmp(fieldName,"whichChoice")==0)
		return makeEventIn(this,whichChoice);
	else
		return GraphNode::getEventIn(fieldName);
	}

void SwitchNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"choice")==0)
		{
		vrmlFile.parseMFNode(choice);
		}
	else if(strcmp(fieldName,"whichChoice")==0)
		{
		vrmlFile.parseField(whichChoice);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

unsigned int SwitchNode::update(void)
	{
	/* Set this node's pass mask to the current choice's pass mask or nothing if the choice is invalid : */
	PassMask newPassMask=0x0U;
	int wc=whichChoice.getValue();
	if(wc>=0&&wc<int(choice.getNumValues()))
		newPassMask=choice.getValue(wc)->getPassMask();
	return setPassMask(newPassMask);
	}

unsigned int SwitchNode::cascadingUpdate(Node& child,unsigned int childUpdateResult)
	{
	unsigned int result=NoCascade;
	
	/* Check if the current choice is valid, and the given node is the current choice: */
	int wc=whichChoice.getValue();
	if(wc>=0&&wc<int(choice.getNumValues())&&choice.getValue(wc)==static_cast<GraphNode*>(&child))
		{
		/* Set the pass mask to the current choice's new pass mask: */
		result=setPassMask(choice.getValue(wc)->getPassMask());
		}
	
	return result;
	}

Box SwitchNode::calcBoundingBox(void) const
	{
	/* Calculate the group's bounding box as the union of the choice's boxes: */
	Box result=Box::empty;
	for(MFGraphNode::ValueList::const_iterator cIt=choice.getValues().begin();cIt!=choice.getValues().end();++cIt)
		result.addBox((*cIt)->calcBoundingBox());
	return result;
	}

unsigned int SwitchNode::updatePassMask(void)
	{
	/* Tell the current choice, if it is valid, to update its pass mask and assign the result to this node: */
	PassMask newPassMask=0x0U;
	int wc=whichChoice.getValue();
	if(wc>=0&&wc<int(choice.getNumValues()))
		{
		choice.getValue(wc)->updatePassMask();
		newPassMask=choice.getValue(wc)->getPassMask();
		}
	return setPassMask(newPassMask);
	}

void SwitchNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Apply the collision query to the current choice (this wouldn't be called if the choice weren't valid): */
	choice.getValue(whichChoice.getValue())->testCollision(collisionQuery);
	}

void SwitchNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Call the render action of the current choice (this wouldn't be called if the choice weren't valid): */
	choice.getValue(whichChoice.getValue())->glRenderAction(renderState);
	}

void SwitchNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Call the render action of the current choice (this wouldn't be called if the choice weren't valid): */
	choice.getValue(whichChoice.getValue())->alRenderAction(renderState);
	}

}
