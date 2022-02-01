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

#include <SceneGraph/ALRenderState.h>

#include <AL/Config.h>

namespace SceneGraph {

/******************************
Methods of class ALRenderState:
******************************/

ALRenderState::ALRenderState(ALContextData& sContextData)
	:contextData(sContextData),
	 passCounter(0),sourcePassMap(5)
	{
	}

ALRenderState::~ALRenderState(void)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Stop all still-playing sound sources: */
	for(SourcePassMap::Iterator spmIt=sourcePassMap.begin();!spmIt.isFinished();++spmIt)
		{
		/* Stop the source if it hasn't been destroyed yet: */
		if(alIsSource(spmIt->getSource()))
			alSourceStop(spmIt->getSource());
		}
	
	#endif
	}

void ALRenderState::startTraversal(const Point& newBaseViewerPos,const Vector& newBaseUpVector)
	{
	/* Call the base class method: */
	TraversalState::startTraversal(DOGTransform::identity,newBaseViewerPos,newBaseUpVector);
	
	/* Advance the pass counter for this scene graph traversal: */
	++passCounter;
	}

void ALRenderState::useSource(unsigned int sourceId)
	{
	/* Add the source to the source pass mask and check if it was in there before: */
	if(!sourcePassMap.setEntry(SourcePassMap::Entry(sourceId,passCounter)))
		{
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		
		/* Start the source: */
		alSourcePlay(sourceId);
		
		#endif
		}
	}

void ALRenderState::endTraversal(void)
	{
	/* Stop and remove all sources whose pass counter does not match the current pass counter: */
	for(SourcePassMap::Iterator spmIt=sourcePassMap.begin();!spmIt.isFinished();)
		{
		/* Advance the map iterator to keep it from getting invalidated: */
		SourcePassMap::Iterator nextSpmIt=spmIt;
		++nextSpmIt;
		
		if(spmIt->getDest()!=passCounter)
			{
			#if ALSUPPORT_CONFIG_HAVE_OPENAL
			
			/* Stop the source if it hasn't been destroyed yet: */
			if(alIsSource(spmIt->getSource()))
				alSourceStop(spmIt->getSource());
			
			#endif
			
			/* Remove the source's entry, which will invalidate the iterator: */
			sourcePassMap.removeEntry(spmIt);
			}
		
		/* Go to the next map entry: */
		spmIt=nextSpmIt;
		}
	}

}
