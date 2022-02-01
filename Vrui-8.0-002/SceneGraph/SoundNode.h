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

#ifndef SCENEGRAPH_SOUNDNODE_INCLUDED
#define SCENEGRAPH_SOUNDNODE_INCLUDED

#include <Misc/Autopointer.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <AL/Config.h>
#include <AL/ALObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GraphNode.h>
#include <SceneGraph/AudioClipNode.h>

namespace SceneGraph {

class SoundNode:public GraphNode,public ALObject
	{
	/* Embedded classes: */
	public:
	typedef SF<AudioClipNodePointer> SFAudioClipNode;
	
	protected:
	struct DataItem:public ALObject::DataItem
		{
		/* Elements: */
		ALuint sourceId; // ID of the audio source playing back the audio clip
		ALuint bufferId; // ID of the audio buffer which the audio source is currently playing
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		
		/* Methods from class ALObject::DataItem: */
		virtual void shutdown(void);
		};
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFVector direction;
	SFFloat intensity;
  SFPoint location;
	SFFloat maxBack;
	SFFloat maxFront;
	SFFloat minBack;
	SFFloat minFront;
	SFFloat priority;
	SFAudioClipNode source;
	SFBool spatialize;
	
	/* Constructors and destructors: */
	SoundNode(void); // Creates a sound node with no associated audio clip
	virtual ~SoundNode(void);
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual unsigned int update(void);
	
	/* Methods from class GraphNode: */
	virtual void alRenderAction(ALRenderState& renderState) const;
	
	/* Methods from class ALObject: */
	virtual void initContext(ALContextData& contextData) const;
	};

typedef Misc::Autopointer<SoundNode> SoundNodePointer;

}

#endif
