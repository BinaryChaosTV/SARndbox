/***********************************************************************
SoundNode - Class for nodes playing back audio clips.
Copyright (c) 2021 Oliver Kreylos

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

#include <SceneGraph/SoundNode.h>

#include <string.h>
#include <Math/Math.h>
#include <AL/ALContextData.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/ALRenderState.h>

namespace SceneGraph {

/************************************
Methods of class SoundNode::DataItem:
************************************/

SoundNode::DataItem::DataItem(void)
	:sourceId(0),bufferId(0)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Create an audio source: */
	alGenSources(1,&sourceId);
	
	#endif
	}

SoundNode::DataItem::~DataItem(void)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Destroy the audio source: */
	alDeleteSources(1,&sourceId);
	
	#endif
	}

void SoundNode::DataItem::shutdown(void)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	alSourcei(sourceId,AL_BUFFER,0);
	
	#endif
	}

/**********************************
Static elements of class SoundNode:
**********************************/

const char* SoundNode::className="Sound";

/**************************
Methods of class SoundNode:
**************************/

SoundNode::SoundNode(void)
	:direction(Vector(0,0,1)),intensity(1),location(Point::origin),
	 maxBack(10),maxFront(10),minBack(1),minFront(1),
	 priority(0),spatialize(true)
	{
	/* Disable all processing until there is a source node: */
	passMask=0x0U;
	}

SoundNode::~SoundNode(void)
	{
	/* Release this node's context data item early to ensure that OpenAL sources are stopped and deleted before their buffers: */
	ALContextData::destroyThing(this);
	}

const char* SoundNode::getClassName(void) const
	{
	return className;
	}

void SoundNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"direction")==0)
		vrmlFile.parseField(direction);
	else if(strcmp(fieldName,"intensity")==0)
		vrmlFile.parseField(intensity);
	else if(strcmp(fieldName,"location")==0)
		vrmlFile.parseField(location);
	else if(strcmp(fieldName,"maxBack")==0)
		vrmlFile.parseField(maxBack);
	else if(strcmp(fieldName,"maxFront")==0)
		vrmlFile.parseField(maxFront);
	else if(strcmp(fieldName,"minBack")==0)
		vrmlFile.parseField(minBack);
	else if(strcmp(fieldName,"minFront")==0)
		vrmlFile.parseField(minFront);
	else if(strcmp(fieldName,"priority")==0)
		vrmlFile.parseField(priority);
	else if(strcmp(fieldName,"source")==0)
		vrmlFile.parseSFNode(source);
	else if(strcmp(fieldName,"spatialize")==0)
		vrmlFile.parseField(spatialize);
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

unsigned int SoundNode::update(void)
	{
	/* Clamp the intensity, distance, and priority fields: */
	intensity.setValue(Math::clamp(intensity.getValue(),Scalar(0),Scalar(1)));
	if(maxBack.getValue()<Scalar(0))
		maxBack.setValue(Scalar(0));
	if(maxFront.getValue()<Scalar(0))
		maxFront.setValue(Scalar(0));
	if(minBack.getValue()<Scalar(0))
		minBack.setValue(Scalar(0));
	if(minFront.getValue()<Scalar(0))
		minFront.setValue(Scalar(0));
	priority.setValue(Math::clamp(priority.getValue(),Scalar(0),Scalar(1)));
	
	/* Update the processing pass mask if there is a source: */
	return setPassMask(source.getValue()!=0?ALRenderPass:0x0U);
	}

void SoundNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Retrieve the OpenAL buffer object ID of the audio clip and bail out if the buffer is invalid: */
	ALuint bufferId=source.getValue()->getBufferObject(renderState);
	if(bufferId==0)
		return;
	
	/* Retrieve the OpenAL context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Set up the audio source: */
	alSourcei(dataItem->sourceId,AL_LOOPING,source.getValue()->loop.getValue()?AL_TRUE:AL_FALSE);
	alSourcef(dataItem->sourceId,AL_PITCH,source.getValue()->pitch.getValue());
	alSourcef(dataItem->sourceId,AL_GAIN,intensity.getValue());
	
	/* Calculate reference and maximum distances based on VRML 97's attenuation model: */
	Scalar refDist=Math::mid(maxBack.getValue(),maxFront.getValue());
	renderState.sourceReferenceDistance(dataItem->sourceId,refDist);
	Scalar maxDist=Math::mid(minBack.getValue(),minFront.getValue());
	renderState.sourceMaxDistance(dataItem->sourceId,maxDist);
	
	/* Set the source's position in current model coordinates: */
	renderState.sourcePosition(dataItem->sourceId,location.getValue());
	
	/* Re-bind the audio source's buffer if the buffer has changed: */
	if(dataItem->bufferId!=bufferId)
		{
		/* Stop playing the source on the previous buffer: */
		if(dataItem->bufferId!=0)
			alSourceStop(dataItem->sourceId);
		
		/* Bind the new buffer: */
		alSourcei(dataItem->sourceId,AL_BUFFER,bufferId);
		dataItem->bufferId=bufferId;
		
		/* Start playing the source on the new buffer: */
		if(dataItem->bufferId!=0)
			alSourcePlay(dataItem->sourceId);
		}
	
	/* Notify the render state object that this source is in use: */
	renderState.useSource(dataItem->sourceId);
	
	#endif
	}

void SoundNode::initContext(ALContextData& contextData) const
	{
	/* Create a data item and store it in the AL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}
