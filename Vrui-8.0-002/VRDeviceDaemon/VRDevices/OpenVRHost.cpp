/***********************************************************************
OpenVRHost - Class to wrap a low-level OpenVR tracking and display
device driver in a VRDevice.
Copyright (c) 2016-2021 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <VRDeviceDaemon/VRDevices/OpenVRHost.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <dlfcn.h>
#include <Misc/SizedTypes.h>
#include <Misc/StringPrintf.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Geometry/OutputOperators.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/HMDConfiguration.h>

#include <VRDeviceDaemon/VRDeviceManager.h>
#include <VRDeviceDaemon/VRDevices/OpenVRHost-Config.h>

/***********************************************************************
A fake implementation of SDL functions used by Valve's lighthouse
driver, to fool the driver into detecting a connected Vive HMD:
***********************************************************************/

/**********************************************
Inter-operability declarations from SDL2/SDL.h:
**********************************************/

extern "C" {

#ifndef DECLSPEC
	#if defined(__GNUC__) && __GNUC__ >= 4
		#define DECLSPEC __attribute__ ((visibility("default")))
	#else
		#define DECLSPEC
	#endif
#endif

#ifndef SDLCALL
#define SDLCALL
#endif

typedef struct
	{
	Misc::UInt32 format;
	int w;
	int h;
	int refresh_rate;
	void *driverdata;
	} SDL_DisplayMode;

typedef struct
	{
	int x,y;
	int w, h;
	} SDL_Rect;

DECLSPEC int SDLCALL SDL_GetNumVideoDisplays(void);
DECLSPEC int SDLCALL SDL_GetCurrentDisplayMode(int displayIndex,SDL_DisplayMode* mode);
DECLSPEC int SDLCALL SDL_GetDisplayBounds(int displayIndex,SDL_Rect* rect);
DECLSPEC const char* SDLCALL SDL_GetDisplayName(int displayIndex);

}

/******************
Fake SDL functions:
******************/

int SDL_GetNumVideoDisplays(void)
	{
	return 2; // Create two fake displays so the driver doesn't complain about the HMD being the primary
	}

int SDL_GetCurrentDisplayMode(int displayIndex,SDL_DisplayMode* mode)
	{
	memset(mode,0,sizeof(SDL_DisplayMode));
	mode->format=0x16161804U; // Hard-coded for SDL_PIXELFORMAT_RGB888
	if(displayIndex==1)
		{
		/* Return a fake Vive HMD: */
		mode->w=2160;
		mode->h=1200;
		mode->refresh_rate=89;
		mode->driverdata=0;
		}
	else
		{
		/* Return a fake monitor: */
		mode->w=1920;
		mode->h=1080;
		mode->refresh_rate=60;
		mode->driverdata=0;
		}
	
	return 0;
	}

int SDL_GetDisplayBounds(int displayIndex,SDL_Rect* rect)
	{
	if(displayIndex==1)
		{
		/* Return a fake Vive HMD: */
		rect->x=1920;
		rect->y=0;
		rect->w=2160;
		rect->h=1200;
		}
	else
		{
		/* Return a fake monitor: */
		rect->x=0;
		rect->y=0;
		rect->w=1920;
		rect->h=1080;
		}
	
	return 0;
	}

const char* SDL_GetDisplayName(int displayIndex)
	{
	if(displayIndex==1)
		return "HTC Vive 5\"";
	else
		return "Acme Inc. HD Display";
	}

namespace {

/****************
Helper functions:
****************/

const char* propErrorName(vr::ETrackedPropertyError error)
	{
	switch(error)
		{
		case vr::TrackedProp_Success:
			return "success";
		
		case vr::TrackedProp_WrongDataType:
			return "wrong data type";
		
		case vr::TrackedProp_WrongDeviceClass:
			return "wrong device class";
		
		case vr::TrackedProp_BufferTooSmall:
			return "buffer too small";
		
		case vr::TrackedProp_UnknownProperty:
			return "unknown property";
		
		case vr::TrackedProp_InvalidDevice:
			return "invalid device";
		
		case vr::TrackedProp_CouldNotContactServer:
			return "could not contact server";
		
		case vr::TrackedProp_ValueNotProvidedByDevice:
			return "value not provided by device";
		
		case vr::TrackedProp_StringExceedsMaximumLength:
			return "string exceeds maximum length";
		
		case vr::TrackedProp_NotYetAvailable:
			return "not yet available";
		
		case vr::TrackedProp_PermissionDenied:
			return "permission denied";
		
		case vr::TrackedProp_InvalidOperation:
			return "invalid operation";
		
		default:
			return "unknown error";
		}
	}

}

/**********************************************
Methods of class OpenVRHost::PropertyContainer:
**********************************************/

OpenVRHost::PropertyContainer::PropertyContainer(void)
	:properties(101)
	{
	}

OpenVRHost::PropertyContainer::~PropertyContainer(void)
	{
	/* Delete all property value buffers: */
	for(PropertyMap::Iterator pIt=properties.begin();!pIt.isFinished();++pIt)
		free(pIt->getDest().value);
	}

vr::ETrackedPropertyError OpenVRHost::PropertyContainer::read(vr::PropertyRead_t* prop) const
	{
	/* Find the requested property in the property map: */
	PropertyMap::ConstIterator pIt=properties.findEntry(prop->prop);
	if(!pIt.isFinished())
		{
		/* Initialize the property slot: */
		const Property& p=pIt->getDest();
		prop->unTag=vr::k_unInvalidPropertyTag;
		prop->unRequiredBufferSize=0;
		prop->eError=p.state;
		
		/* Check if the stored property is OK: */
		if(p.state==vr::TrackedProp_Success)
			{
			/* Initialize property type and size: */
			prop->unTag=p.typeTag;
			prop->unRequiredBufferSize=p.size;
			
			/* Check if the property slot has sufficient space to hold the property's value: */
			if(prop->unBufferSize>=p.size)
				{
				/* Copy the property's value: */
				memcpy(prop->pvBuffer,p.value,p.size);
				}
			else
				prop->eError=vr::TrackedProp_BufferTooSmall;
			}
		}
	else
		{
		/* Set the property slot to a (temporarily) undefined property: */
		prop->unTag=vr::k_unInvalidPropertyTag;
		prop->unRequiredBufferSize=0;
		prop->eError=vr::TrackedProp_UnknownProperty;
		}
	
	return prop->eError;
	}

vr::ETrackedPropertyError OpenVRHost::PropertyContainer::write(const vr::PropertyWrite_t* prop)
	{
	/* Check the property slot's write mode: */
	vr::ETrackedPropertyError result=vr::TrackedProp_Success;
	switch(prop->writeType)
		{
		case vr::PropertyWrite_Set:
			{
			/* Find the property in the map: */
			PropertyMap::Iterator pIt=properties.findEntry(prop->prop);
			if(pIt.isFinished())
				{
				/* Create a new property and update the iterator: */
				properties.setEntry(PropertyMap::Entry(prop->prop,Property()));
				pIt=properties.findEntry(prop->prop);
				}
			
			/* Set the property's state and value: */
			Property& p=pIt->getDest();
			p.state=vr::TrackedProp_Success;
			p.typeTag=prop->unTag;
			if(p.size<prop->unBufferSize)
				{
				free(p.value);
				p.value=malloc(prop->unBufferSize);
				}
			p.size=prop->unBufferSize;
			memcpy(p.value,prop->pvBuffer,p.size);
			
			break;
			}
		
		case vr::PropertyWrite_Erase:
			{
			/* Find the property in the map: */
			PropertyMap::Iterator pIt=properties.findEntry(prop->prop);
			if(!pIt.isFinished())
				{
				/* Release the property's value and remove it from the map: */
				free(pIt->getDest().value);
				properties.removeEntry(pIt);
				}
			
			break;
			}
		
		case vr::PropertyWrite_SetError:
			{
			/* Find the property in the map: */
			PropertyMap::Iterator pIt=properties.findEntry(prop->prop);
			if(pIt.isFinished())
				{
				/* Create a new property and update the iterator: */
				properties.setEntry(PropertyMap::Entry(prop->prop,Property()));
				pIt=properties.findEntry(prop->prop);
				}
			
			/* Set the property's state: */
			pIt->getDest().state=prop->eSetError;
			result=prop->eSetError;
			
			break;
			}
		}
	
	return result;
	}

namespace {

/**************
Helper classes:
**************/

template <class ValueParam>
class VRTypeTraits
	{
	};

template <>
class VRTypeTraits<float>
	{
	/* Elements: */
	public:
	static const vr::PropertyTypeTag_t tag=vr::k_unFloatPropertyTag;
	};

template <>
class VRTypeTraits<int32_t>
	{
	/* Elements: */
	public:
	static const vr::PropertyTypeTag_t tag=vr::k_unInt32PropertyTag;
	};

template <>
class VRTypeTraits<uint64_t>
	{
	/* Elements: */
	public:
	static const vr::PropertyTypeTag_t tag=vr::k_unUint64PropertyTag;
	};

template <>
class VRTypeTraits<bool>
	{
	/* Elements: */
	public:
	static const vr::PropertyTypeTag_t tag=vr::k_unBoolPropertyTag;
	};

template <>
class VRTypeTraits<std::string>
	{
	/* Elements: */
	public:
	static const vr::PropertyTypeTag_t tag=vr::k_unStringPropertyTag;
	};

}

template <class ValueParam>
inline bool OpenVRHost::PropertyContainer::get(vr::ETrackedDeviceProperty property,ValueParam& value) const
	{
	bool result=false;
	
	/* Find the property in the map: */
	PropertyMap::ConstIterator pIt=properties.findEntry(property);
	if(!pIt.isFinished())
		{
		const Property& p=pIt->getDest();
		
		/* Check if the property is valid, matches the requested result type, and is big enough: */
		if(p.state==vr::TrackedProp_Success&&p.typeTag==VRTypeTraits<ValueParam>::tag&&p.size>=sizeof(ValueParam))
			{
			/* Extract the value: */
			value=*static_cast<const ValueParam*>(p.value);
			result=true;
			}
		}
	
	return result;
	}

template <>
inline bool OpenVRHost::PropertyContainer::get(vr::ETrackedDeviceProperty property,std::string& value) const
	{
	bool result=false;
	
	/* Find the property in the map: */
	PropertyMap::ConstIterator pIt=properties.findEntry(property);
	if(!pIt.isFinished())
		{
		const Property& p=pIt->getDest();
		
		/* Check if the property is valid, matches the requested result type, and is big enough: */
		if(p.state==vr::TrackedProp_Success&&p.typeTag==vr::k_unStringPropertyTag&&p.size>=sizeof(char))
			{
			/* Extract the value: */
			value.clear();
			value.reserve(p.size);
			const char* vPtr=static_cast<const char*>(p.value);
			for(uint32_t i=0;i<p.size&&*vPtr!='\0';++i,++vPtr)
				value.push_back(*vPtr);
			result=true;
			}
		}
	
	return result;
	}

template <class ValueParam>
inline void OpenVRHost::PropertyContainer::set(vr::ETrackedDeviceProperty property,const ValueParam& value)
	{
	/* Find the property in the map: */
	PropertyMap::Iterator pIt=properties.findEntry(property);
	if(pIt.isFinished())
		{
		/* Create a new property and update the iterator: */
		properties.setEntry(PropertyMap::Entry(property,Property()));
		pIt=properties.findEntry(property);
		}
	
	/* Set the property: */
	Property& p=pIt->getDest();
	p.state=vr::TrackedProp_Success;
	p.typeTag=VRTypeTraits<ValueParam>::tag;
	if(p.size<sizeof(ValueParam))
		{
		free(p.value);
		p.value=malloc(sizeof(ValueParam));
		}
	p.size=sizeof(ValueParam);
	memcpy(p.value,&value,sizeof(ValueParam));
	}

template <>
inline void OpenVRHost::PropertyContainer::set(vr::ETrackedDeviceProperty property,const std::string& value)
	{
	/* Find the property in the map: */
	PropertyMap::Iterator pIt=properties.findEntry(property);
	if(pIt.isFinished())
		{
		/* Create a new property and update the iterator: */
		properties.setEntry(PropertyMap::Entry(property,Property()));
		pIt=properties.findEntry(property);
		}
	
	/* Set the property: */
	Property& p=pIt->getDest();
	p.state=vr::TrackedProp_Success;
	p.typeTag=vr::k_unStringPropertyTag;
	if(p.size<value.length()+1)
		{
		free(p.value);
		p.value=malloc(value.length()+1);
		}
	p.size=value.length()+1;
	memcpy(p.value,value.c_str(),value.length()+1);
	}

void OpenVRHost::PropertyContainer::print(vr::ETrackedDeviceProperty property) const
	{
	/* Find the property in the map: */
	PropertyMap::ConstIterator pIt=properties.findEntry(property);
	if(!pIt.isFinished())
		{
		const Property& p=pIt->getDest();
		if(p.state==vr::TrackedProp_Success)
			{
			switch(p.typeTag)
				{
				case vr::k_unFloatPropertyTag:
					{
					uint32_t numItems=p.size/sizeof(float);
					if(numItems!=1)
						printf("(float [%u])",numItems);
					else
						printf("(float)");
					const float* vPtr=static_cast<const float*>(p.value);
					for(uint32_t i=0;i<numItems;++i,++vPtr)
						printf(" %f",*vPtr);
					
					break;
					}
				
				case vr::k_unInt32PropertyTag:
					{
					uint32_t numItems=p.size/sizeof(int32_t);
					if(numItems!=1)
						printf("(int32 [%u])",numItems);
					else
						printf("(int32)");
					const int32_t* vPtr=static_cast<const int32_t*>(p.value);
					for(uint32_t i=0;i<numItems;++i,++vPtr)
						printf(" %d",*vPtr);
					
					break;
					}
				
				case vr::k_unUint64PropertyTag:
					{
					uint32_t numItems=p.size/sizeof(uint64_t);
					if(numItems!=1)
						printf("(uint64 [%u])",numItems);
					else
						printf("(uint64)");
					const uint64_t* vPtr=static_cast<const uint64_t*>(p.value);
					for(uint32_t i=0;i<numItems;++i,++vPtr)
						printf(" %lu",*vPtr);
					
					break;
					}
				
				case vr::k_unBoolPropertyTag:
					{
					uint32_t numItems=p.size/sizeof(bool);
					if(numItems!=1)
						printf("(bool [%u])",numItems);
					else
						printf("(bool)");
					const bool* vPtr=static_cast<const bool*>(p.value);
					for(uint32_t i=0;i<numItems;++i,++vPtr)
						printf(" %s",*vPtr?"true":"false");
					
					break;
					}
				
				case vr::k_unStringPropertyTag:
					{
					printf("(string) ");
					const char* vPtr=static_cast<const char*>(p.value);
					for(uint32_t i=1;i<p.size&&*vPtr!='\0';++i,++vPtr)
						fputc(*vPtr,stdout);
					
					break;
					}
				
				case vr::k_unDoublePropertyTag:
					{
					uint32_t numItems=p.size/sizeof(double);
					if(numItems!=1)
						printf("(double [%u])",numItems);
					else
						printf("(double)");
					const double* vPtr=static_cast<const double*>(p.value);
					for(uint32_t i=0;i<numItems;++i,++vPtr)
						printf(" %lf",*vPtr);
					
					break;
					}
				
				case vr::k_unHmdMatrix34PropertyTag:
					{
					uint32_t numItems=p.size/sizeof(vr::HmdMatrix34_t);
					if(numItems!=1)
						printf("(matrix3x4 [%u])",numItems);
					else
						printf("(matrix3x4)");
					const vr::HmdMatrix34_t* vPtr=static_cast<const vr::HmdMatrix34_t*>(p.value);
					for(uint32_t n=0;n<numItems;++n,++vPtr)
						{
						printf(" (");
						for(int i=0;i<3;++i)
							for(int j=0;j<4;++j)
								printf(" %f",vPtr->m[i][j]);
						printf(" )");
						}
					
					break;
					}
				
				case vr::k_unHmdVector3PropertyTag:
					{
					uint32_t numItems=p.size/sizeof(vr::HmdVector3_t);
					if(numItems!=1)
						printf("(vector3 [%u])",numItems);
					else
						printf("(vector3)");
					const vr::HmdVector3_t* vPtr=static_cast<const vr::HmdVector3_t*>(p.value);
					for(uint32_t n=0;n<numItems;++n,++vPtr)
						{
						printf(" (");
						for(int i=0;i<3;++i)
							printf(" %f",vPtr->v[i]);
						printf(" )");
						}
					
					break;
					}
				
				default:
					printf("(unknown type %u of size %u)",p.typeTag,p.size);
				}
			}
		else
			printf("(%s)",propErrorName(p.state));
		}
	else
		printf("(undefined)");
	}

void OpenVRHost::PropertyContainer::remove(vr::ETrackedDeviceProperty property)
	{
	/* Remove the property: */
	properties.removeEntry(property);
	}

/****************************************
Methods of class OpenVRHost::DeviceState:
****************************************/

OpenVRHost::DeviceState::DeviceState(void)
	:deviceType(NumDeviceTypes),
	 driver(0),display(0),
	 trackerIndex(-1),
	 willDriftInYaw(true),isWireless(false),hasProximitySensor(false),providesBatteryStatus(false),canPowerOff(false),
	 proximitySensorState(false),hmdConfiguration(0),
	 nextButtonIndex(0),numButtons(0),nextValuatorIndex(0),numValuators(0),nextHapticFeatureIndex(0),numHapticFeatures(0),
	 connected(false),tracked(false)
	{
	}

/***************************
Methods of class OpenVRHost:
***************************/

void OpenVRHost::log(int messageLevel,const char* formatString,...) const
	{
	if(messageLevel<=verbosity)
		{
		printf("OpenVRHost: ");
		va_list ap;
		va_start(ap,formatString);
		vprintf(formatString,ap);
		va_end(ap);
		fflush(stdout);
		}
	}

void OpenVRHost::updateHMDConfiguration(OpenVRHost::DeviceState& deviceState) const
	{
	Threads::Mutex::Lock hmdConfigurationLock(deviceManager->getHmdConfigurationMutex());
	
	/* Update recommended pre-distortion render target size: */
	uint32_t renderTargetSize[2];
	deviceState.display->GetRecommendedRenderTargetSize(&renderTargetSize[0],&renderTargetSize[1]);
	deviceState.hmdConfiguration->setRenderTargetSize(renderTargetSize[0],renderTargetSize[1]);
	
	/* Update per-eye state: */
	bool distortionMeshesUpdated=false;
	for(int eyeIndex=0;eyeIndex<2;++eyeIndex)
		{
		vr::EVREye eye=eyeIndex==0?vr::Eye_Left:vr::Eye_Right;
		
		/* Update output viewport: */
		uint32_t viewport[4];
		deviceState.display->GetEyeOutputViewport(eye,&viewport[0],&viewport[1],&viewport[2],&viewport[3]);
		deviceState.hmdConfiguration->setViewport(eyeIndex,viewport[0],viewport[1],viewport[2],viewport[3]);
		
		/* Update tangent-space FoV boundaries: */
		float fov[4];
		deviceState.display->GetProjectionRaw(eye,&fov[0],&fov[1],&fov[2],&fov[3]);
		deviceState.hmdConfiguration->setFov(eyeIndex,fov[0],fov[1],fov[2],fov[3]);
		
		/* Evaluate and update lens distortion correction formula: */
		const Vrui::HMDConfiguration::UInt* dmSize=deviceState.hmdConfiguration->getDistortionMeshSize();
		Vrui::HMDConfiguration::DistortionMeshVertex* dmPtr=deviceState.hmdConfiguration->getDistortionMesh(eyeIndex);
		for(unsigned int v=0;v<dmSize[1];++v)
			{
			float vf=float(v)/float(dmSize[1]-1);
			for(unsigned int u=0;u<dmSize[0];++u,++dmPtr)
				{
				/* Calculate the vertex's undistorted positions: */
				float uf=float(u)/float(dmSize[0]-1);
				vr::DistortionCoordinates_t out=deviceState.display->ComputeDistortion(eye,uf,vf);
				Vrui::HMDConfiguration::Point2 red(out.rfRed);
				Vrui::HMDConfiguration::Point2 green(out.rfGreen);
				Vrui::HMDConfiguration::Point2 blue(out.rfBlue);
				
				/* Check if there is actually a change: */
				distortionMeshesUpdated=distortionMeshesUpdated||dmPtr->red!=red||dmPtr->green!=green||dmPtr->blue!=blue;
				dmPtr->red=red;
				dmPtr->green=green;
				dmPtr->blue=blue;
				}
			}
		}
	if(distortionMeshesUpdated)
		deviceState.hmdConfiguration->updateDistortionMeshes();
	
	/* Tell the device manager that the HMD configuration was updated: */
	deviceManager->updateHmdConfiguration(deviceState.hmdConfiguration);
	}

void OpenVRHost::deviceThreadMethod(void)
	{
	/* Run the OpenVR driver's main loop to dispatch events: */
	while(true)
		{
		openvrTrackedDeviceProvider->RunFrame();
		usleep(threadWaitTime);
		}
	}

namespace {

/****************
Helper functions:
****************/

std::string pathcat(const std::string& prefix,const std::string& suffix) // Concatenates two partial paths if the suffix is not absolute
	{
	/* Check if the path suffix is relative: */
	if(suffix.empty()||suffix[0]!='/')
		{
		/* Return the concatenation of the two paths: */
		std::string result;
		result.reserve(prefix.size()+1+suffix.size());
		
		result=prefix;
		result.push_back('/');
		result.append(suffix);
		
		return result;
		}
	else
		{
		/* Return the path suffix unchanged: */
		return suffix;
		}
	}

}

OpenVRHost::OpenVRHost(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 verbosity(configFile.retrieveValue<int>("./verbosity",0)),
	 blockQueueHandles(17),nextBlockQueueHandle(0xa00000001UL),
	 pathHandles(17),nextPathHandle(0x10002afc0000003cUL),
	 openvrDriverDsoHandle(0),
	 openvrTrackedDeviceProvider(0),
	 ioBufferMap(17),lastIOBufferHandle(0),
	 openvrSettingsSection(configFile.getSection("Settings")),
	 driverHandle(0x200000003UL),deviceHandleBase(0x100000000UL),
	 printLogMessages(configFile.retrieveValue<bool>("./printLogMessages",false)),
	 threadWaitTime(configFile.retrieveValue<unsigned int>("./threadWaitTime",100000U)),
	 exiting(false),
	 configuredPostTransformations(0),numHapticFeatures(0),
	 deviceStates(0),hapticEvents(0),powerFeatureDevices(0),hmdConfiguration(0),
	 nextComponentHandle(1),componentFeatureIndices(0),valuatorStates(0)
	{
	/*********************************************************************
	First initialization step: Dynamically load the appropriate OpenVR
	driver shared library.
	*********************************************************************/
	
	/* Retrieve the Steam root directory: */
	std::string steamRootDir;
	if(strncmp(VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMDIR,"$HOME/",6)==0)
		{
		const char* homeDir=getenv("HOME");
		if(homeDir!=0)
			steamRootDir=homeDir;
		steamRootDir.append(VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMDIR+5);
		}
	else
		steamRootDir=VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMDIR;
	steamRootDir=configFile.retrieveString("./steamRootDir",steamRootDir);
	
	/* Construct the OpenVR root directory: */
	openvrRootDir=VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMVRDIR;
	openvrRootDir=configFile.retrieveString("./openvrRootDir",openvrRootDir);
	openvrRootDir=pathcat(steamRootDir,openvrRootDir);
	
	/* Retrieve the name of the OpenVR device driver: */
	std::string openvrDriverName=configFile.retrieveString("./openvrDriverName","lighthouse");
	
	/* Retrieve the directory containing the OpenVR device driver: */
	openvrDriverRootDir=VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMVRDIR;
	openvrDriverRootDir.append("/drivers/");
	openvrDriverRootDir.append(openvrDriverName);
	openvrDriverRootDir.append("/bin/linux64");
	openvrDriverRootDir=configFile.retrieveString("./openvrDriverRootDir",openvrDriverRootDir);
	openvrDriverRootDir=pathcat(steamRootDir,openvrDriverRootDir);
	
	/* Retrieve the name of the OpenVR device driver dynamic library: */
	std::string openvrDriverDsoName="driver_";
	openvrDriverDsoName.append(openvrDriverName);
	openvrDriverDsoName.append(".so");
	openvrDriverDsoName=configFile.retrieveString("./openvrDriverDsoName",openvrDriverDsoName);
	openvrDriverDsoName=pathcat(openvrDriverRootDir,openvrDriverDsoName);
	
	/* Open the OpenVR device driver dso: */
	log(1,"Loading OpenVR driver module from %s\n",openvrDriverDsoName.c_str());
	openvrDriverDsoHandle=dlopen(openvrDriverDsoName.c_str(),RTLD_NOW);
	if(openvrDriverDsoHandle==0)
		Misc::throwStdErr("OpenVRHost: Unable to load OpenVR driver dynamic shared object %s due to error %s",openvrDriverDsoName.c_str(),dlerror());
	
	/* Retrieve the name of the main driver factory function: */
	std::string openvrFactoryFunctionName="HmdDriverFactory";
	openvrFactoryFunctionName=configFile.retrieveString("./openvrFactoryFunctionName",openvrFactoryFunctionName);
	
	/* Resolve the main factory function (using an evil hack to avoid warnings): */
	typedef void* (*HmdDriverFactoryFunction)(const char* pInterfaceName,int* pReturnCode);
	ptrdiff_t factoryIntermediate=reinterpret_cast<ptrdiff_t>(dlsym(openvrDriverDsoHandle,openvrFactoryFunctionName.c_str()));
	HmdDriverFactoryFunction HmdDriverFactory=reinterpret_cast<HmdDriverFactoryFunction>(factoryIntermediate);
	if(HmdDriverFactory==0)
		{
		dlclose(openvrDriverDsoHandle);
		Misc::throwStdErr("OpenVRHost: Unable to resolve OpenVR driver factory function %s due to error %s",openvrFactoryFunctionName.c_str(),dlerror());
		}
	
	/* Get a pointer to the server-side driver object: */
	int error=0;
	openvrTrackedDeviceProvider=reinterpret_cast<vr::IServerTrackedDeviceProvider*>(HmdDriverFactory(vr::IServerTrackedDeviceProvider_Version,&error));
	if(openvrTrackedDeviceProvider==0)
		{
		dlclose(openvrDriverDsoHandle);
		Misc::throwStdErr("OpenVRHost: Unable to retrieve server-side driver object due to error %d",error);
		}
	
	/*********************************************************************
	Second initialization step: Initialize the VR device driver module.
	*********************************************************************/
	
	/* Retrieve the OpenVR device driver configuration directory: */
	openvrDriverConfigDir="config/";
	openvrDriverConfigDir.append(openvrDriverName);
	openvrDriverConfigDir=configFile.retrieveString("./openvrDriverConfigDir",openvrDriverConfigDir);
	openvrDriverConfigDir=pathcat(steamRootDir,openvrDriverConfigDir);
	log(1,"OpenVR driver module configuration directory is %s\n",openvrDriverConfigDir.c_str());
	
	/* Initialize the driver's property container: */
	properties.set(vr::Prop_UserConfigPath_String,openvrDriverConfigDir);
	properties.set(vr::Prop_InstallPath_String,openvrDriverRootDir);
	
	/* Create descriptors for supported device types: */
	
	/* Head-mounted devices: */
	deviceConfigurations[0].nameTemplate=configFile.retrieveString("./hmdName","HMD");
	deviceConfigurations[0].haveTracker=true;
	deviceConfigurations[0].numButtons=2;
	deviceConfigurations[0].buttonNames.push_back("Button");
	deviceConfigurations[0].buttonNames.push_back("FaceDetector");
	#if 0 // These are on Vive Pro; figure out how to set this dynamically
	deviceConfigurations[0].numButtons=5;
	deviceConfigurations[0].buttonNames.push_back("VolumeUp");
	deviceConfigurations[0].buttonNames.push_back("VolumeDown");
	deviceConfigurations[0].buttonNames.push_back("MuteMic");
	#endif
	deviceConfigurations[0].numValuators=0;
	deviceConfigurations[0].numHapticFeatures=0;
	deviceConfigurations[0].numPowerFeatures=0;
	
	/* Controllers: */
	deviceConfigurations[1].nameTemplate=configFile.retrieveString("./controllerNameTemplate","Controller%u");
	deviceConfigurations[1].haveTracker=true;
	deviceConfigurations[1].numButtons=6;
	deviceConfigurations[1].buttonNames.push_back("System");
	deviceConfigurations[1].buttonNames.push_back("Grip");
	deviceConfigurations[1].buttonNames.push_back("Menu");
	deviceConfigurations[1].buttonNames.push_back("Trigger");
	deviceConfigurations[1].buttonNames.push_back("TouchpadClick");
	deviceConfigurations[1].buttonNames.push_back("TouchpadTouch");
	deviceConfigurations[1].numValuators=3;
	deviceConfigurations[1].valuatorNames.push_back("AnalogTrigger");
	deviceConfigurations[1].valuatorNames.push_back("TouchpadX");
	deviceConfigurations[1].valuatorNames.push_back("TouchpadY");
	deviceConfigurations[1].numHapticFeatures=1;
	deviceConfigurations[1].hapticFeatureNames.push_back("Haptic");
	deviceConfigurations[1].numPowerFeatures=1;
	
	/* Trackers: */
	deviceConfigurations[2].nameTemplate=configFile.retrieveString("./trackerNameTemplate","Tracker%u");
	deviceConfigurations[2].haveTracker=true;
	
	// Experimental!
	deviceConfigurations[2].numButtons=1;
	deviceConfigurations[2].buttonNames.push_back("Power");
	
	deviceConfigurations[2].numValuators=0;
	deviceConfigurations[2].numHapticFeatures=0;
	deviceConfigurations[2].numPowerFeatures=1;
	
	/* Tracking base stations: */
	deviceConfigurations[3].nameTemplate=configFile.retrieveString("./baseStationNameTemplate","BaseStation%u");
	deviceConfigurations[3].haveTracker=false;
	deviceConfigurations[3].numButtons=0;
	deviceConfigurations[3].numValuators=0;
	deviceConfigurations[3].numHapticFeatures=0;
	deviceConfigurations[3].numPowerFeatures=0;
	
	/* Read the maximum number of supported controllers, trackers, and tracking base stations: */
	maxNumDevices[HMD]=1;
	maxNumDevices[Controller]=configFile.retrieveValue<unsigned int>("./maxNumControllers",2);
	maxNumDevices[Tracker]=configFile.retrieveValue<unsigned int>("./maxNumTrackers",0);
	maxNumDevices[BaseStation]=configFile.retrieveValue<unsigned int>("./maxNumBaseStations",2);
	
	/* Calculate total number of device state components: */
	maxNumDevices[NumDeviceTypes]=0;
	unsigned int totalNumTrackers=0;
	unsigned int totalNumButtons=0;
	unsigned int totalNumValuators=0;
	numHapticFeatures=0;
	unsigned int totalNumPowerFeatures=0;
	for(unsigned int deviceType=HMD;deviceType<NumDeviceTypes;++deviceType)
		{
		/* Get the maximum number of devices of this type: */
		unsigned int mnd=maxNumDevices[deviceType];
		maxNumDevices[NumDeviceTypes]+=mnd;
		
		/* Calculate number of device state components for this device type: */
		if(deviceConfigurations[deviceType].haveTracker)
			totalNumTrackers+=mnd;
		totalNumButtons+=mnd*deviceConfigurations[deviceType].numButtons;
		totalNumValuators+=mnd*deviceConfigurations[deviceType].numValuators;
		numHapticFeatures+=mnd*deviceConfigurations[deviceType].numHapticFeatures;
		totalNumPowerFeatures+=mnd*deviceConfigurations[deviceType].numPowerFeatures;
		}
	
	/* Initialize VRDevice's device state variables: */
	setNumTrackers(totalNumTrackers,configFile);
	setNumButtons(totalNumButtons,configFile);
	setNumValuators(totalNumValuators,configFile);
	
	/* Store the originally configured tracker post-transformations to manipulate them later: */
	configuredPostTransformations=new TrackerPostTransformation[totalNumTrackers];
	for(unsigned int i=0;i<totalNumTrackers;++i)
		configuredPostTransformations[i]=trackerPostTransformations[i];
	
	/* Create array of OpenVR device states: */
	deviceStates=new DeviceState[maxNumDevices[NumDeviceTypes]];
	
	/* Create an array of pending haptic events: */
	hapticEvents=new HapticEvent[numHapticFeatures];
	for(unsigned int i=0;i<numHapticFeatures;++i)
		hapticEvents[i].pending=false;
	
	/* Create power features: */
	for(unsigned int i=0;i<totalNumPowerFeatures;++i)
		deviceManager->addPowerFeature(this,i);
	
	/* Create array to map power features to OpenVR devices: */
	powerFeatureDevices=new DeviceState*[totalNumPowerFeatures];
	for(unsigned int i=0;i<totalNumPowerFeatures;++i)
		powerFeatureDevices[i]=0;
	
	/* Create virtual devices for all tracked device types: */
	unsigned int nextTrackerIndex=0;
	unsigned int nextButtonIndex=0;
	unsigned int nextValuatorIndex=0;
	unsigned int nextHapticFeatureIndex=0;
	for(unsigned int deviceType=HMD;deviceType<NumDeviceTypes;++deviceType)
		{
		DeviceConfiguration& dc=deviceConfigurations[deviceType];
		if(dc.haveTracker)
			{
			virtualDeviceIndices[deviceType]=new unsigned int[maxNumDevices[deviceType]];
			for(unsigned int deviceIndex=0;deviceIndex<maxNumDevices[deviceType];++deviceIndex)
				{
				/* Create a virtual device: */
				Vrui::VRDeviceDescriptor* vd=new Vrui::VRDeviceDescriptor(dc.numButtons,dc.numValuators,dc.numHapticFeatures);
				vd->name=Misc::stringPrintf(dc.nameTemplate.c_str(),1U+deviceIndex);
				
				vd->trackType=Vrui::VRDeviceDescriptor::TRACK_POS|Vrui::VRDeviceDescriptor::TRACK_DIR|Vrui::VRDeviceDescriptor::TRACK_ORIENT;
				vd->rayDirection=Vrui::VRDeviceDescriptor::Vector(0,0,-1);
				vd->rayStart=0.0f;
				
				/* Assign a tracker index: */
				vd->trackerIndex=getTrackerIndex(nextTrackerIndex++);
				
				/* Assign buttons names and indices: */
				for(unsigned int i=0;i<dc.numButtons;++i)
					{
					vd->buttonNames[i]=dc.buttonNames[i];
					vd->buttonIndices[i]=getButtonIndex(nextButtonIndex++);
					}
				
				/* Assign valuator names and indices: */
				for(unsigned int i=0;i<dc.numValuators;++i)
					{
					vd->valuatorNames[i]=dc.valuatorNames[i];
					vd->valuatorIndices[i]=getValuatorIndex(nextValuatorIndex++);
					}
				
				/* Assign haptic feature names and indices: */
				for(unsigned int i=0;i<dc.numHapticFeatures;++i)
					{
					vd->hapticFeatureNames[i]=dc.hapticFeatureNames[i];
					vd->hapticFeatureIndices[i]=deviceManager->addHapticFeature(this,nextHapticFeatureIndex++);
					}
				
				/* Override virtual device settings from a configuration file section of the device's name: */
				vd->load(configFile.getSection(vd->name.c_str()));
				
				/* Register the virtual device: */
				virtualDeviceIndices[deviceType][deviceIndex]=addVirtualDevice(vd);
				}
			}
		else
			virtualDeviceIndices[deviceType]=0;
		
		/* Initialize number of connected devices of this type: */
		numConnectedDevices[deviceType]=0;
		}
	
	/* Initialize the total number of connected devices: */
	numConnectedDevices[NumDeviceTypes]=0;
	
	/* Read the number of distortion mesh vertices to calculate: */
	unsigned int distortionMeshSize[2];
	distortionMeshSize[1]=distortionMeshSize[0]=32;
	Misc::CFixedArrayValueCoder<unsigned int,2> distortionMeshSizeVC(distortionMeshSize);
	configFile.retrieveValueWC<unsigned int*>("./distortionMeshSize",distortionMeshSize,distortionMeshSizeVC);
	
	/* Add an HMD configuration for the headset: */
	hmdConfiguration=deviceManager->addHmdConfiguration();
	hmdConfiguration->setTrackerIndex(getTrackerIndex(0));
	hmdConfiguration->setEyePos(Vrui::HMDConfiguration::Point(-0.0635f*0.5f,0,0),Vrui::HMDConfiguration::Point(0.0635f*0.5f,0,0)); // Initialize default eye positions
	hmdConfiguration->setDistortionMeshSize(distortionMeshSize[0],distortionMeshSize[1]);
	
	/* Initialize the component feature index array: */
	componentFeatureIndices=new unsigned int[totalNumButtons+totalNumValuators+numHapticFeatures];
	valuatorStates=new float[totalNumValuators];
	for(unsigned int i=0;i<totalNumValuators;++i)
		valuatorStates[i]=0.0f;
	}

OpenVRHost::~OpenVRHost(void)
	{
	/* Enter stand-by mode: */
	log(1,"Powering down devices\n");
	exiting=true;
	
	/* Put all tracked devices into stand-by mode: */
	for(unsigned int i=0;i<numConnectedDevices[NumDeviceTypes];++i)
		deviceStates[i].driver->EnterStandby();
	
	/* Put the main server into stand-by mode: */
	openvrTrackedDeviceProvider->EnterStandby();
	usleep(100000);
	
	/* Deactivate all devices: */
	for(unsigned int i=0;i<numConnectedDevices[NumDeviceTypes];++i)
		deviceStates[i].driver->Deactivate();
	usleep(500000);
	
	log(1,"Shutting down OpenVR driver module\n");
	openvrTrackedDeviceProvider->Cleanup();
	
	/* Stop the device thread: */
	log(1,"Stopping event processing\n");
	stopDeviceThread();
	
	/* Delete VRDeviceManager association tables: */
	delete[] configuredPostTransformations;
	for(unsigned int deviceType=HMD;deviceType<NumDeviceTypes;++deviceType)
		delete[] virtualDeviceIndices[deviceType];
	delete[] hapticEvents;
	delete[] powerFeatureDevices;
	delete[] componentFeatureIndices;
	delete[] valuatorStates;
	
	/* Delete the OpenVR device state array: */
	delete[] deviceStates;
	
	/* Close the OpenVR device driver dso: */
	dlclose(openvrDriverDsoHandle);
	}

/* Methods inherited from class VRDevice: */

void OpenVRHost::initialize(void)
	{
	/*********************************************************************
	Third initialization step: Initialize the server-side interface of the
	OpenVR driver contained in the shared library.
	*********************************************************************/
	
	/* Start the device thread already to dispatch driver messages during initialization: */
	log(1,"Starting event processing\n");
	startDeviceThread();
	
	/* Initialize the server-side driver object: */
	log(1,"Initializing OpenVR driver module\n");
	vr::EVRInitError initError=openvrTrackedDeviceProvider->Init(static_cast<vr::IVRDriverContext*>(this));
	if(initError!=vr::VRInitError_None)
		Misc::throwStdErr("OpenVRHost: Unable to initialize server-side driver object due to OpenVR error %d",int(initError));
	
	/* Leave stand-by mode: */
	log(1,"Powering up devices\n");
	openvrTrackedDeviceProvider->LeaveStandby();
	}

void OpenVRHost::start(void)
	{
	/* Could un-suspend OpenVR driver at this point... */
	}

void OpenVRHost::stop(void)
	{
	/* Could suspend OpenVR driver at this point... */
	}

void OpenVRHost::powerOff(int devicePowerFeatureIndex)
	{
	/* Power off the device if it is connected and can power off: */
	if(powerFeatureDevices[devicePowerFeatureIndex]!=0&&powerFeatureDevices[devicePowerFeatureIndex]->canPowerOff)
		{
		log(1,"Powering off device with serial number %s\n",powerFeatureDevices[devicePowerFeatureIndex]->serialNumber.c_str());
		
		/* Power off the device: */
		powerFeatureDevices[devicePowerFeatureIndex]->driver->EnterStandby();
		}
	}

void OpenVRHost::hapticTick(int deviceHapticFeatureIndex,unsigned int duration,unsigned int frequency,unsigned int amplitude)
	{
	/* Bail out if there is already a pending event: */
	HapticEvent& he=hapticEvents[deviceHapticFeatureIndex];
	if(!he.pending)
		{
		/* Store the new event: */
		he.pending=true;
		he.duration=float(duration)*0.001f;
		he.frequency=float(frequency);
		he.amplitude=float(amplitude)/255.0f;
		}
	}

/* Methods inherited from class vr::IVRSettings: */

const char* OpenVRHost::GetSettingsErrorNameFromEnum(vr::EVRSettingsError eError)
	{
	switch(eError)
		{
		case vr::VRSettingsError_None:
			return "No error";
		
		case vr::VRSettingsError_IPCFailed:
			return "IPC failed";
		
		case vr::VRSettingsError_WriteFailed:
			return "Write failed";
		
		case vr::VRSettingsError_ReadFailed:
			return "Read failed";
		
		case vr::VRSettingsError_JsonParseFailed:
			return "Parse failed";
		
		case vr::VRSettingsError_UnsetSettingHasNoDefault:
			return "";
		
		default:
			return "Unknown settings error";
		}
	}

void OpenVRHost::SetBool(const char* pchSection,const char* pchSettingsKey,bool bValue,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and store the value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	section.storeValue<bool>(pchSettingsKey,bValue);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void OpenVRHost::SetInt32(const char* pchSection,const char* pchSettingsKey,int32_t nValue,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and store the value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	section.storeValue<int32_t>(pchSettingsKey,nValue);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void OpenVRHost::SetFloat(const char* pchSection,const char* pchSettingsKey,float flValue,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and store the value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	section.storeValue<float>(pchSettingsKey,flValue);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void OpenVRHost::SetString(const char* pchSection,const char* pchSettingsKey,const char* pchValue,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and store the value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	section.storeString(pchSettingsKey,pchValue);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

bool OpenVRHost::GetBool(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and retrieve the requested value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	bool result=section.retrieveValue<bool>(pchSettingsKey,false);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

int32_t OpenVRHost::GetInt32(const char* pchSection,const char*pchSettingsKey,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and retrieve the requested value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	int32_t result=section.retrieveValue<int32_t>(pchSettingsKey,0);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

float OpenVRHost::GetFloat(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and retrieve the requested value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	float result=section.retrieveValue<float>(pchSettingsKey,0.0f);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

void OpenVRHost::GetString(const char* pchSection,const char* pchSettingsKey,char* pchValue,uint32_t unValueLen,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and retrieve the requested value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	std::string result=section.retrieveString(pchSettingsKey,"");
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	
	/* Copy the result string into the provided buffer: */
	if(unValueLen>=result.size()+1)
		memcpy(pchValue,result.c_str(),result.size()+1);
	else
		{
		pchValue[0]='\0';
		if(peError!=0)
			*peError=vr::VRSettingsError_ReadFailed;
		}
	}

void OpenVRHost::RemoveSection(const char* pchSection,vr::EVRSettingsError* peError)
	{
	/* Ignore this request: */
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void OpenVRHost::RemoveKeyInSection(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	/* Ignore this request: */
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

/* Methods inherited from class vr::IVRDriverContext: */

void* OpenVRHost::GetGenericInterface(const char* pchInterfaceVersion,vr::EVRInitError* peError)
	{
	if(peError!=0)
		*peError=vr::VRInitError_None;
	
	/* Cast the driver module object to the requested type and return it: */
	if(strcmp(pchInterfaceVersion,vr::IVRSettings_Version)==0)
		return static_cast<vr::IVRSettings*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRProperties_Version)==0)
		return static_cast<vr::IVRProperties*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRBlockQueue_Version)==0)
		return static_cast<vr::IVRBlockQueue*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRPaths_Version)==0)
		return static_cast<vr::IVRPaths*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRDriverInput_Version)==0)
		return static_cast<vr::IVRDriverInput*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRDriverLog_Version)==0)
		return static_cast<vr::IVRDriverLog*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRServerDriverHost_Version)==0)
		return static_cast<vr::IVRServerDriverHost*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRResources_Version)==0)
		return static_cast<vr::IVRResources*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRIOBuffer_Version)==0)
		return static_cast<vr::IVRIOBuffer*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRDriverManager_Version)==0)
		return static_cast<vr::IVRDriverManager*>(this);
	else
		{
		/* Signal an error: */
		log(2,"Warning: Requested server interface %s not found\n",pchInterfaceVersion);
		if(peError!=0)
			*peError=vr::VRInitError_Init_InterfaceNotFound;
		return 0;
		}
	}

vr::DriverHandle_t OpenVRHost::GetDriverHandle(void)
	{
	/* Driver itself has a fixed handle, based on OpenVR's vrserver: */
	return driverHandle;
	}

/* Methods inherited from class vr::IVRProperties: */

vr::ETrackedPropertyError OpenVRHost::ReadPropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyRead_t* pBatch,uint32_t unBatchEntryCount)
	{
	/* Access the requested property container: */
	class PropertyContainer* propertyContainer;
	const char* containerName;
	if(ulContainerHandle==driverHandle)
		{
		/* Retrieve driver-wide properties: */
		propertyContainer=&properties;
		containerName="driver";
		}
	else if(ulContainerHandle>=deviceHandleBase&&ulContainerHandle<deviceHandleBase+numConnectedDevices[NumDeviceTypes])
		{
		/* Retrieve device properties: */
		propertyContainer=&deviceStates[ulContainerHandle-deviceHandleBase].properties;
		containerName=deviceStates[ulContainerHandle-deviceHandleBase].serialNumber.c_str();
		}
	else
		{
		/* Mark all batch slots as invalid: */
		vr::PropertyRead_t* propPtr=pBatch;
		for(uint32_t entryIndex=0;entryIndex<unBatchEntryCount;++entryIndex,++propPtr)
			{
			log(3,"Read property %u from invalid container %lu\n",propPtr->prop,ulContainerHandle);
			propPtr->unTag=vr::k_unInvalidPropertyTag;
			propPtr->unRequiredBufferSize=0;
			propPtr->eError=vr::TrackedProp_InvalidDevice;
			}
		
		return vr::TrackedProp_InvalidDevice;
		}
	
	/* Process the batch of property read requests: */
	Threads::Mutex::Lock propertiesLock(propertyContainer->getMutex());
	vr::ETrackedPropertyError result=vr::TrackedProp_Success;
	vr::PropertyRead_t* propPtr=pBatch;
	for(uint32_t entryIndex=0;entryIndex<unBatchEntryCount;++entryIndex,++propPtr)
		{
		if(verbosity>=3)
			{
			printf("OpenVRHost: Read property %u from %s: ",propPtr->prop,containerName);
			propertyContainer->print(propPtr->prop);
			printf("\n");
			fflush(stdout);
			}
		
		/* Read the slot: */
		vr::ETrackedPropertyError propResult=propertyContainer->read(propPtr);
		if(result==vr::TrackedProp_Success)
			result=propResult;
		}
	
	/* Always return success; seems to be the driver's approach: */
	return vr::TrackedProp_Success;
	}

vr::ETrackedPropertyError OpenVRHost::WritePropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyWrite_t* pBatch,uint32_t unBatchEntryCount)
	{
	/* Access the requested property container: */
	DeviceState* ds=0;
	class PropertyContainer* propertyContainer;
	const char* containerName;
	if(ulContainerHandle==driverHandle)
		{
		/* Retrieve driver-wide properties: */
		propertyContainer=&properties;
		containerName="driver";
		}
	else if(ulContainerHandle>=deviceHandleBase&&ulContainerHandle<deviceHandleBase+numConnectedDevices[NumDeviceTypes])
		{
		/* Retrieve device properties: */
		ds=&deviceStates[ulContainerHandle-deviceHandleBase];
		propertyContainer=&ds->properties;
		containerName=ds->serialNumber.c_str();
		}
	else
		{
		vr::PropertyWrite_t* propPtr=pBatch;
		for(uint32_t entryIndex=0;entryIndex<unBatchEntryCount;++entryIndex,++propPtr)
			log(3,"Write property %u to invalid container %lu\n",propPtr->prop,ulContainerHandle);
		
		return vr::TrackedProp_InvalidDevice;
		}
	
	/* Process the batch of property write requests: */
	Threads::Mutex::Lock propertiesLock(propertyContainer->getMutex());
	vr::ETrackedPropertyError result=vr::TrackedProp_Success;
	vr::PropertyWrite_t* propPtr=pBatch;
	for(uint32_t entryIndex=0;entryIndex<unBatchEntryCount;++entryIndex,++propPtr)
		{
		/* Write the slot: */
		vr::ETrackedPropertyError propResult=propertyContainer->write(propPtr);
		if(result==vr::TrackedProp_Success)
			result=propResult;
		
		if(verbosity>=3)
			{
			printf("OpenVRHost: Write property %u to %s: ",propPtr->prop,containerName);
			propertyContainer->print(propPtr->prop);
			printf("\n");
			fflush(stdout);
			}
		
		/* Retrieve important properties exposed at the VRDeviceDaemon driver interface: */
		switch(propPtr->prop)
			{
			case vr::Prop_WillDriftInYaw_Bool: // 1004
				if(ds!=0&&propertyContainer->get(propPtr->prop,ds->willDriftInYaw))
					log(1,"Device %s %s drift in yaw\n",ds->serialNumber.c_str(),ds->willDriftInYaw?"will":"will not");
				break;
			
			case vr::Prop_DeviceIsWireless_Bool: // 1010
				if(ds!=0&&propertyContainer->get(propPtr->prop,ds->isWireless))
					{
					log(1,"Device %s is %s\n",ds->serialNumber.c_str(),ds->isWireless?"wireless":"wired");
					
					/* Notify the device manager: */
					deviceManager->updateBatteryState(ds->virtualDeviceIndex,ds->batteryState);
					}
				break;
			
			case vr::Prop_DeviceIsCharging_Bool: // 1011
				{
				bool newBatteryCharging;
				if(ds!=0&&propertyContainer->get(propPtr->prop,newBatteryCharging)&&ds->batteryState.charging!=newBatteryCharging)
					{
					ds->batteryState.charging=newBatteryCharging;
					log(0,"Device %s is now %s\n",ds->serialNumber.c_str(),ds->batteryState.charging?"charging":"discharging");
					
					/* Notify the device manager: */
					deviceManager->updateBatteryState(ds->virtualDeviceIndex,ds->batteryState);
					}
				break;
				}
			
			case vr::Prop_DeviceBatteryPercentage_Float: // 1012
				{
				float newBatteryLevel;
				if(ds!=0&&propertyContainer->get(propPtr->prop,newBatteryLevel))
					{
					unsigned int newBatteryPercent=(unsigned int)(Math::floor(newBatteryLevel*100.0f+0.5f));
					if(ds->batteryState.batteryLevel!=newBatteryPercent)
						{
						ds->batteryState.batteryLevel=newBatteryPercent;
						log(0,"Battery level on device %s is %u%%\n",ds->serialNumber.c_str(),ds->batteryState.batteryLevel);
					
						/* Notify the device manager: */
						deviceManager->updateBatteryState(ds->virtualDeviceIndex,ds->batteryState);
						}
					}
				break;
				}
			
			case vr::Prop_ContainsProximitySensor_Bool: // 1025
				{
				bool newHasProximitySensor;
				if(ds!=0&&propertyContainer->get(propPtr->prop,newHasProximitySensor)&&ds->hasProximitySensor!=newHasProximitySensor)
					{
					ds->hasProximitySensor=newHasProximitySensor;
					log(1,"Device %s %s proximity sensor\n",ds->serialNumber.c_str(),ds->hasProximitySensor?"has":"does not have");
					}
				break;
				}
			
			case vr::Prop_DeviceProvidesBatteryStatus_Bool: // 1026
				{
				bool newProvidesBatteryStatus;
				if(ds!=0&&propertyContainer->get(propPtr->prop,newProvidesBatteryStatus)&&ds->providesBatteryStatus!=newProvidesBatteryStatus)
					{
					ds->providesBatteryStatus=newProvidesBatteryStatus;
					log(1,"Device %s %s battery status\n",ds->serialNumber.c_str(),ds->providesBatteryStatus?"provides":"does not provide");
					}
				break;
				}
			
			case vr::Prop_DeviceCanPowerOff_Bool: // 1027
				{
				bool newCanPowerOff;
				if(ds!=0&&propertyContainer->get(propPtr->prop,newCanPowerOff)&&ds->canPowerOff!=newCanPowerOff)
					{
					ds->canPowerOff=newCanPowerOff;
					log(1,"Device %s %s power off\n",ds->serialNumber.c_str(),ds->canPowerOff?"can":"can not");
					}
				break;
				}
			
			case vr::Prop_UserIpdMeters_Float: // 2003
				{
				float ipd;
				if(ds!=0&&propertyContainer->get(propPtr->prop,ipd))
					{
					log(0,"User IPD = %fmm\n",ipd*1000.0f);
					
					{
					Threads::Mutex::Lock hmdConfigurationLock(deviceManager->getHmdConfigurationMutex());
					
					/* Update the HMD's IPD: */
					ds->hmdConfiguration->setIpd(ipd);
					
					/* Update the HMD configuration in the device manager: */
					deviceManager->updateHmdConfiguration(ds->hmdConfiguration);
					}
					}
				break;
				}
			
			case vr::Prop_DisplayMCImageLeft_String:
			case vr::Prop_DisplayMCImageRight_String:
				/* Remove mura correction images from the property container because we can't create the MC interface: */
				propertyContainer->remove(propPtr->prop);
				break;
			
			default:
				; // Do nothing
			}
		}
	
	return result;
	}

const char* OpenVRHost::GetPropErrorNameFromEnum(vr::ETrackedPropertyError error)
	{
	return propErrorName(error);
	}

vr::PropertyContainerHandle_t OpenVRHost::TrackedDeviceToPropertyContainer(vr::TrackedDeviceIndex_t nDevice)
	{
	return deviceHandleBase+nDevice;
	}

/* Methods inherited from class vr::IVRBlockQueue: */
vr::EBlockQueueError OpenVRHost::Create(vr::PropertyContainerHandle_t* pulQueueHandle,char* pchPath,uint32_t unBlockDataSize,uint32_t unBlockHeaderSize,uint32_t unBlockCount)
	{
	/* Find the block queue in the handle map: */
	std::string path(pchPath);
	BlockQueueHandleMap::Iterator bqhIt=blockQueueHandles.findEntry(path);
	if(bqhIt.isFinished())
		{
		/* Create a new block queue: */
		blockQueueHandles.setEntry(BlockQueueHandleMap::Entry(path,nextBlockQueueHandle));
		*pulQueueHandle=nextBlockQueueHandle;
		++nextBlockQueueHandle;
		return vr::EBlockQueueError_BlockQueueError_None;
		}
	else
		{
		/* Block queue already exists: */
		*pulQueueHandle=0;
		return vr::EBlockQueueError_BlockQueueError_QueueAlreadyExists;
		}
	}

vr::EBlockQueueError OpenVRHost::Connect(vr::PropertyContainerHandle_t* pulQueueHandle,char* pchPath)
	{
	/* Find the block queue in the handle map: */
	std::string path(pchPath);
	BlockQueueHandleMap::Iterator bqhIt=blockQueueHandles.findEntry(path);
	if(!bqhIt.isFinished())
		{
		/* Return the existing block queue: */
		*pulQueueHandle=bqhIt->getDest();
		return vr::EBlockQueueError_BlockQueueError_None;
		}
	else
		{
		/* Block queue doesn't exist: */
		*pulQueueHandle=0;
		return vr::EBlockQueueError_BlockQueueError_QueueNotFound;
		}
	}

vr::EBlockQueueError OpenVRHost::Destroy(vr::PropertyContainerHandle_t ulQueueHandle)
	{
	/* Just ignore this for now: */
	return vr::EBlockQueueError_BlockQueueError_None;
	}

vr::EBlockQueueError OpenVRHost::AcquireWriteOnlyBlock(vr::PropertyContainerHandle_t ulQueueHandle,vr::PropertyContainerHandle_t* pulBlockHandle,void** ppvBuffer)
	{
	/* Just ignore this for now: */
	*pulBlockHandle=0;
	ppvBuffer=0;
	return vr::EBlockQueueError_BlockQueueError_None;
	}

vr::EBlockQueueError OpenVRHost::ReleaseWriteOnlyBlock(vr::PropertyContainerHandle_t ulQueueHandle,vr::PropertyContainerHandle_t ulBlockHandle)
	{
	/* Just ignore this for now: */
	return vr::EBlockQueueError_BlockQueueError_None;
	}

vr::EBlockQueueError OpenVRHost::WaitAndAcquireReadOnlyBlock(vr::PropertyContainerHandle_t ulQueueHandle,vr::PropertyContainerHandle_t* pulBlockHandle,void** ppvBuffer,vr::EBlockQueueReadType eReadType,uint32_t unTimeoutMs)
	{
	/* Just ignore this for now: */
	*pulBlockHandle=0;
	ppvBuffer=0;
	return vr::EBlockQueueError_BlockQueueError_None;
	}

vr::EBlockQueueError OpenVRHost::AcquireReadOnlyBlock(vr::PropertyContainerHandle_t ulQueueHandle,vr::PropertyContainerHandle_t* pulBlockHandle,void** ppvBuffer,vr::EBlockQueueReadType eReadType)
	{
	/* Just ignore this for now: */
	*pulBlockHandle=0;
	ppvBuffer=0;
	return vr::EBlockQueueError_BlockQueueError_None;
	}

vr::EBlockQueueError OpenVRHost::ReleaseReadOnlyBlock(vr::PropertyContainerHandle_t ulQueueHandle,vr::PropertyContainerHandle_t ulBlockHandle)
	{
	/* Just ignore this for now: */
	return vr::EBlockQueueError_BlockQueueError_None;
	}

vr::EBlockQueueError OpenVRHost::QueueHasReader(vr::PropertyContainerHandle_t ulQueueHandle,bool* pbHasReaders)
	{
	/* Pretend there are no readers: */
	*pbHasReaders=false;
	return vr::EBlockQueueError_BlockQueueError_None;
	}

/* Methods inherited from class vr::IVRPaths: */

vr::ETrackedPropertyError OpenVRHost::ReadPathBatch(vr::PropertyContainerHandle_t ulRootHandle,struct vr::PathRead_t* pBatch,uint32_t unBatchEntryCount)
	{
	vr::PathRead_t* pPtr=pBatch;
	for(uint32_t i=0;i<unBatchEntryCount;++i,++pPtr)
		{
		log(3,"ReadPathBatch for %lu to %lu, \n",ulRootHandle,pPtr->ulPath);
		pPtr->unTag=vr::k_unInvalidPropertyTag;
		pPtr->unRequiredBufferSize=0;
		pPtr->eError=vr::TrackedProp_UnknownProperty;
		}
	
	return vr::TrackedProp_Success;
	}

vr::ETrackedPropertyError OpenVRHost::WritePathBatch(vr::PropertyContainerHandle_t ulRootHandle,struct vr::PathWrite_t* pBatch,uint32_t unBatchEntryCount)
	{
	vr::PathWrite_t* pPtr=pBatch;
	for(uint32_t i=0;i<unBatchEntryCount;++i,++pPtr)
		{
		if(verbosity>=3)
			{
			printf("WritePathBatch for %lu to %lu, \n",ulRootHandle,pPtr->ulPath);
			#if 0
			vr::PropertyWrite_t pw;
			pw.prop=vr::ETrackedDeviceProperty(0);
			pw.writeType=pPtr->writeType;
			pw.eSetError=pPtr->eSetError;
			pw.pvBuffer=pPtr->pvBuffer;
			pw.unBufferSize=pPtr->unBufferSize;
			pw.unTag=pPtr->unTag;
			pw.eError=pPtr->eError;
			printProperty(pw);
			#endif
			}
		
		pPtr->eError=vr::TrackedProp_Success;
		}
	
	return vr::TrackedProp_Success;
	}

vr::ETrackedPropertyError OpenVRHost::StringToHandle(vr::PathHandle_t* pHandle,char* pchPath)
	{
	/* Find the given string in the path handle map: */
	std::string path(pchPath);
	PathHandleMap::Iterator phIt=pathHandles.findEntry(path);
	if(!phIt.isFinished())
		{
		/* Return the existing handle: */
		*pHandle=phIt->getDest();
		}
	else
		{
		/* Put the new path into the table: */
		pathHandles.setEntry(PathHandleMap::Entry(path,nextPathHandle));
		*pHandle=nextPathHandle;
		++nextPathHandle;
		}
	log(3,"StringToHandle: returning handle %lu for path %s\n",*pHandle,path.c_str());
	
	return vr::TrackedProp_Success;
	}

vr::ETrackedPropertyError OpenVRHost::HandleToString(vr::PathHandle_t pHandle,char* pchBuffer,uint32_t unBufferSize,uint32_t* punBufferSizeUsed)
	{
	/* Find the handle in the path handle map the hard way: */
	PathHandleMap::Iterator phIt;
	for(phIt=pathHandles.begin();!phIt.isFinished()&&phIt->getDest()!=pHandle;++phIt)
		;
	if(!phIt.isFinished())
		{
		return vr::TrackedProp_Success;
		}
	else
		{
		log(1,"HandleToString called with unknown handle %lu\n",pHandle);
		
		/* Signal an error: */
		*punBufferSizeUsed=0;
		return vr::TrackedProp_UnknownProperty; // No idea if this is the correct error code
		}
	}

/* Methods inherited from class vr::IVRDriverInput: */

vr::EVRInputError OpenVRHost::CreateBooleanComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle)
	{
	/* Get a reference to the container device's device state: */
	if(ulContainer>=deviceHandleBase&&ulContainer<deviceHandleBase+numConnectedDevices[NumDeviceTypes])
		{
		unsigned int deviceIndex=ulContainer-deviceHandleBase;
		DeviceState& ds=deviceStates[deviceIndex];
		
		/* Check if there is room in the device's button array: */
		if(ds.numButtons<deviceConfigurations[ds.deviceType].numButtons)
			{
			log(2,"Creating button %s on device %s with index %d\n",pchName,ds.serialNumber.c_str(),ds.nextButtonIndex);
			
			/* Assign the next device button index to the next component handle: */
			*pHandle=nextComponentHandle;
			componentFeatureIndices[nextComponentHandle-1]=ds.nextButtonIndex;
			++nextComponentHandle;
			++ds.nextButtonIndex;
			++ds.numButtons;
			
			return vr::VRInputError_None;
			}
		else
			{
			log(1,"Ignoring extra boolean input %s on device %s\n",pchName,ds.serialNumber.c_str());
			return vr::VRInputError_MaxCapacityReached;
			}
		}
	else
		{
		log(1,"Ignoring boolean input %s due to invalid container handle %u\n",pchName,(unsigned int)ulContainer);
		return vr::VRInputError_InvalidHandle;
		}
	}

vr::EVRInputError OpenVRHost::UpdateBooleanComponent(vr::VRInputComponentHandle_t ulComponent,bool bNewValue, double fTimeOffset)
	{
	/* Check if the component is valid: */
	if(ulComponent<nextComponentHandle)
		{
		unsigned int buttonIndex=componentFeatureIndices[ulComponent-1];
		log(4,"Setting button %u to %s\n",buttonIndex,bNewValue?"pressed":"released");
		
		/* Set the button's state: */
		setButtonState(buttonIndex,bNewValue);
		return vr::VRInputError_None;
		}
	else
		{
		log(4,"Ignoring invalid boolean input %lu\n",ulComponent);
		return vr::VRInputError_InvalidHandle;
		}
	}

vr::EVRInputError OpenVRHost::CreateScalarComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle,vr::EVRScalarType eType,vr::EVRScalarUnits eUnits)
	{
	/* Get a reference to the container device's device state: */
	if(ulContainer>=deviceHandleBase&&ulContainer<deviceHandleBase+numConnectedDevices[NumDeviceTypes])
		{
		unsigned int deviceIndex=ulContainer-deviceHandleBase;
		DeviceState& ds=deviceStates[deviceIndex];
		
		/* Check if there is room in the device's valuator array: */
		if(ds.numValuators<deviceConfigurations[ds.deviceType].numValuators)
			{
			log(2,"Creating valuator %s on device %s with index %d\n",pchName,ds.serialNumber.c_str(),ds.nextValuatorIndex);
			
			/* Assign the next device valuator index to the next component handle: */
			*pHandle=nextComponentHandle;
			componentFeatureIndices[nextComponentHandle-1]=ds.nextValuatorIndex;
			++nextComponentHandle;
			++ds.nextValuatorIndex;
			++ds.numValuators;
			
			return vr::VRInputError_None;
			}
		else
			{
			log(1,"Ignoring extra scalar input %s on device %s\n",pchName,ds.serialNumber.c_str());
			return vr::VRInputError_MaxCapacityReached;
			}
		}
	else
		{
		log(1,"Ignoring scalar input %s due to invalid container handle %u\n",pchName,(unsigned int)ulContainer);
		return vr::VRInputError_InvalidHandle;
		}
	}

vr::EVRInputError OpenVRHost::UpdateScalarComponent(vr::VRInputComponentHandle_t ulComponent,float fNewValue, double fTimeOffset)
	{
	/* Check if the component is valid: */
	if(ulComponent<nextComponentHandle)
		{
		/* Check if the valuator actually changed: */
		unsigned int valuatorIndex=componentFeatureIndices[ulComponent-1];
		if(valuatorStates[valuatorIndex]!=fNewValue)
			{
			log(4,"Setting valuator %u to %f\n",valuatorIndex,fNewValue);
			
			/* Set the valuator's state: */
			valuatorStates[valuatorIndex]=fNewValue;
			setValuatorState(valuatorIndex,fNewValue);
			}
		
		return vr::VRInputError_None;
		}
	else
		{
		log(4,"Ignoring invalid scalar input %lu\n",ulComponent);
		return vr::VRInputError_InvalidHandle;
		}
	}

vr::EVRInputError OpenVRHost::CreateHapticComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle)
	{
	/* Get a reference to the container device's device state: */
	if(ulContainer>=deviceHandleBase&&ulContainer<deviceHandleBase+numConnectedDevices[NumDeviceTypes])
		{
		unsigned int deviceIndex=ulContainer-deviceHandleBase;
		DeviceState& ds=deviceStates[deviceIndex];
		
		/* Check if there is room in the device's haptic feature array: */
		if(ds.numHapticFeatures<deviceConfigurations[ds.deviceType].numHapticFeatures)
			{
			log(2,"Creating haptic feature %s on device %s with index %d\n",pchName,ds.serialNumber.c_str(),ds.nextHapticFeatureIndex);
			
			/* Assign the next device haptic feature index to the next component handle: */
			*pHandle=nextComponentHandle;
			HapticEvent& he=hapticEvents[ds.nextHapticFeatureIndex];
			he.containerHandle=ulContainer;
			he.componentHandle=nextComponentHandle;
			he.pending=false;
			he.duration=0;
			he.frequency=0;
			he.amplitude=0;
			++nextComponentHandle;
			++ds.nextHapticFeatureIndex;
			++ds.numHapticFeatures;
			
			return vr::VRInputError_None;
			}
		else
			{
			log(1,"Ignoring extra haptic component %s on device %s\n",pchName,ds.serialNumber.c_str());
			return vr::VRInputError_MaxCapacityReached;
			}
		}
	else
		{
		log(1,"Ignoring haptic component %s due to invalid container handle %u\n",pchName,(unsigned int)ulContainer);
		return vr::VRInputError_InvalidHandle;
		}
	}

vr::EVRInputError OpenVRHost::CreateSkeletonComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,const char* pchSkeletonPath,const char* pchBasePosePath,vr::EVRSkeletalTrackingLevel eSkeletalTrackingLevel,const vr::VRBoneTransform_t* pGripLimitTransforms,uint32_t unGripLimitTransformCount,vr::VRInputComponentHandle_t* pHandle)
	{
	log(1,"Ignoring call to CreateSkeletonComponent\n");
	
	return vr::VRInputError_None;
	}

vr::EVRInputError OpenVRHost::UpdateSkeletonComponent(vr::VRInputComponentHandle_t ulComponent,vr::EVRSkeletalMotionRange eMotionRange,const vr::VRBoneTransform_t* pTransforms,uint32_t unTransformCount)
	{
	log(1,"Ignoring call to UpdateSkeletonComponent\n");
	
	return vr::VRInputError_None;
	}

/* Methods from vr::IVRDriverLog: */

void OpenVRHost::Log(const char* pchLogMessage)
	{
	if(printLogMessages)
		{
		printf("OpenVRHost: Driver log: %s",pchLogMessage);
		fflush(stdout);
		}
	}

/* Methods inherited from class vr::IVRServerDriverHost: */

bool OpenVRHost::TrackedDeviceAdded(const char* pchDeviceSerialNumber,vr::ETrackedDeviceClass eDeviceClass,vr::ITrackedDeviceServerDriver* pDriver)
	{
	/* Determine the new device's class: */
	DeviceTypes deviceType=NumDeviceTypes;
	const char* newDeviceClass=0;
	switch(eDeviceClass)
		{
		case vr::TrackedDeviceClass_Invalid:
			newDeviceClass="invalid tracked device";
			break;
		
		case vr::TrackedDeviceClass_HMD:
			deviceType=HMD;
			newDeviceClass="head-mounted display";
			break;
		
		case vr::TrackedDeviceClass_Controller:
			deviceType=Controller;
			newDeviceClass="controller";
			break;
		
		case vr::TrackedDeviceClass_GenericTracker:
			deviceType=Tracker;
			newDeviceClass="generic tracker";
			break;
		
		case vr::TrackedDeviceClass_TrackingReference:
			deviceType=BaseStation;
			newDeviceClass="tracking base station";
			break;
		
		default:
			newDeviceClass="unknown device";
		}
	
	/* Bail out if the device has unknown type or the state array is full: */
	if(deviceType==NumDeviceTypes||numConnectedDevices[deviceType]>=maxNumDevices[deviceType])
		{
		log(1,"Ignoring %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
		return false;
		}
	
	/* Grab the next free device state structure: */
	const DeviceConfiguration& dc=deviceConfigurations[deviceType];
	DeviceState& ds=deviceStates[numConnectedDevices[NumDeviceTypes]];
	ds.deviceType=deviceType;
	ds.serialNumber=pchDeviceSerialNumber;
	ds.driver=pDriver;
	
	/* Initialize the device's property container: */
	ds.properties.set(vr::Prop_SerialNumber_String,ds.serialNumber);
	ds.properties.set(vr::Prop_DeviceClass_Int32,int32_t(eDeviceClass));
	
	/* Check whether the device is tracked: */
	if(dc.haveTracker)
		{
		/* Assign the device state's tracker index: */
		ds.trackerIndex=0;
		for(unsigned int dt=HMD;dt<deviceType;++dt)
			ds.trackerIndex+=maxNumDevices[dt];
		ds.trackerIndex+=numConnectedDevices[deviceType];
		
		/* Assign the device state's virtual device index: */
		ds.virtualDeviceIndex=virtualDeviceIndices[deviceType][numConnectedDevices[deviceType]];
		}
	
	if(deviceType==HMD)
		{
		/* Assign the device state's HMD configuration: */
		ds.hmdConfiguration=hmdConfiguration;
		hmdConfiguration=0;
		
		/* Get the device's display component: */
		ds.display=static_cast<vr::IVRDisplayComponent*>(ds.driver->GetComponent(vr::IVRDisplayComponent_Version));
		if(ds.display!=0)
			{
			/* Initialize the device state's HMD configuration: */
			updateHMDConfiguration(ds);
			}
		else
			log(1,"Head-mounted display with serial number %s does not advertise a display\n",pchDeviceSerialNumber);
		}
	
	/* Assign the device state's first button index: */
	ds.nextButtonIndex=0;
	for(unsigned int dt=HMD;dt<deviceType;++dt)
		ds.nextButtonIndex+=maxNumDevices[dt]*deviceConfigurations[dt].numButtons;
	ds.nextButtonIndex+=numConnectedDevices[deviceType]*dc.numButtons;
	
	/* Assign the device state's first valuator index: */
	ds.nextValuatorIndex=0;
	for(unsigned int dt=HMD;dt<deviceType;++dt)
		ds.nextValuatorIndex+=maxNumDevices[dt]*deviceConfigurations[dt].numValuators;
	ds.nextValuatorIndex+=numConnectedDevices[deviceType]*dc.numValuators;
	
	/* Assign the device state's first haptic feature index: */
	ds.nextHapticFeatureIndex=0;
	for(unsigned int dt=HMD;dt<deviceType;++dt)
		ds.nextHapticFeatureIndex+=maxNumDevices[dt]*deviceConfigurations[dt].numHapticFeatures;
	ds.nextHapticFeatureIndex+=numConnectedDevices[deviceType]*dc.numHapticFeatures;
	
	/* Associate the device state with its power features: */
	unsigned int powerFeatureIndexBase=0;
	for(unsigned int dt=HMD;dt<deviceType;++dt)
		powerFeatureIndexBase+=maxNumDevices[dt]*deviceConfigurations[dt].numPowerFeatures;
	powerFeatureIndexBase+=numConnectedDevices[deviceType]*dc.numPowerFeatures;
	for(unsigned int i=0;i<dc.numPowerFeatures;++i)
		powerFeatureDevices[powerFeatureIndexBase+i]=&ds;
	
	/* Increase the number of connected devices: */
	++numConnectedDevices[deviceType];
	++numConnectedDevices[NumDeviceTypes];
	
	/* Activate the device: */
	log(1,"Activating newly-added %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
	ds.driver->Activate(numConnectedDevices[NumDeviceTypes]-1);
	log(1,"Done activating newly-added %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
	
	return true;
	}

void OpenVRHost::TrackedDevicePoseUpdated(uint32_t unWhichDevice,const vr::DriverPose_t& newPose,uint32_t unPoseStructSize)
	{
	/* Get a time stamp for the new device pose: */
	Vrui::VRDeviceState::TimeStamp poseTimeStamp=deviceManager->getTimeStamp(newPose.poseTimeOffset);
	
	/* Access the state of the affected device: */
	DeviceState& ds=deviceStates[unWhichDevice];
	
	/* Check if the device changed connection state: */
	if(ds.connected!=newPose.deviceIsConnected)
		{
		/* Change the device's connected state: */
		ds.connected=newPose.deviceIsConnected;
		
		log(1,"Tracked device with serial number %s is now %s\n",ds.serialNumber.c_str(),ds.connected?"connected":"disconnected");
		}
	
	/* Check if the device changed tracking state: */
	if(ds.tracked!=newPose.poseIsValid)
		{
		ds.tracked=newPose.poseIsValid;
		
		/* Disable the device if it is no longer tracked: */
		if(ds.trackerIndex>=0&&!newPose.poseIsValid)
			disableTracker(ds.trackerIndex);
		
		log(1,"Tracked device with serial number %s %s tracking\n",ds.serialNumber.c_str(),ds.tracked?"regained":"lost");
		}
	
	/* Update the device's transformation if it is being tracked: */
	if(ds.trackerIndex>=0&&ds.tracked)
		{
		Vrui::VRDeviceState::TrackerState ts;
		typedef PositionOrientation::Vector Vector;
		typedef Vector::Scalar Scalar;
		typedef PositionOrientation::Rotation Rotation;
		typedef Rotation::Scalar RScalar;
		
		/* Get the device's world transformation: */
		Rotation worldRot(RScalar(newPose.qWorldFromDriverRotation.x),RScalar(newPose.qWorldFromDriverRotation.y),RScalar(newPose.qWorldFromDriverRotation.z),RScalar(newPose.qWorldFromDriverRotation.w));
		Vector worldTrans(Scalar(newPose.vecWorldFromDriverTranslation[0]),Scalar(newPose.vecWorldFromDriverTranslation[1]),Scalar(newPose.vecWorldFromDriverTranslation[2]));
		PositionOrientation world(worldTrans,worldRot);
		
		/* Get the device's local transformation: */
		Rotation localRot(RScalar(newPose.qDriverFromHeadRotation.x),RScalar(newPose.qDriverFromHeadRotation.y),RScalar(newPose.qDriverFromHeadRotation.z),RScalar(newPose.qDriverFromHeadRotation.w));
		Vector localTrans(Scalar(newPose.vecDriverFromHeadTranslation[0]),Scalar(newPose.vecDriverFromHeadTranslation[1]),Scalar(newPose.vecDriverFromHeadTranslation[2]));
		PositionOrientation local(localTrans,localRot);
		
		/* Check for changes: */
		if(ds.worldTransform!=world)
			ds.worldTransform=world;
		if(ds.localTransform!=local)
			{
			ds.localTransform=local;
			
			/* Combine the driver's reported local transformation and the configured tracker post-transformation: */
			trackerPostTransformations[ds.trackerIndex]=local*configuredPostTransformations[ds.trackerIndex];
			}
		
		/* Get the device's driver transformation: */
		Rotation driverRot(RScalar(newPose.qRotation.x),RScalar(newPose.qRotation.y),RScalar(newPose.qRotation.z),RScalar(newPose.qRotation.w));
		Vector driverTrans(Scalar(newPose.vecPosition[0]),Scalar(newPose.vecPosition[1]),Scalar(newPose.vecPosition[2]));
		PositionOrientation driver(driverTrans,driverRot);
		
		/* Assemble the device's world-space tracking state: */
		ts.positionOrientation=world*driver;
		
		/* Reported linear velocity is in base station space, as optimal for the sensor fusion algorithm: */
		ts.linearVelocity=ds.worldTransform.transform(Vrui::VRDeviceState::TrackerState::LinearVelocity(newPose.vecVelocity));
		
		/* Reported angular velocity is in IMU space, as optimal for the sensor fusion algorithm: */
		ts.angularVelocity=ts.positionOrientation.transform(Vrui::VRDeviceState::TrackerState::AngularVelocity(newPose.vecAngularVelocity));
		
		/* Set the tracker state in the device manager, which will apply the device's local transformation and configured post-transformation: */
		setTrackerState(ds.trackerIndex,ts,poseTimeStamp);
		}
	
	/* Force a device state update if the HMD reported in: */
	if(ds.trackerIndex==0)
		updateState();
	}

void OpenVRHost::VsyncEvent(double vsyncTimeOffsetSeconds)
	{
	log(1,"Ignoring vsync event with time offset %f\n",vsyncTimeOffsetSeconds);
	}

void OpenVRHost::VendorSpecificEvent(uint32_t unWhichDevice,vr::EVREventType eventType,const vr::VREvent_Data_t& eventData,double eventTimeOffset)
	{
	log(1,"Ignoring vendor-specific event of type %d for device %u\n",int(eventType),unWhichDevice);
	}

bool OpenVRHost::IsExiting(void)
	{
	/* Return true if the driver module is shutting down: */
	return exiting;
	}

bool OpenVRHost::PollNextEvent(vr::VREvent_t* pEvent,uint32_t uncbVREvent)
	{
	/* Check if there is a pending haptic event on any haptic component: */
	for(unsigned int hapticFeatureIndex=0;hapticFeatureIndex<numHapticFeatures;++hapticFeatureIndex)
		{
		HapticEvent& he=hapticEvents[hapticFeatureIndex];
		if(he.pending)
			{
			/* Fill in the event structure: */
			pEvent->eventType=vr::VREvent_Input_HapticVibration;
			pEvent->trackedDeviceIndex=he.containerHandle-deviceHandleBase;
			pEvent->eventAgeSeconds=0.0f;
			
			vr::VREvent_HapticVibration_t& hv=pEvent->data.hapticVibration;
			hv.containerHandle=he.containerHandle;
			hv.componentHandle=he.componentHandle;
			hv.fDurationSeconds=he.duration;
			hv.fFrequency=he.frequency;
			hv.fAmplitude=he.amplitude;
			
			/* Mark the event as processed: */
			he.pending=false;
			
			return true;
			}
		}
	
	return false;
	}

void OpenVRHost::GetRawTrackedDevicePoses(float fPredictedSecondsFromNow,vr::TrackedDevicePose_t* pTrackedDevicePoseArray,uint32_t unTrackedDevicePoseArrayCount)
	{
	log(1,"Ignoring GetRawTrackedDevicePoses request\n");
	}

void OpenVRHost::RequestRestart(const char* pchLocalizedReason,const char* pchExecutableToStart,const char* pchArguments,const char* pchWorkingDirectory)
	{
	log(1,"Ignoring RequestRestart request with reason %s, executable %s, arguments %s and working directory %s\n",pchLocalizedReason,pchExecutableToStart,pchArguments,pchWorkingDirectory);
	}

uint32_t OpenVRHost::GetFrameTimings(vr::Compositor_FrameTiming* pTiming,uint32_t nFrames)
	{
	log(1,"Ignoring GetFrameTimings request with result array %p of size %u\n",static_cast<void*>(pTiming),nFrames);
	
	return 0;
	}

void OpenVRHost::SetDisplayEyeToHead(uint32_t unWhichDevice,const vr::HmdMatrix34_t& eyeToHeadLeft,const vr::HmdMatrix34_t& eyeToHeadRight)
	{
	log(1,"Ignoring SetDisplayEyeToHead request\n");
	}

void OpenVRHost::SetDisplayProjectionRaw(uint32_t unWhichDevice,const vr::HmdRect2_t& eyeLeft,const vr::HmdRect2_t& eyeRight)
	{
	if(verbosity>=1)
		{
		printf("OpenVRHost: Setting HMD's raw projection parameters\n");
		printf("\tLeft eye : left %f, right %f, top %f, bottom %f\n",eyeLeft.vTopLeft.v[0],eyeLeft.vBottomRight.v[0],eyeLeft.vTopLeft.v[1],eyeLeft.vBottomRight.v[1]);
		printf("\tRight eye: left %f, right %f, top %f, bottom %f\n",eyeRight.vTopLeft.v[0],eyeRight.vBottomRight.v[0],eyeRight.vTopLeft.v[1],eyeRight.vBottomRight.v[1]);
		fflush(stdout);
		}
	}

void OpenVRHost::SetRecommendedRenderTargetSize(uint32_t unWhichDevice,uint32_t nWidth,uint32_t nHeight)
	{
	log(1,"Setting HMD's recommended render target size to %u x %u\n",nWidth,nHeight);
	
	/* Update the state of the affected device: */
	DeviceState& ds=deviceStates[unWhichDevice];
	
	Threads::Mutex::Lock hmdConfigurationLock(deviceManager->getHmdConfigurationMutex());
	
	/* Update recommended pre-distortion render target size: */
	ds.hmdConfiguration->setRenderTargetSize(nWidth,nHeight);
	
	/* Tell the device manager that the HMD configuration was updated: */
	deviceManager->updateHmdConfiguration(ds.hmdConfiguration);
	}

/* Methods inherited from class vr::IVRResources: */

uint32_t OpenVRHost::LoadSharedResource(const char* pchResourceName,char* pchBuffer,uint32_t unBufferLen)
	{
	log(2,"LoadSharedResource called with resource name %s and buffer size %u\n",pchResourceName,unBufferLen);
	
	/* Extract the driver name template from the given resource name: */
	const char* driverStart=0;
	const char* driverEnd=0;
	for(const char* rnPtr=pchResourceName;*rnPtr!='\0';++rnPtr)
		{
		if(*rnPtr=='{')
			driverStart=rnPtr;
		else if(*rnPtr=='}')
			driverEnd=rnPtr+1;
		}
	
	/* Assemble the resource path based on the OpenVR root directory and the driver name: */
	std::string resourcePath=openvrRootDir;
	resourcePath.append("/drivers/");
	resourcePath.append(std::string(driverStart+1,driverEnd-1));
	resourcePath.append("/resources");
	resourcePath.append(driverEnd);
	
	/* Open the resource file: */
	try
		{
		IO::SeekableFilePtr resourceFile=IO::openSeekableFile(resourcePath.c_str());
		
		/* Check if the resource fits into the given buffer: */
		size_t resourceSize=resourceFile->getSize();
		if(resourceSize<=unBufferLen)
			{
			/* Load the resource into the buffer: */
			resourceFile->readRaw(pchBuffer,resourceSize);
			}
		
		return uint32_t(resourceSize);
		}
	catch(const std::runtime_error& err)
		{
		log(0,"Resource %s could not be loaded due to exception %s\n",resourcePath.c_str(),err.what());
		return 0;
		}
	}

uint32_t OpenVRHost::GetResourceFullPath(const char* pchResourceName,const char* pchResourceTypeDirectory,char* pchPathBuffer,uint32_t unBufferLen)
	{
	log(2,"GetResourceFullPath called with resource name %s and resource type directory %s\n",pchResourceName,pchResourceTypeDirectory);
	
	/* Extract the driver name template from the given resource name: */
	const char* driverStart=0;
	const char* driverEnd=0;
	for(const char* rnPtr=pchResourceName;*rnPtr!='\0';++rnPtr)
		{
		if(*rnPtr=='{')
			driverStart=rnPtr;
		else if(*rnPtr=='}')
			driverEnd=rnPtr+1;
		}
	
	/* Assemble the resource path based on the OpenVR root directory and the driver name: */
	std::string resourcePath=openvrRootDir;
	if(driverStart!=0&&driverEnd!=0)
		{
		resourcePath.append("/drivers/");
		resourcePath.append(driverStart+1,driverEnd-1);
		}
	resourcePath.append("/resources/");
	if(pchResourceTypeDirectory!=0)
		{
		resourcePath.append(pchResourceTypeDirectory);
		resourcePath.push_back('/');
		}
	if(driverEnd!=0)
		resourcePath.append(driverEnd);
	else
		resourcePath.append(pchResourceName);
	
	log(2,"Full resource path is %s\n",resourcePath.c_str());
	
	/* Copy the resource path into the buffer if there is enough space: */
	if(unBufferLen>=resourcePath.size()+1)
		memcpy(pchPathBuffer,resourcePath.c_str(),resourcePath.size()+1);
	else
		pchPathBuffer[0]='\0';
	return resourcePath.size()+1;
	}

vr::EIOBufferError OpenVRHost::Open(const char* pchPath,vr::EIOBufferMode mode,uint32_t unElementSize,uint32_t unElements,vr::IOBufferHandle_t* pulBuffer)
	{
	log(2,"Open called with path %s, buffer mode %u, element size %u and number of elements %u\n",pchPath,uint32_t(mode),unElementSize,unElements);
	
	vr::EIOBufferError result=vr::IOBuffer_Success;
	
	/* Find an I/O buffer of the given path: */
	IOBufferMap::Iterator iobIt;
	for(iobIt=ioBufferMap.begin();!iobIt.isFinished()&&iobIt->getDest().path!=pchPath;++iobIt)
		;
	if(mode&vr::IOBufferMode_Create)
		{
		if(iobIt.isFinished())
			{
			/* Create a new I/O buffer: */
			++lastIOBufferHandle;
			ioBufferMap.setEntry(IOBufferMap::Entry(lastIOBufferHandle,IOBuffer(lastIOBufferHandle)));
			IOBuffer& buf=ioBufferMap.getEntry(lastIOBufferHandle).getDest();
			buf.path=pchPath;
			buf.size=size_t(unElements)*size_t(unElementSize);
			buf.buffer=malloc(buf.size);
			
			/* Return the new buffer's handle: */
			*pulBuffer=lastIOBufferHandle;
			}
		else
			{
			log(0,"Open: Path %s already exists\n",pchPath);
			result=vr::IOBuffer_PathExists;
			}
		}
	else
		{
		if(!iobIt.isFinished())
			{
			/* Return the existing buffer's handle: */
			*pulBuffer=iobIt->getDest().handle;
			}
		else
			{
			log(0,"Open: Path %s does not exist\n",pchPath);
			result=vr::IOBuffer_PathDoesNotExist;
			}
		}
	
	return result;
	}

vr::EIOBufferError OpenVRHost::Close(vr::IOBufferHandle_t ulBuffer)
	{
	log(2,"Close called with buffer handle %lu\n",ulBuffer);
	
	vr::EIOBufferError result=vr::IOBuffer_Success;
	
	/* Find the I/O buffer: */
	IOBufferMap::Iterator iobIt=ioBufferMap.findEntry(ulBuffer);
	if(!iobIt.isFinished())
		{
		/* Delete the buffer: */
		ioBufferMap.removeEntry(iobIt);
		}
	else
		{
		log(0,"Close: Invalid buffer handle %lu\n",ulBuffer);
		result=vr::IOBuffer_InvalidHandle;
		}
	
	return result;
	}

vr::EIOBufferError OpenVRHost::Read(vr::IOBufferHandle_t ulBuffer,void* pDst,uint32_t unBytes,uint32_t* punRead)
	{
	vr::EIOBufferError result=vr::IOBuffer_Success;
	
	/* Find the I/O buffer: */
	IOBufferMap::Iterator iobIt=ioBufferMap.findEntry(ulBuffer);
	if(!iobIt.isFinished())
		{
		/* Read as much data as requested and available: */
		IOBuffer& buf=iobIt->getDest();
		uint32_t canRead=unBytes;
		if(canRead>buf.dataSize)
			canRead=buf.dataSize;
		memcpy(pDst,buf.buffer,canRead);
		*punRead=canRead;
		}
	else
		{
		log(0,"Read: Invalid buffer handle %lu\n",ulBuffer);
		result=vr::IOBuffer_InvalidHandle;
		}
	
	return result;
	}

vr::EIOBufferError OpenVRHost::Write(vr::IOBufferHandle_t ulBuffer,void* pSrc,uint32_t unBytes)
	{
	vr::EIOBufferError result=vr::IOBuffer_Success;
	
	/* Find the I/O buffer: */
	IOBufferMap::Iterator iobIt=ioBufferMap.findEntry(ulBuffer);
	if(!iobIt.isFinished())
		{
		/* Write if the data fits into the buffer: */
		IOBuffer& buf=iobIt->getDest();
		if(unBytes<=buf.size)
			{
			memcpy(buf.buffer,pSrc,unBytes);
			buf.dataSize=unBytes;
			}
		else
			{
			log(0,"Write: Overflow on buffer handle %lu\n",ulBuffer);
			result=vr::IOBuffer_InvalidArgument;
			}
		}
	else
		{
		log(0,"Write: Invalid buffer handle %lu\n",ulBuffer);
		fflush(stdout);
		result=vr::IOBuffer_InvalidHandle;
		}
	
	return result;
	}

vr::PropertyContainerHandle_t OpenVRHost::PropertyContainer(vr::IOBufferHandle_t ulBuffer)
	{
	log(2,"PropertyContainer called with buffer handle %lu\n",ulBuffer);
	
	return vr::k_ulInvalidPropertyContainer;
	}

bool OpenVRHost::HasReaders(vr::IOBufferHandle_t ulBuffer)
	{
	log(4,"HasReaders called with buffer handle %lu\n",ulBuffer);
	
	return false;
	}

/* Methods inherited from class vr::IVRDriverManager: */

uint32_t OpenVRHost::GetDriverCount(void) const
	{
	/* There appear to be two drivers: htc and lighthouse: */
	return 2;
	}

uint32_t OpenVRHost::GetDriverName(vr::DriverId_t nDriver,char* pchValue,uint32_t unBufferSize)
	{
	/* Return one of the two hard-coded driver names: */
	static const char* driverNames[2]={"lighthouse","htc"};
	if(nDriver<2)
		{
		size_t dnLen=strlen(driverNames[nDriver])+1;
		if(dnLen<=unBufferSize)
			memcpy(pchValue,driverNames[nDriver],dnLen);
		return dnLen;
		}
	else
		return 0;
	}

vr::DriverHandle_t OpenVRHost::GetDriverHandle(const char *pchDriverName)
	{
	log(2,"GetDriverHandle called with driver name %s\n",pchDriverName);
	
	/* Driver itself has a fixed handle, based on OpenVR's vrserver: */
	return driverHandle;
	}

bool OpenVRHost::IsEnabled(vr::DriverId_t nDriver) const
	{
	log(2,"IsEnabled called for driver %u\n",nDriver);
	
	return true;
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectOpenVRHost(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new OpenVRHost(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectOpenVRHost(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
