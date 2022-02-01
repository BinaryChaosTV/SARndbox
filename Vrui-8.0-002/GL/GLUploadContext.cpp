/***********************************************************************
GLUploadContext - Class to represent an OpenGL context associated with
an unmapped X window to enable asynchronous bulk data upload into
another OpenGL context.
Copyright (c) 2020 Oliver Kreylos

This file is part of the OpenGL/GLX Support Library (GLXSupport).

The OpenGL/GLX Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL/GLX Support Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL/GLX Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GL/GLUploadContext.h>

#include <stdexcept>
#include <GL/GLContext.h>
#include <GL/GLExtensionManager.h>

/********************************
Methods of class GLUploadContext:
********************************/

GLUploadContext::GLUploadContext(GLContext& destContext)
	:display(destContext.getDisplay()),context(0),colorMap(None),window(None),
	 extensionManager(0)
	{
	/* Work on the display's default screen: */
	int screen=DefaultScreen(display);
	Window root=RootWindow(display,screen);
	
	/* Find a minimalistic GL-compatible visual: */
	int glxVisualAttributes[]={GLX_RGBA,None};
	XVisualInfo* visInfo=glXChooseVisual(display,screen,glxVisualAttributes);
	if(visInfo==0)
		throw std::runtime_error("GLUploadContext::GLUploadContext: No suitable visual found");
	
	/* Create a GL context: */
	context=glXCreateContext(display,visInfo,destContext.getContext(),GL_TRUE);
	if(context==0)
		{
		XFree(visInfo);
		throw std::runtime_error("GLUploadContext::GLUploadContext: Unable to create context");
		}
	
	#if 0 // Testing performance of attaching to an already-used window
	
	/* Create an X colormap (visual might not be default): */
	colorMap=XCreateColormap(display,root,visInfo->visual,AllocNone);
	
	/* Create an X window with the selected visual: */
	XSetWindowAttributes swa;
	swa.border_pixel=0;
	swa.colormap=colorMap;
	swa.override_redirect=False;
	int width=32;
	int height=32;
	swa.event_mask=0;
	unsigned long attributeMask=CWBorderPixel|CWColormap|CWOverrideRedirect|CWEventMask;
	window=XCreateWindow(display,root,0,0,width,height,0,visInfo->depth,InputOutput,visInfo->visual,attributeMask,&swa);
	if(window==None)
		{
		XFree(visInfo);
		XFreeColormap(display,colorMap);
		glXDestroyContext(display,context);
		throw std::runtime_error("GLUploadContext::GLUploadContext: Unable to create window");
		}
	
	#endif
	
	/* Clean up: */
	XFree(visInfo);
	}

GLUploadContext::~GLUploadContext(void)
	{
	/* Clean up: */
	delete extensionManager;
	// XDestroyWindow(display,window); // Testing
	XFreeColormap(display,colorMap);
	glXDestroyContext(display,context);
	}

void GLUploadContext::makeCurrent(void)
	{
	/* Install the OpenGL context: */
	glXMakeCurrent(display,window,context);
	
	/* Create the extension manager if it doesn't already exist: */
	if(extensionManager==0)
		extensionManager=new GLExtensionManager;
	
	/* Install this context's GL extension manager: */
	GLExtensionManager::makeCurrent(extensionManager);
	}

void GLUploadContext::release(void)
	{
	/* Uninstall this context's extension manager: */
	GLExtensionManager::makeCurrent(0);
	
	/* Uninstall the OpenGL context: */
	glXMakeCurrent(display,None,0);
	}
