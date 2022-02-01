/***********************************************************************
MessageLogger - Class derived from Misc::MessageLogger to log and
present messages inside a Vrui application.
Copyright (c) 2015-2020 Oliver Kreylos

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

#include <Vrui/Internal/MessageLogger.h>

#include <ctype.h>
#include <unistd.h>
#include <utility>
#include <string>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <Vrui/Vrui.h>

namespace Vrui {

/******************************
Methods of class MessageLogger:
******************************/

void MessageLogger::logMessageInternal(Target target,int messageLevel,const char* message)
	{
	/* Handle messages based on target: */
	if(target==Log)
		{
		/* For now, write the message directly to stdout if this is the master node: */
		if(Vrui::isMaster())
			{
			/* Append a newline to the message: */
			std::string paddedMessage=message;
			paddedMessage.append("\n");
			
			/* Write message directly to stdout, bypassing any buffers: */
			if(write(STDOUT_FILENO,paddedMessage.data(),paddedMessage.size())!=ssize_t(paddedMessage.size()))
				{
				/* Whatcha gonna do? */
				}
			}
		}
	else if(target==Console||userToConsole)
		{
		/* Write the message directly to stderr if this is the master node: */
		if(Vrui::isMaster())
			{
			/* Append a newline to the message: */
			std::string paddedMessage=message;
			paddedMessage.append("\n");
			
			/* Write message directly to stderr, bypassing any buffers: */
			if(write(STDERR_FILENO,paddedMessage.data(),paddedMessage.size())!=ssize_t(paddedMessage.size()))
				{
				/* Whatcha gonna do? */
				}
			}
		}
	else
		{
		/* Store the message for presentation as a dialog window during the next frame: */
		Threads::Mutex::Lock pendingMessagesLock(pendingMessagesMutex);
		pendingMessages.push_back(PendingMessage(messageLevel,message));
		
		if(!frameCallbackRegistered)
			{
			/* Register a callback with Vrui to be called during the next frame: */
			Vrui::addFrameCallback(frameCallback,this);
			
			/* Wake up the main thread: */
			Vrui::requestUpdate();
			
			frameCallbackRegistered=true;
			}
		}
	}

void MessageLogger::showMessageDialog(int messageLevel,const char* messageString)
	{
	/* Assemble the dialog title: */
	std::string title=messageLevel<Warning?"Note":messageLevel<Error?"Warning":"Error";
	
	/* Check if the messageString starts with a source identifier: */
	const char* colonPtr=0;
	for(const char* mPtr=messageString;*mPtr!='\0'&&!isspace(*mPtr);++mPtr)
		if(*mPtr==':')
			colonPtr=mPtr;
	
	if(colonPtr!=0&&colonPtr[1]!='\0'&&isspace(colonPtr[1]))
		{
		/* Append the messageString source to the title: */
		title.append(" from ");
		title.append(messageString,colonPtr);
		
		/* Cut the source from the messageString: */
		messageString=colonPtr+1;
		}
	
	/* Create an appropriate acknowledgment button label: */
	const char* buttonLabel=messageLevel<Warning?"Gee, thanks":messageLevel<Error?"Alright then":"Darn it!";
	
	/* Show a message dialog: */
	showErrorMessage(title.c_str(),messageString,buttonLabel);
	}

bool MessageLogger::frameCallback(void* userData)
	{
	MessageLogger* thisPtr=static_cast<MessageLogger*>(userData);
	
	/* Grab the list of pending messages: */
	std::vector<PendingMessage> pendingMessages;
	{
	Threads::Mutex::Lock pendingMessagesLock(thisPtr->pendingMessagesMutex);
	std::swap(thisPtr->pendingMessages,pendingMessages);
	thisPtr->frameCallbackRegistered=false;
	}
	
	/* Process all grabbed messages: */
	for(std::vector<PendingMessage>::iterator pmIt=pendingMessages.begin();pmIt!=pendingMessages.end();++pmIt)
		thisPtr->showMessageDialog(pmIt->messageLevel,pmIt->message.c_str());
	
	/* Remove the callback again: */
	return true;
	}

MessageLogger::MessageLogger(void)
	:userToConsole(true),
	 frameCallbackRegistered(false)
	{
	}

MessageLogger::~MessageLogger(void)
	{
	}

void MessageLogger::setUserToConsole(bool newUserToConsole)
	{
	userToConsole=newUserToConsole;
	}

}
