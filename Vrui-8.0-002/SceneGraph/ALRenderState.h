/***********************************************************************
ALRenderState - Class encapsulating the traversal state of a scene graph
during OpenAL rendering.
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

#ifndef SCENEGRAPH_ALRENDERSTATE_INCLUDED
#define SCENEGRAPH_ALRENDERSTATE_INCLUDED

#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <AL/Config.h>
#include <AL/ALContextData.h>
#include <SceneGraph/TraversalState.h>

namespace SceneGraph {

class ALRenderState:public TraversalState
	{
	/* Embedded classes: */
	private:
	typedef Misc::HashTable<ALuint,unsigned int> SourcePassMap; // Type for hash tables mapping OpenAL source IDs to traversal pass counters
	
	/* Elements: */
	public:
	ALContextData& contextData; // Context data of the current OpenAL context
	private:
	unsigned int passCounter; // Counter for traversal passes
	SourcePassMap sourcePassMap; // Map of currently playing OpenAL sources and the traversal passes in which they were last used
	
	/* Constructors and destructors: */
	public:
	ALRenderState(ALContextData& sContextData); // Creates a render state for the given OpenAL context with an empty source set
	~ALRenderState(void); // Stops all still-playing sound sources and destroys the render state
	
	/* Methods from class TraversalState: */
	void startTraversal(const Point& newBaseViewerPos,const Vector& newBaseUpVector); // Starts a new scene graph traversal from physical space
	
	/* New methods: */
	
	/* Traversal scoping: */
	void useSource(ALuint sourceId); // Notifies the render state that the given OpenAL sound source is being used in the current traversal pass
	void endTraversal(void); // Stops all still-playing sources that were not used in the current traversal pass
	
	/* Convenience methods to set OpenAL state: */
	void sourceReferenceDistance(ALuint sourceId,ALfloat referenceDistance) // Sets the given source's reference distance in current model space units
		{
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Scale the reference distance to ear space and upload it to OpenAL: */
		alSourcef(sourceId,AL_REFERENCE_DISTANCE,ALfloat(currentTransform.getScaling())*referenceDistance);
		#endif
		}
	void sourceMaxDistance(ALuint sourceId,ALfloat maxDistance) // Sets the given source's maximum distance in current model space units
		{
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Scale the maximum distance to ear space and upload it to OpenAL: */
		alSourcef(sourceId,AL_MAX_DISTANCE,ALfloat(currentTransform.getScaling())*maxDistance);
		#endif
		}
	void sourcePosition(ALuint sourceId,const Point& position) // Sets the given source's position in current model space
		{
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Transform the position to ear space and upload it to OpenAL: */
		DOGTransform::Point earPosition=currentTransform.transform(position);
		alSource3f(sourceId,AL_POSITION,ALfloat(earPosition[0]),ALfloat(earPosition[1]),ALfloat(earPosition[2]));
		#endif
		}
	void sourceVelocity(ALuint sourceId,const Vector& velocity) // Sets the given source's velocity in current model space
		{
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Transform the velocity to ear space and upload it to OpenAL: */
		DOGTransform::Vector earVelocity=currentTransform.transform(velocity);
		alSource3f(sourceId,AL_VELOCITY,ALfloat(earVelocity[0]),ALfloat(earVelocity[1]),ALfloat(earVelocity[2]));
		#endif
		}
	void sourceDirection(ALuint sourceId,const Vector& direction) // Sets the given source's direction in current model space
		{
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Transform the direction to ear space and upload it to OpenAL: */
		DOGTransform::Vector earDirection=currentTransform.transform(direction);
		alSource3f(sourceId,AL_DIRECTION,ALfloat(earDirection[0]),ALfloat(earDirection[1]),ALfloat(earDirection[2]));
		#endif
		}
	};

}

#endif
