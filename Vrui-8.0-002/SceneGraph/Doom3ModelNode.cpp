/***********************************************************************
Doom3ModelNode - Class for nodes to render static models using Doom3's
lighting model.
Copyright (c) 2010-2021 Oliver Kreylos

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

#include <SceneGraph/Doom3ModelNode.h>

#include <string.h>
#include <Misc/FileNameExtensions.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>
#include <SceneGraph/Internal/Doom3MaterialManager.h>
#include <SceneGraph/Internal/Doom3Model.h>
#include <SceneGraph/Internal/LoadModelFromASEFile.h>
#include <SceneGraph/Internal/LoadModelFromLWOFile.h>

namespace SceneGraph {

/***************************************
Static elements of class Doom3ModelNode:
***************************************/

const char* Doom3ModelNode::className="Doom3Model";

/*******************************
Methods of class Doom3ModelNode:
*******************************/

Doom3ModelNode::Doom3ModelNode(void)
	:mesh(0)
	{
	}

Doom3ModelNode::~Doom3ModelNode(void)
	{
	delete mesh;
	}

const char* Doom3ModelNode::getClassName(void) const
	{
	return className;
	}

void Doom3ModelNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"dataContext")==0)
		{
		vrmlFile.parseSFNode(dataContext);
		}
	else if(strcmp(fieldName,"model")==0)
		{
		vrmlFile.parseField(model);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

unsigned int Doom3ModelNode::update(void)
	{
	/* Delete the current model: */
	delete mesh;
	mesh=0;
	
	/* Determine the model's format: */
	const char* fileExt=Misc::getExtension(model.getValue().c_str());
	
	try
		{
		/* Load the model using the appropriate format parser: */
		if(strcasecmp(fileExt,".lwo")==0)
			mesh=loadModelFromLWOFile(*dataContext.getValue()->getFileManager(),*dataContext.getValue()->getMaterialManager(),model.getValue().c_str());
		else if(strcasecmp(fileExt,".ase")==0)
			mesh=loadModelFromASEFile(*dataContext.getValue()->getFileManager(),*dataContext.getValue()->getMaterialManager(),model.getValue().c_str());
		
		/* Tell the material manager to load all requested materials: */
		dataContext.getValue()->getMaterialManager()->loadMaterials(*dataContext.getValue()->getFileManager());
		}
	catch(const std::runtime_error&)
		{
		/* Just delete the model again: */
		delete mesh;
		mesh=0;
		}
	
	/* If there is a mesh, set the pass mask to opaque OpenGL rendering, otherwise make it empty: */
	return setPassMask(mesh!=0?GLRenderPass:0x0U);
	}

Box Doom3ModelNode::calcBoundingBox(void) const
	{
	/* Return the model's bounding box: */
	if(mesh!=0)
		return mesh->getBoundingBox();
	else
		return Box::empty;
	}

void Doom3ModelNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	renderState.setFrontFace(GL_CW);
	
	/* Initialize the material manager: */
	Doom3MaterialManager::RenderContext mmRc=dataContext.getValue()->getMaterialManager()->start(renderState.contextData,false);
	
	/* Draw the model: */
	mesh->glRenderAction(renderState.contextData,mmRc);
	
	/* Shut down the material manager: */
	dataContext.getValue()->getMaterialManager()->finish(mmRc);
	}

}
