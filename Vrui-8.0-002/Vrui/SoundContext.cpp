/***********************************************************************
SoundContext - Class for OpenAL contexts that are used to map a listener
to an OpenAL sound device.
Copyright (c) 2008-2021 Oliver Kreylos

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

#include <Vrui/SoundContext.h>

#include <stdio.h>
#include <string>
#include <iostream>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <AL/Config.h>
#if ALSUPPORT_CONFIG_HAVE_OPENAL
#ifdef __APPLE__
#include <OpenAL/alc.h>
#else
#include <AL/alc.h>
#endif
#endif
#include <AL/ALTemplates.h>
#include <AL/ALGeometryWrappers.h>
#include <AL/ALContextData.h>
#include <SceneGraph/ALRenderState.h>
#include <Vrui/Vrui.h>
#include <Vrui/Listener.h>
#include <Vrui/Internal/Vrui.h>

namespace Misc {

/**************************************************
Helper class to decode distance attenuation models:
**************************************************/

template <>
class ValueCoder<Vrui::SoundContext::DistanceAttenuationModel>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::SoundContext::DistanceAttenuationModel& value)
		{
		switch(value)
			{
			case Vrui::SoundContext::CONSTANT:
				return "Constant";
			
			case Vrui::SoundContext::INVERSE:
				return "Inverse";
			
			case Vrui::SoundContext::INVERSE_CLAMPED:
				return "InverseClamped";
			
			case Vrui::SoundContext::LINEAR:
				return "Linear";
			
			case Vrui::SoundContext::LINEAR_CLAMPED:
				return "LinearClamped";
			
			case Vrui::SoundContext::EXPONENTIAL:
				return "Exponential";
			
			case Vrui::SoundContext::EXPONENTIAL_CLAMPED:
				return "ExponentialClamped";
			}
		
		/* Never reached; just to make compiler happy: */
		return "";
		}
	static Vrui::SoundContext::DistanceAttenuationModel decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		if(end-start>=8&&strncasecmp(start,"Constant",8)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+8;
			return Vrui::SoundContext::CONSTANT;
			}
		else if(end-start>=14&&strncasecmp(start,"InverseClamped",14)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+14;
			return Vrui::SoundContext::INVERSE_CLAMPED;
			}
		else if(end-start>=7&&strncasecmp(start,"Inverse",7)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+7;
			return Vrui::SoundContext::INVERSE;
			}
		else if(end-start>=13&&strncasecmp(start,"LinearClamped",13)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+13;
			return Vrui::SoundContext::LINEAR_CLAMPED;
			}
		else if(end-start>=6&&strncasecmp(start,"Linear",6)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+6;
			return Vrui::SoundContext::LINEAR;
			}
		else if(end-start>=18&&strncasecmp(start,"ExponentialClamped",18)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+18;
			return Vrui::SoundContext::EXPONENTIAL_CLAMPED;
			}
		else if(end-start>=11&&strncasecmp(start,"Exponential",11)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+11;
			return Vrui::SoundContext::EXPONENTIAL;
			}
		else
			throw DecodingError(std::string("Unable to convert \"")+std::string(start,end)+std::string("\" to SoundContext::DistanceAttenuationModel"));
		}
	};

}

/* Constants from ALC_SOFT_HRTF OpenAL extension: */
#define ALC_HRTF_SOFT 0x1992
#define ALC_HRTF_STATUS_SOFT 0x1993
#define ALC_NUM_HRTF_SPECIFIERS_SOFT 0x1994
#define ALC_HRTF_SPECIFIER_SOFT 0x1995
#define ALC_HRTF_ID_SOFT 0x1996

#define ALC_DONT_CARE_SOFT 0x0002

#define ALC_HRTF_DISABLED_SOFT 0x0000
#define ALC_HRTF_ENABLED_SOFT 0x0001
#define ALC_HRTF_DENIED_SOFT 0x0002
#define ALC_HRTF_REQUIRED_SOFT 0x0003
#define ALC_HRTF_HEADPHONES_DETECTED_SOFT 0x0004
#define ALC_HRTF_UNSUPPORTED_FORMAT_SOFT 0x0005

/* Entry points from ALC_SOFT_HRTF OpenAL extension: */
typedef const ALCchar* (*PFNALCGETSTRINGISOFTPROC)(ALCdevice* device,ALCenum paramName,ALCsizei index);
typedef ALCboolean (*PFNALCRESETDEVICESOFTPROC)(ALCdevice* device,const ALCint* attrList);

namespace Vrui {

/*****************************
Methods of class SoundContext:
*****************************/

SoundContext::SoundContext(const Misc::ConfigurationFileSection& configFileSection,VruiState* sVruiState)
	:vruiState(sVruiState),
	 #if ALSUPPORT_CONFIG_HAVE_OPENAL
	 alDevice(0),alContext(0),
	 #endif
	 contextData(0),renderState(0),
	 listener(findListener(configFileSection.retrieveString("./listenerName").c_str())),
	 speedOfSound(float(getMeterFactor())*343.0f),
	 dopplerFactor(1.0f),
	 distanceAttenuationModel(CONSTANT),referenceDistance(float(getDisplaySize()*Scalar(2))),rolloffFactor(1.0f),
	 recordingDeviceName(configFileSection.retrieveString("./recordingDeviceName","Default"))
	{
	/* Set sound context parameters from configuration file: */
	speedOfSound=configFileSection.retrieveValue<float>("./speedOfSound",speedOfSound);
	dopplerFactor=configFileSection.retrieveValue<float>("./dopplerFactor",dopplerFactor);
	distanceAttenuationModel=configFileSection.retrieveValue<DistanceAttenuationModel>("./distanceAttenuationModel",distanceAttenuationModel);
	referenceDistance=configFileSection.retrieveValue<float>("./referenceDistance",referenceDistance);
	rolloffFactor=configFileSection.retrieveValue<float>("./rolloffFactor",rolloffFactor);
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	if(vruiVerbose&&alcIsExtensionPresent(0,"ALC_ENUMERATE_ALL_EXT"))
		{
		/* Enumerate all OpenAL devices: */
		const ALchar* devices=alcGetString(0,ALC_ALL_DEVICES_SPECIFIER);
		std::cout<<"\tOpenAL device names:"<<std::endl;
		const ALCchar* dPtr=devices;
		while(*dPtr!='\0')
			{
			const ALCchar* dEnd;
			for(dEnd=dPtr;*dEnd!='\0';++dEnd)
				;
			std::cout<<"\t\t"<<dPtr<<std::endl;
			dPtr=dEnd+1;
			}
		}
	
	/* Open the OpenAL device: */
	std::string alDeviceName=configFileSection.retrieveString("./deviceName","Default");
	alDevice=alcOpenDevice(alDeviceName!="Default"?alDeviceName.c_str():0);
	if(alDevice==0)
		Misc::throwStdErr("SoundContext::SoundContext: Could not open OpenAL sound device \"%s\"",alDeviceName.c_str());
	
	/* Check if the OpenAL device supports head-related transfer functions: */
	bool supportsHrtf=alcIsExtensionPresent(alDevice,"ALC_SOFT_HRTF");
	PFNALCGETSTRINGISOFTPROC alcGetStringiSOFTProc=0;
	if(supportsHrtf&&vruiVerbose)
		{
		/* Retrieve extension function pointers: */
		alcGetStringiSOFTProc=(PFNALCGETSTRINGISOFTPROC)(alcGetProcAddress(alDevice,"alcGetStringiSOFT"));
		
		/* Print a list of supported HRTF models: */
		ALCint numHrtfs=0;
		alcGetIntegerv(alDevice,ALC_NUM_HRTF_SPECIFIERS_SOFT,1,&numHrtfs);
		std::cout<<"\tSupported head-related transfer functions:"<<std::endl;
		for(ALCint i=0;i<numHrtfs;++i)
			{
			const ALCchar* hrtfName=alcGetStringiSOFTProc(alDevice,ALC_HRTF_SPECIFIER_SOFT,i);
			std::cout<<"\t\t"<<hrtfName<<std::endl;
			}
		}
	
	/* Create a list of context attributes: */
	ALCint alContextAttributes[9];
	ALCint* attPtr=alContextAttributes;
	if(configFileSection.hasTag("./mixerFrequency"))
		{
		*(attPtr++)=ALC_FREQUENCY;
		*(attPtr++)=configFileSection.retrieveValue<ALCint>("./mixerFrequency");
		}
	if(configFileSection.hasTag("./refreshFrequency"))
		{
		*(attPtr++)=ALC_REFRESH;
		*(attPtr++)=configFileSection.retrieveValue<ALCint>("./refreshFrequency");
		}
	if(configFileSection.hasTag("./numMonoSources"))
		{
		*(attPtr++)=ALC_MONO_SOURCES;
		*(attPtr++)=configFileSection.retrieveValue<ALCint>("./numMonoSources");
		}
	if(configFileSection.hasTag("./numStereoSources"))
		{
		*(attPtr++)=ALC_STEREO_SOURCES;
		*(attPtr++)=configFileSection.retrieveValue<ALCint>("./numStereoSources");
		}
	if(supportsHrtf)
		{
		if(configFileSection.hasTag("./useHrtf"))
			{
			*(attPtr++)=ALC_HRTF_SOFT;
			*(attPtr++)=configFileSection.retrieveValue<bool>("./useHrtf")?ALC_TRUE:ALC_FALSE;
			}
		if(configFileSection.hasTag("./hrtfModel"))
			{
			/* Retrieve the requested model name: */
			std::string hrtfModel=configFileSection.retrieveString("./hrtfModel");
			
			/* Find the index of the requested model name: */
			ALCint numHrtfs=0;
			alcGetIntegerv(alDevice,ALC_NUM_HRTF_SPECIFIERS_SOFT,1,&numHrtfs);
			ALCint hrtfIndex;
			for(hrtfIndex=0;hrtfIndex<numHrtfs;++hrtfIndex)
				{
				const ALCchar* hrtfName=alcGetStringiSOFTProc(alDevice,ALC_HRTF_SPECIFIER_SOFT,hrtfIndex);
				if(hrtfModel==hrtfName)
					break;
				}
			if(hrtfIndex<numHrtfs)
				{
				*(attPtr++)=ALC_HRTF_ID_SOFT;
				*(attPtr++)=hrtfIndex;
				}
			else
				{
				alcCloseDevice(alDevice);
				Misc::throwStdErr("SoundContext::SoundContext: Requested HRTF model %s not found",hrtfModel.c_str());
				}
			}
		}
	*(attPtr++)=ALC_INVALID;
	
	/* Create an OpenAL context: */
	alContext=alcCreateContext(alDevice,alContextAttributes);
	if(alContext==0)
		{
		alcCloseDevice(alDevice);
		Misc::throwStdErr("SoundContext::SoundContext: Could not create OpenAL context for sound device %s",alDeviceName.c_str());
		}
	
	if(vruiVerbose)
		{
		/* Print basic info about the OpenAL context: */
		ALCint frequency,refresh;
		alcGetIntegerv(alDevice,ALC_FREQUENCY,1,&frequency);
		std::cout<<"\tOpenAL mixer frequency: "<<frequency<<" Hz"<<std::endl;
		alcGetIntegerv(alDevice,ALC_REFRESH,1,&refresh);
		std::cout<<"\tOpenAL mixer refresh rate: "<<refresh<<" Hz"<<std::endl;
		
		if(supportsHrtf)
			{
			/* Check if head-related transfer functions are enabled: */
			ALCint hrtfEnabled;
			alcGetIntegerv(alDevice,ALC_HRTF_SOFT,1,&hrtfEnabled);
			std::cout<<"\tHead-related transfer functions "<<(hrtfEnabled==ALC_TRUE?"enabled":"disabled")<<std::endl;
			ALCint hrtfStatus;
			alcGetIntegerv(alDevice,ALC_HRTF_STATUS_SOFT,1,&hrtfStatus);
			switch(hrtfStatus)
				{
				case ALC_HRTF_DISABLED_SOFT:
					std::cout<<"\tHead-related transfer functions are disabled"<<std::endl;
					break;
				
				case ALC_HRTF_ENABLED_SOFT:
					std::cout<<"\tHead-related transfer functions are enabled"<<std::endl;
					break;
				
				case ALC_HRTF_DENIED_SOFT:
					std::cout<<"\tHead-related transfer functions are not allowed on the selected device"<<std::endl;
					break;
				
				case ALC_HRTF_REQUIRED_SOFT:
					std::cout<<"\tHead-related transfer functions are required on the selected device"<<std::endl;
					break;
				
				case ALC_HRTF_HEADPHONES_DETECTED_SOFT:
					std::cout<<"\tHead-related transfer enabled because selected device uses headphones"<<std::endl;
					break;
				
				case ALC_HRTF_UNSUPPORTED_FORMAT_SOFT:
					std::cout<<"\tHead-related transfer functions are incompatible with device's current format"<<std::endl;
					break;
				
				default:
					std::cout<<"\tUnknown HRTF status response"<<std::endl;
				}
			if(hrtfEnabled)
				{
				/* Query the name of the HRTF model: */
				const ALCchar* hrtfModel=alcGetString(alDevice,ALC_HRTF_SPECIFIER_SOFT);
				std::cout<<"\tHead-related transfer function: "<<hrtfModel<<std::endl;
				}
			}
		else
			std::cout<<"\tHead-related transfer functions not supported"<<std::endl;
		}
	
	#endif
	
	/* Create an AL context data object: */
	contextData=new ALContextData(101);
	
	/* Initialize per-source settings: */
	contextData->setAttenuation(referenceDistance,rolloffFactor);
	
	/* Initialize the sound context's OpenAL context: */
	makeCurrent();
	
	/* Create a scene graph traversal state: */
	renderState=new SceneGraph::ALRenderState(*contextData);
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Set global OpenAL parameters: */
	alSpeedOfSound(speedOfSound);
	alDopplerFactor(dopplerFactor);
	switch(distanceAttenuationModel)
		{
		case CONSTANT:
			alDistanceModel(AL_NONE);
			break;
		
		case INVERSE:
			alDistanceModel(AL_INVERSE_DISTANCE);
			break;
		
		case INVERSE_CLAMPED:
			alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
			break;
		
		case LINEAR:
			alDistanceModel(AL_LINEAR_DISTANCE);
			break;
		
		case LINEAR_CLAMPED:
			alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
			break;
		
		case EXPONENTIAL:
			alDistanceModel(AL_EXPONENT_DISTANCE);
			break;
		
		case EXPONENTIAL_CLAMPED:
			alDistanceModel(AL_EXPONENT_DISTANCE_CLAMPED);
			break;
		}
	
	#endif
	}

SoundContext::~SoundContext(void)
	{
	delete renderState;
	ALContextData::makeCurrent(0);
	delete contextData;
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	if(alcGetCurrentContext()==alContext)
		alcMakeContextCurrent(0);
	alcDestroyContext(alContext);
	if(!alcCloseDevice(alDevice))
		{
		fprintf(stderr,"SoundContext::~SoundContext: Failure in alcCloseDevice!\n");
		fflush(stderr);
		}
	
	#endif
	}

void SoundContext::makeCurrent(void)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Activate the sound context's OpenAL context: */
	if(alcGetCurrentContext()!=alContext)
		alcMakeContextCurrent(alContext);
	
	#endif
	
	/* Install the sound context's AL context data manager: */
	ALContextData::makeCurrent(contextData);
	}

void SoundContext::draw(void)
	{
	makeCurrent();
	
	/* Update things in the sound context's AL context data: */
	contextData->updateThings();
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Set the listener in physical coordinates: */
	contextData->resetMatrixStack();
	contextData->setListenerPosition(listener->getHeadPosition());
	contextData->setUpVector(vruiState->upDirection);
	alListenerPosition(listener->getHeadPosition());
	alListenerVelocity(Vector::zero);
	alListenerOrientation(listener->getListenDirection(),listener->getUpDirection());
	alListenerGain(listener->getGain());
	
	/* Render Vrui state: */
	vruiState->sound(*renderState);
	
	/* Check for OpenAL errors: */
	ALenum error;
	ALContextData::Error alcdError=ALContextData::NO_ERROR;
	while((error=alGetError())!=AL_NO_ERROR||(alcdError=contextData->getError())!=ALContextData::NO_ERROR)
		{
		printf("AL error: ");
		switch(error)
			{
			case AL_INVALID_ENUM:
				printf("Invalid enum");
				break;
			
			case AL_INVALID_NAME:
				printf("Invalid name");
				break;
			
			case AL_INVALID_OPERATION:
				printf("Invalid operation");
				break;
			
			case AL_INVALID_VALUE:
				printf("Invalid value");
				break;
			
			case AL_OUT_OF_MEMORY:
				printf("Out of memory");
				break;
			
			default:
				;
			}
		switch(alcdError)
			{
			case ALContextData::STACK_OVERFLOW:
				printf("Stack overflow");
				break;
			
			case ALContextData::STACK_UNDERFLOW:
				printf("Stack underflow");
				break;
			
			default:
				;
			}
		printf("\n");
		}
	
	#endif
	}

}
