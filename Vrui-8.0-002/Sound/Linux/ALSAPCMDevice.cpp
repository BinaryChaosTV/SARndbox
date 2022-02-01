/***********************************************************************
ALSAPCMDevice - Simple wrapper class around PCM devices as represented
by the Advanced Linux Sound Architecture (ALSA) library.
Copyright (c) 2009-2020 Oliver Kreylos

This file is part of the Basic Sound Library (Sound).

The Basic Sound Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Sound Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Sound Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Sound/Linux/ALSAPCMDevice.h>

#include <poll.h>
#include <Misc/ThrowStdErr.h>
#include <Sound/SoundDataFormat.h>

namespace Sound {

/******************************
Methods of class ALSAPCMDevice:
******************************/

void ALSAPCMDevice::throwException(const char* methodName,int error)
	{
	if(error==-EPIPE)
		{
		char buffer[512];
		if(recording)
			throw OverrunError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"ALSAPCMDevice::%s: Overrun detected",methodName));
		else
			throw UnderrunError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"ALSAPCMDevice::%s: Underrun detected",methodName));
		}
	else
		Misc::throwStdErr("ALSAPCMDevice::%s: ALSA error %d (%s)",methodName,-error,snd_strerror(error));
	}

bool ALSAPCMDevice::pcmEventForwarder(Threads::EventDispatcher::ListenerKey eventKey,int eventTypeMask,void* userData)
	{
	ALSAPCMDevice* thisPtr=static_cast<ALSAPCMDevice*>(userData);
	
	/* Find the poll structure on whose file descriptor this event occurred: */
	int index;
	for(index=0;index<thisPtr->numPCMEventFds&&thisPtr->pcmEventListenerKeys[index]!=eventKey;++index)
		;
	struct pollfd& pfd=thisPtr->pcmEventPolls[index];
		
	/* Update the poll structure's event mask: */
	pfd.revents=0x0;
	if(eventTypeMask&Threads::EventDispatcher::Read)
		pfd.revents|=POLLIN;
	if(eventTypeMask&Threads::EventDispatcher::Write)
		pfd.revents|=POLLOUT;
	
	/* Parse the event: */
	unsigned short event;
	if(snd_pcm_poll_descriptors_revents(thisPtr->pcmDevice,thisPtr->pcmEventPolls,thisPtr->numPCMEventFds,&event)==0&&(event&(POLLIN|POLLOUT)))
		{
		/* Call the event callback: */
		thisPtr->pcmEventCallback(*thisPtr,thisPtr->pcmEventCallbackUserData);
		}
	
	/* Keep listening for events: */
	return false;
	}

ALSAPCMDevice::ALSAPCMDevice(const char* pcmDeviceName,bool sRecording,bool nonBlocking)
	:pcmDevice(0),
	 recording(sRecording),
	 pcmSampleFormat(SND_PCM_FORMAT_UNKNOWN),pcmChannels(1),pcmRate(8000),pcmBufferFrames(0),pcmPeriodFrames(0),
	 pcmConfigPending(true),
	 pcmEventCallback(0),pcmEventCallbackUserData(0),
	 numPCMEventFds(0),pcmEventPolls(0),pcmEventListenerKeys(0)
	{
	/* Open the PCM device: */
	int error=snd_pcm_open(&pcmDevice,pcmDeviceName,recording?SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK,nonBlocking?SND_PCM_NONBLOCK:0);
	if(error<0)
		{
		pcmDevice=0;
		Misc::throwStdErr("ALSAPCMDevice::ALSAPCMDevice: Error %s while opening PCM device %s for %s",snd_strerror(error),pcmDeviceName,recording?"recording":"playback");
		}
	
	/* Create a PCM hardware parameter context with initial values: */
	snd_pcm_hw_params_t* pcmHwParams;
	snd_pcm_hw_params_alloca(&pcmHwParams);
	snd_pcm_hw_params_any(pcmDevice,pcmHwParams);
	}

ALSAPCMDevice::~ALSAPCMDevice(void)
	{
	if(pcmDevice!=0)
		snd_pcm_close(pcmDevice);
	
	/* Delete the poll structure and listener key array just in case: */
	delete[] pcmEventPolls;
	delete[] pcmEventListenerKeys;
	}

snd_async_handler_t* ALSAPCMDevice::registerAsyncHandler(snd_async_callback_t callback,void* privateData)
	{
	snd_async_handler_t* result;
	int error;
	if((error=snd_async_add_pcm_handler(&result,pcmDevice,callback,privateData))<0)
		Misc::throwStdErr("ALSAPCMDevice::registerAsyncHandler: Error %s while registering asynchronous event handler",snd_strerror(error));
	
	return result;
	}

void ALSAPCMDevice::setSoundDataFormat(const SoundDataFormat& newFormat)
	{
	/* Retrieve ALSA sample format from sound data format structure: */
	pcmSampleFormat=newFormat.getPCMFormat();
	
	/* Retrieve number of channels and sample rate: */
	pcmChannels=(unsigned int)(newFormat.samplesPerFrame);
	pcmRate=(unsigned int)(newFormat.framesPerSecond);
	
	pcmConfigPending=true;
	}

void ALSAPCMDevice::setBufferSize(size_t numBufferFrames,size_t numPeriodFrames)
	{
	/* Retrieve buffer and period sizes: */
	pcmBufferFrames=snd_pcm_uframes_t(numBufferFrames);
	pcmPeriodFrames=snd_pcm_uframes_t(numPeriodFrames);
	
	pcmConfigPending=true;
	}

size_t ALSAPCMDevice::getBufferSize(void) const
	{
	/* Return the most recent buffer size actually configured: */
	return size_t(pcmBufferFrames);
	}

size_t ALSAPCMDevice::getPeriodSize(void) const
	{
	/* Return the most recent period size actually configured: */
	return size_t(pcmPeriodFrames);
	}

void ALSAPCMDevice::setStartThreshold(size_t numStartFrames)
	{
	int error;
	
	/* Allocate a software parameter context: */
	snd_pcm_sw_params_t* pcmSwParams;
	snd_pcm_sw_params_alloca(&pcmSwParams);
	
	/* Get the PCM device's current software parameter context: */
	if((error=snd_pcm_sw_params_current(pcmDevice,pcmSwParams))<0)
		Misc::throwStdErr("ALSAPCMDevice::setStartThreshold: Error %s while getting device's software parameter context",snd_strerror(error));
	
	/* Set the start threshold: */
	if((error=snd_pcm_sw_params_set_start_threshold(pcmDevice,pcmSwParams,snd_pcm_uframes_t(numStartFrames)))<0)
		{
		snd_pcm_sw_params_free(pcmSwParams);
		Misc::throwStdErr("ALSAPCMDevice::setStartThreshold: Error %s while setting start threshold",snd_strerror(error));
		}
	
	/* Write the changed software parameter set to the PCM device: */
	if((error=snd_pcm_sw_params(pcmDevice,pcmSwParams))<0)
		{
		snd_pcm_sw_params_free(pcmSwParams);
		Misc::throwStdErr("ALSAPCMDevice::setStartThreshold: Error %s while writing software parameters to device",snd_strerror(error));
		}
	}

void ALSAPCMDevice::prepare(void)
	{
	int error;
	
	if(pcmConfigPending)
		{
		/* Create a PCM hardware parameter context with initial values: */
		snd_pcm_hw_params_t* pcmHwParams;
		snd_pcm_hw_params_alloca(&pcmHwParams);
		if((error=snd_pcm_hw_params_any(pcmDevice,pcmHwParams))<0)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while creating device's hardware parameter context",snd_strerror(error));
		
		/* Enable the PCM device's hardware resampling for non-native sample rates: */
		if((error=snd_pcm_hw_params_set_rate_resample(pcmDevice,pcmHwParams,1))<0)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while enabling device's hardware resampler",snd_strerror(error));
		
		/* Set the PCM device's access method: */
		if((error=snd_pcm_hw_params_set_access(pcmDevice,pcmHwParams,SND_PCM_ACCESS_RW_INTERLEAVED))<0)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while setting device's access method",snd_strerror(error));
		
		/* Set the PCM device's sample format: */
		if((error=snd_pcm_hw_params_set_format(pcmDevice,pcmHwParams,pcmSampleFormat))<0)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while setting device's sample format",snd_strerror(error));
		
		/* Set the PCM device's number of channels: */
		if((error=snd_pcm_hw_params_set_channels(pcmDevice,pcmHwParams,pcmChannels))<0)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while setting device's number of channels",snd_strerror(error));
		
		/* Set the PCM device's sample rate: */
		unsigned int requestedPcmRate=pcmRate;
		if((error=snd_pcm_hw_params_set_rate(pcmDevice,pcmHwParams,pcmRate,0))<0)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while setting device's sample rate",snd_strerror(error));
		
		/* Check if the requested sample rate was correctly set: */
		if(pcmRate!=requestedPcmRate)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Requested sample rate %u, got %u instead",requestedPcmRate,pcmRate);
		
		/* Set PCM device's buffer size: */
		if(pcmBufferFrames!=0)
			{
			unsigned int bufferTime=(unsigned int)(size_t(pcmBufferFrames)*size_t(1000000)/size_t(pcmRate));
			int pcmBufferDir=0;
			if((error=snd_pcm_hw_params_set_buffer_time_near(pcmDevice,pcmHwParams,&bufferTime,&pcmBufferDir))<0)
				Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while setting device's buffer size",snd_strerror(error));
			}
		
		/* Read the configured buffer size: */
		snd_pcm_hw_params_get_buffer_size(pcmHwParams,&pcmBufferFrames);
		
		/* Set PCM device's period size: */
		int pcmPeriodDir=0;
		if(pcmPeriodFrames!=0)
			{
			unsigned int periodTime=(unsigned int)(size_t(pcmPeriodFrames)*size_t(1000000)/size_t(pcmRate));
			if((error=snd_pcm_hw_params_set_period_time_near(pcmDevice,pcmHwParams,&periodTime,&pcmPeriodDir))<0)
				Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while setting device's period size",snd_strerror(error));
			}
		
		/* Read the configured period size: */
		snd_pcm_hw_params_get_period_size(pcmHwParams,&pcmPeriodFrames,&pcmPeriodDir);
		
		/* Write the changed hardware parameter set to the PCM device: */
		if((error=snd_pcm_hw_params(pcmDevice,pcmHwParams))<0)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while writing hardware parameters to device",snd_strerror(error));
		
		pcmConfigPending=false;
		}
	else
		{
		/* Prepare the PCM device: */
		if((error=snd_pcm_prepare(pcmDevice))<0)
			Misc::throwStdErr("ALSAPCMDevice::prepare: Error %s while preparing device",snd_strerror(error));
		}
	}

void ALSAPCMDevice::link(ALSAPCMDevice& other)
	{
	int result=snd_pcm_link(pcmDevice,other.pcmDevice);
	if(result<0)
		throwException("link",result);
	}

void ALSAPCMDevice::unlink(void)
	{
	int result=snd_pcm_unlink(pcmDevice);
	if(result<0)
		throwException("unlink",result);
	}

void ALSAPCMDevice::addPCMEventListener(Threads::EventDispatcher& dispatcher,ALSAPCMDevice::PCMEventCallback eventCallback,void* eventCallbackUserData)
	{
	/* Check if there is already a PCM event callback: */
	if(pcmEventCallback!=0)
		throw std::runtime_error("ALSAPCMDevice::addPCMEventListener: PCM event listener already registered");
	
	/* Store the callback: */
	pcmEventCallback=eventCallback;
	pcmEventCallbackUserData=eventCallbackUserData;
	
	/* Retrieve the set of file descriptors that need to be watched: */
	numPCMEventFds=snd_pcm_poll_descriptors_count(pcmDevice);
	pcmEventPolls=new struct pollfd[numPCMEventFds];
	numPCMEventFds=snd_pcm_poll_descriptors(pcmDevice,pcmEventPolls,numPCMEventFds);
	
	/* Create IO event listeners for all PCM file descriptors: */
	pcmEventListenerKeys=new Threads::EventDispatcher::ListenerKey[numPCMEventFds];
	for(int i=0;i<numPCMEventFds;++i)
		{
		/* Assemble a proper event mask: */
		int eventMask=0x0;
		if(pcmEventPolls[i].events&POLLIN)
			eventMask|=Threads::EventDispatcher::Read;
		if(pcmEventPolls[i].events&POLLOUT)
			eventMask|=Threads::EventDispatcher::Write;
		pcmEventListenerKeys[i]=dispatcher.addIOEventListener(pcmEventPolls[i].fd,eventMask,pcmEventForwarder,this);
		}
	}

void ALSAPCMDevice::removePCMEventListener(Threads::EventDispatcher& dispatcher)
	{
	/* Bail out if there is no PCM event callback: */
	if(pcmEventCallback==0)
		return;
	
	/* Remove the callback: */
	pcmEventCallback=0;
	pcmEventCallbackUserData=0;
	
	/* Remove all previously created IO event listeners: */
	for(int i=0;i<numPCMEventFds;++i)
		dispatcher.removeIOEventListener(pcmEventListenerKeys[i]);
	numPCMEventFds=0;
	delete[] pcmEventPolls;
	pcmEventPolls=0;
	delete[] pcmEventListenerKeys;
	pcmEventListenerKeys=0;
	}

void ALSAPCMDevice::start(void)
	{
	int result=snd_pcm_start(pcmDevice);
	if(result<0)
		throwException("start",result);
	}

void ALSAPCMDevice::drop(void)
	{
	int result=snd_pcm_drop(pcmDevice);
	if(result<0)
		throwException("drop",result);
	}

void ALSAPCMDevice::drain(void)
	{
	int result=snd_pcm_drain(pcmDevice);
	if(result<0)
		throwException("drop",result);
	}

}
