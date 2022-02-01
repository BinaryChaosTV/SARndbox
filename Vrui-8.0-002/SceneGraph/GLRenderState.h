/***********************************************************************
GLRenderState - Class encapsulating the traversal state of a scene graph
during OpenGL rendering.
Copyright (c) 2009-2021 Oliver Kreylos

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

#ifndef SCENEGRAPH_GLRENDERSTATE_INCLUDED
#define SCENEGRAPH_GLRENDERSTATE_INCLUDED

#include <Misc/SizedTypes.h>
#include <Geometry/AffineTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLFrustum.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <SceneGraph/TraversalState.h>

/* Forward declarations: */
class GLContextData;

namespace SceneGraph {

class GLRenderState:public TraversalState
	{
	/* Embedded classes: */
	public:
	typedef GLColor<GLfloat,4> Color; // Type for RGBA colors
	typedef GLFrustum<Scalar> Frustum; // Class describing the rendering context's view frustum
	typedef Geometry::AffineTransformation<Scalar,3> TextureTransform; // Affine texture transformation
	
	private:
	struct GLState // Structure to track current OpenGL state to minimize changes
		{
		/* Elements: */
		public:
		GLenum frontFace;
		bool cullingEnabled;
		GLenum culledFace;
		bool lightingEnabled;
		bool normalizeEnabled;
		GLenum lightModelTwoSide;
		Color emissiveColor;
		bool colorMaterialEnabled;
		int highestTexturePriority; // Priority level of highest enabled texture unit (None=-1, 1D=0, 2D, 3D, cube map)
		GLuint boundTextures[4]; // Texture object IDs of currently bound 1D, 2D, 3D, and cube map textures
		GLenum lightModelColorControl;
		GLenum blendSrcFactor,blendDstFactor; // Blend function coefficients for transparent rendering
		GLenum matrixMode; // Current matrix mode
		int activeVertexArraysMask; // Bit mask of currently active vertex arrays, from GLVertexArrayParts
		GLuint vertexBuffer; // ID of currently bound vertex buffer
		GLuint indexBuffer; // ID of currently bound index buffer
		GLhandleARB shaderProgram; // Currently bound shader program, or null
		};
	
	/* Elements: */
	public:
	GLContextData& contextData; // Context data of the current OpenGL context
	private:
	Frustum baseFrustum; // The rendering context's view frustum in initial model coordinates
	Misc::UInt32 initialRenderPass; // The initially active rendering pass
	Misc::UInt32 currentRenderPass; // The currently active rendering pass
	bool modelviewOutdated; // Flag if OpenGL's modelview matrix does not correspond to the current model transformation
	bool haveTextureTransform; // Flag if a texture transformation has been set
	
	/* Elements shadowing current OpenGL state: */
	public:
	GLState initialState; // OpenGL state when render state object was created
	GLState currentState; // Current OpenGL state
	
	/* Private methods: */
	private:
	void loadCurrentTransform(void); // Uploads the current transformation as OpenGL's modelview matrix
	void changeVertexArraysMask(int currentMask,int newMask); // Changes the set of active vertex arrays
	
	/* Constructors and destructors: */
	public:
	GLRenderState(GLContextData& sContextData,const DOGTransform& initialTransform,const Point& sBaseViewerPos,const Vector& sBaseUpVector); // Creates a render state object
	~GLRenderState(void); // Releases OpenGL state and destroys render state object
	
	/* Methods from class TraversalState: */
	void startTraversal(const DOGTransform& newCurrentTransform,const Point& newBaseViewerPos,const Vector& newBaseUpVector);
	DOGTransform pushTransform(const DOGTransform& deltaTransform)
		{
		/* Mark OpenGL's modelview matrix as outdated and update the transformation: */
		modelviewOutdated=true;
		return TraversalState::pushTransform(deltaTransform);
		}
	DOGTransform pushTransform(const ONTransform& deltaTransform)
		{
		/* Mark OpenGL's modelview matrix as outdated and update the transformation: */
		modelviewOutdated=true;
		return TraversalState::pushTransform(deltaTransform);
		}
	DOGTransform pushTransform(const OGTransform& deltaTransform)
		{
		/* Mark OpenGL's modelview matrix as outdated and update the transformation: */
		modelviewOutdated=true;
		return TraversalState::pushTransform(deltaTransform);
		}
	void popTransform(const DOGTransform& previousTransform)
		{
		/* Mark OpenGL's modelview matrix as outdated and update the transformation: */
		modelviewOutdated=true;
		TraversalState::popTransform(previousTransform);
		}
	
	/* New methods: */
	Misc::UInt32 getRenderPass(void) const // Returns the mask flag of the current rendering pass
		{
		return currentRenderPass;
		}
	void setRenderPass(Misc::UInt32 newRenderPass); // Switches to the given rendering pass
	bool doesBoxIntersectFrustum(const Box& box) const; // Returns true if the given box in current model coordinates intersects the view frustum
	void setTextureTransform(const TextureTransform& newTextureTransform); // Sets the given transformation as the new texture transformation
	void resetTextureTransform(void); // Resets the texture transformation
	
	/* OpenGL state management methods: */
	void uploadModelview(void) // Uploads the current transformation into OpenGL's modelview matrix
		{
		/* Upload the current transformation if OpenGL's modelview matrix has not been updated since the last transformation change: */
		if(modelviewOutdated)
			loadCurrentTransform();
		}
	void resetState(void); // Resets OpenGL state to the initial state
	void setFrontFace(GLenum newFrontFace); // Selects whether counter-clockwise or clockwise polygons are front-facing
	void enableCulling(GLenum newCulledFace); // Enables OpenGL face culling
	void disableCulling(void); // Disables OpenGL face culling
	void enableMaterials(void); // Enables OpenGL material rendering
	void disableMaterials(void); // Disables OpenGL material rendering
	void setEmissiveColor(const Color& newEmissiveColor); // Sets the current emissive color
	void enableTexture1D(void); // Enables OpenGL 1D texture mapping
	void bindTexture1D(GLuint textureObjectId) // Binds a 1D texture
		{
		/* Check if the texture to bind is not currently bound: */
		if(currentState.boundTextures[0]!=textureObjectId)
			{
			glBindTexture(GL_TEXTURE_1D,textureObjectId);
			currentState.boundTextures[0]=textureObjectId;
			}
		}
	void enableTexture2D(void); // Enables OpenGL 2D texture mapping
	void bindTexture2D(GLuint textureObjectId) // Binds a 2D texture
		{
		/* Check if the texture to bind is not currently bound: */
		if(currentState.boundTextures[1]!=textureObjectId)
			{
			glBindTexture(GL_TEXTURE_2D,textureObjectId);
			currentState.boundTextures[1]=textureObjectId;
			}
		}
	void disableTextures(void); // Disables OpenGL texture mapping
	void blendFunc(GLenum newBlendSrcFactor,GLenum newBlendDstFactor); // Sets the blending function during the transparent rendering pass
	void enableVertexArrays(int vertexArraysMask) // Enables the given set of vertex arrays
		{
		/* Activate/deactivate arrays as needed and update the current state: */
		changeVertexArraysMask(currentState.activeVertexArraysMask,vertexArraysMask);
		currentState.activeVertexArraysMask=vertexArraysMask;
		}
	void bindVertexBuffer(GLuint newVertexBuffer) // Binds the given vertex buffer
		{
		/* Check if the new buffer is different from the current one: */
		if(currentState.vertexBuffer!=newVertexBuffer)
			{
			/* Bind the new buffer and update the current state: */
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,newVertexBuffer);
			currentState.vertexBuffer=newVertexBuffer;
			}
		}
	void bindIndexBuffer(GLuint newIndexBuffer) // Binds the given index buffer
		{
		/* Check if the new buffer is different from the current one: */
		if(currentState.indexBuffer!=newIndexBuffer)
			{
			/* Bind the new buffer and update the current state: */
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,newIndexBuffer);
			currentState.indexBuffer=newIndexBuffer;
			}
		}
	void bindShader(GLhandleARB newShaderProgram); // Binds the shader program of the current handle by calling glUseProgramObjectARB
	void disableShaders(void); // Unbinds any currently-bound shaders and returns to OpenGL fixed functionality
	};

}

#endif
