/***********************************************************************
AudioClipNode - Class for audio clips that can be played by Sound nodes.
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

#include <SceneGraph/AudioClipNode.h>

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <AL/ALContextData.h>
#include <Sound/SoundDataFormat.h>
#include <Sound/WAVFile.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/ALRenderState.h>

namespace SceneGraph {

/****************************************
Methods of class AudioClipNode::DataItem:
****************************************/

AudioClipNode::DataItem::DataItem(void)
	:bufferId(0),version(0)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Create an audio buffer: */
	alGenBuffers(1,&bufferId);
	
	#endif
	}

AudioClipNode::DataItem::~DataItem(void)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Destroy the audio buffer: */
	alDeleteBuffers(1,&bufferId);
	
	#endif
	}

/**************************************
Static elements of class AudioClipNode:
**************************************/

const char* AudioClipNode::className="AudioClip";

/******************************
Methods of class AudioClipNode:
******************************/

AudioClipNode::AudioClipNode(void)
	:loop(false),pitch(1),startTime(0),stopTime(0),
	 version(0)
	{
	}

const char* AudioClipNode::getClassName(void) const
	{
	return className;
	}

void AudioClipNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"description")==0)
		vrmlFile.parseField(description);
	else if(strcmp(fieldName,"loop")==0)
		vrmlFile.parseField(loop);
	else if(strcmp(fieldName,"pitch")==0)
		vrmlFile.parseField(pitch);
	else if(strcmp(fieldName,"startTime")==0)
		vrmlFile.parseField(startTime);
	else if(strcmp(fieldName,"stopTime")==0)
		vrmlFile.parseField(stopTime);
	else if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		/* Remember the VRML file's base directory: */
		baseDirectory=&vrmlFile.getBaseDirectory();
		}
	else
		Node::parseField(fieldName,vrmlFile);
	}

unsigned int AudioClipNode::update(void)
	{
	/* Clamp pitch field: */
	if(pitch.getValue()<Scalar(1)/Scalar(256))
		pitch.setValue(Scalar(1)/Scalar(256));
	
	/* Bump up the sound waveform's version number: */
	++version;
	
	return NoCascade;
	}

void AudioClipNode::initContext(ALContextData& contextData) const
	{
	/* Create a data item and store it in the AL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

void AudioClipNode::setUrl(const std::string& newUrl,IO::Directory& newBaseDirectory)
	{
	/* Store the URL and its base directory: */
	url.setValue(newUrl);
	baseDirectory=&newBaseDirectory;
	
	/* Bump up the texture's version number: */
	++version;
	}

void AudioClipNode::setUrl(const std::string& newUrl)
	{
	/* Store the URL and the current directory: */
	url.setValue(newUrl);
	baseDirectory=IO::Directory::getCurrent();
	
	/* Bump up the texture's version number: */
	++version;
	}

ALuint AudioClipNode::getBufferObject(ALRenderState& renderState) const
	{
	if(url.getNumValues()>0)
		{
		/* Acess the context data item: */
		DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
		
		/* Check if the buffer object needs to be updated: */
		if(dataItem->version!=version)
			{
			/* Find the file name extension of the first given URL: */
			const std::string& url1=url.getValue(0);
			std::string::const_iterator extIt=url1.end();
			for(std::string::const_iterator uIt=url1.begin();uIt!=url1.end();++uIt)
				if(*uIt=='/')
					extIt=url1.end();
				else if(*uIt=='.')
					extIt=uIt;
			
			/* Check the format of the requested audio file: */
			std::string ext(extIt,url1.end());
			if(strcasecmp(ext.c_str(),".wav")==0)
				{
				/* Open the WAV file: */
				Sound::WAVFile wav(baseDirectory->openFile(url1.c_str()));
				
				/* Check if the WAV file's sound format is OpenAL-compatible: */
				const Sound::SoundDataFormat& sdf=wav.getFormat();
				size_t frameSize=sdf.bytesPerSample*sdf.samplesPerFrame;
				ALenum bufferFormat=AL_NONE;
				if(sdf.bitsPerSample==8&&!sdf.signedSamples)
					{
					if(sdf.samplesPerFrame==1&&frameSize==1)
						bufferFormat=AL_FORMAT_MONO8;
					else if(sdf.samplesPerFrame==2&&frameSize==2)
						bufferFormat=AL_FORMAT_STEREO8;
					}
				else if(sdf.bitsPerSample==16&&sdf.signedSamples&&sdf.sampleEndianness==Sound::SoundDataFormat::LittleEndian)
					{
					if(sdf.samplesPerFrame==1&&frameSize==2)
						bufferFormat=AL_FORMAT_MONO16;
					else if(sdf.samplesPerFrame==2&&frameSize==4)
						bufferFormat=AL_FORMAT_STEREO16;
					}
				if(bufferFormat==AL_NONE)
					Misc::throwStdErr("SceneGraph::AudioClipNode::getBufferObject: Sound file %s has unsupported sound data format",baseDirectory->getPath(url1.c_str()));
				
				/* Read the WAV file's contents into an OpenAL buffer: */
				size_t numFrames=wav.getNumAudioFrames();
				
				void* buffer=malloc(numFrames*frameSize);
				wav.readAudioFrames(buffer,numFrames);
				alBufferData(dataItem->bufferId,bufferFormat,buffer,numFrames*frameSize,sdf.framesPerSecond);
				free(buffer);
				}
			else
				Misc::throwStdErr("SceneGraph::AudioClipNode::getBufferObject: Sound file %s has unsupported file format",baseDirectory->getPath(url1.c_str()));
			
			/* Mark the buffer object as up-to-date: */
			dataItem->version=version;
			}
		
		return dataItem->bufferId;
		}
	else
		return 0;
	}

}
