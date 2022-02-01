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

#ifndef GLUPLOADCONTEXT_INCLUDED
#define GLUPLOADCONTEXT_INCLUDED

#include <X11/X.h>
#include <GL/glx.h>

/* Forward declarations: */
class GLContext;
class GLExtensionManager;

class GLUploadContext
	{
	/* Elements: */
	private:
	Display* display; // X display connection shared with the destination context
	GLXContext context; // GLX context handle
	Colormap colorMap; // Colormap used in window
	Window window; // X window handle
	GLExtensionManager* extensionManager; // Pointer to an extension manager for this GLX context
	
	/* Constructors and destructors: */
	public:
	GLUploadContext(GLContext& destContext); // Creates an upload context for the given OpenGL context
	~GLUploadContext(void);
	
	/* Methods: */
	void makeCurrent(void); // Makes the GL context current in the current thread
	void release(void); // Releases the GL context from the current thread
	
	/* Testing methods: */
	void setWindow(Window newWindow) // Sets the window to which the context will be attached for testing purposes
		{
		window=newWindow;
		}
	};

#endif
