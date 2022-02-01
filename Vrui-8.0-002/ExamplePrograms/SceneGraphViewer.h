/***********************************************************************
SceneGraphViewer - Viewer for one or more scene graphs loaded from VRML
2.0 files.
Copyright (c) 2010-2021 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPHVIEWER_INCLUDED
#define SCENEGRAPHVIEWER_INCLUDED

#include <string>
#include <GLMotif/ToggleButton.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/GroupNode.h>
#include <Vrui/Application.h>
#include <Vrui/ToolManager.h>
#include <Vrui/SurfaceNavigationTool.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace GLMotif {
class PopupMenu;
}

class SceneGraphViewer:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef SceneGraph::Scalar Scalar;
	typedef SceneGraph::Point Point;
	typedef SceneGraph::Vector Vector;
	
	struct SGItem // Structure for scene graph list items
		{
		/* Elements: */
		public:
		std::string fileName; // Name of file from which the scene graph was loaded
		SceneGraph::GroupNodePointer root; // Pointer to the scene graph's root node
		bool navigational; // Flag whether the scene graph is in navigational coordinates
		bool enabled; // Flag whether the scene graph is currently enabled
		};
	
	struct AlignmentState:public Vrui::SurfaceNavigationTool::AlignmentState // Structure to keep track of continuing surface-aligned navigation sequences
		{
		public:
		Scalar height; // User height in navigational space from previous frame
		Scalar floorLift; // Amount by which the floor was artificially lifted on the previous frame to correct lack of headroom
		};
	
	class WalkNavigationToolFactory;
	class WalkNavigationTool;
	
	/* Elements: */
	private:
	std::vector<SGItem> sceneGraphs; // List of loaded scene graphs
	GLMotif::PopupMenu* mainMenu;
	
	/* Private methods: */
	void goToPhysicalSpaceCallback(Misc::CallbackData* cbData);
	void sceneGraphToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& index);
	void reloadAllSceneGraphsCallback(Misc::CallbackData* cbData);
	void alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData);
	
	/* Constructors and destructors: */
	public:
	SceneGraphViewer(int& argc,char**& argv);
	~SceneGraphViewer(void);
	
	/* Methods: */
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

#endif
