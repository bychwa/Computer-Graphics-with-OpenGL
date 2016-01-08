#ifdef WIN32
#include <windows.h>
#endif


#include <GL/glew.h>
#include <GL/freeglut.h>

// Headers for DevIL - a library for loading images of many filetypes
#include <IL/il.h>
#include <IL/ilut.h>

#include <cstdlib>

#include "glutil.h"
#include "float4x4.h"

using namespace chag;


// The shaderProgram holds the vertexShader and fragmentShader
GLuint shaderProgram;

// The vertexArrayObject here will hold the pointers to 
// the vertex data (in positionBuffer) and color data per vertex (in colorBuffer)
GLuint		positionBuffer, colorBuffer, indexBuffer, vertexArrayObject;						



void initGL()
{
	/* Initialize GLEW; this gives us access to OpenGL Extensions.
	 */
	glewInit();  

	/* Print information about OpenGL and ensure that we've got at a context 
	 * that supports least OpenGL 3.0. Then setup the OpenGL Debug message
	 * mechanism.
	 */
	startupGLDiagnostics();
	setupGLDebugMessages();

	/* Initialize DevIL, the image library that we use to load textures. Also
	 * tell IL that we intent to use it with OpenGL.
	 */
	ilInit();
	ilutRenderer(ILUT_OPENGL);

	/* Workaround for AMD. It might no longer be necessary, but I dunno if we
	 * are ever going to remove it. (Consider it a piece of living history.)
	 */
	if( !glBindFragDataLocation )
	{
		glBindFragDataLocation = glBindFragDataLocationEXT;
	}

	/* As a general rule, you shouldn't need to change anything before this 
	 * comment in initGL().
	 */

	///////////////////////////////////////////////////////////////////////////
	// Create the floor quad
	///////////////////////////////////////////////////////////////////////////	
	// Vertex positions
	const float positions[] = {
		// X Y Z
		-10.0f,   -10.0f, -30.0f,    // v0
		-10.0f,   -10.0f, -330.0f,   // v1
		 10.0f,   -10.0f, -330.0f,   // v2
		 10.0f,   -10.0f, -30.0f     // v3
	};

	// Vertex colors
	const float colors[] = {
		//  R     G		B
		1.0f, 1.0f, 1.0f,		// White
		0.5f, 0.5f, 0.8f,		// Lightblue
		0.5f, 0.5f, 0.8f,		// Lightblue
		1.0f, 1.0f, 1.0f		// White
	};


	const int indices[] = {
		0,1,3, // Triangle 1
		1,2,3  // Triangle 2
	};

	// Create the buffer objects
	glGenBuffers( 1, &positionBuffer );															// Create a handle for the vertex position buffer
	glBindBuffer( GL_ARRAY_BUFFER, positionBuffer );											// Set the newly created buffer as the current one
	glBufferData( GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW );			// Send the vetex position data to the current buffer

	glGenBuffers( 1, &colorBuffer );															// Create a handle for the vertex color buffer
	glBindBuffer( GL_ARRAY_BUFFER, colorBuffer );										// Set the newly created buffer as the current one
	glBufferData( GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW );			// Send the color data to the current buffer


	glGenBuffers( 1, &indexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );										
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW );			

	// Create the vertex array object
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	glBindBuffer( GL_ARRAY_BUFFER, positionBuffer );	
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/ );	
	glBindBuffer( GL_ARRAY_BUFFER, colorBuffer );
	glVertexAttribPointer(1, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/ );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);


	// The loadShaderProgram and linkShaderProgam functions are defined in glutil.cpp and 
	// do exactly what we did in lab1 but are hidden for convenience
	shaderProgram = loadShaderProgram("simple.vert", "simple.frag"); 
	glBindAttribLocation(shaderProgram, 0, "position"); 	
	glBindAttribLocation(shaderProgram, 1, "color");
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");
	linkShaderProgram(shaderProgram); 
	//**********************************************

	//************************************
	//			Load Texture
	//************************************


}

void display(void)
{
	glClearColor(0.2,0.2,0.8,1.0);						// Set clear color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the color buffer and the z-buffer

	int w = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int h = glutGet((GLenum)GLUT_WINDOW_HEIGHT);
	glViewport(0, 0, w, h);								// Set viewport

	// We disable backface culling for this tutorial, otherwise care must be taken with the winding order
	// of the vertices. It is however a lot faster to enable culling when drawing large scenes.
	glDisable(GL_CULL_FACE);
	// Disable depth testing
	glDisable(GL_DEPTH_TEST);
	// Shader Program
	glUseProgram( shaderProgram );				// Set the shader program to use for this draw call

	// Set up a projection matrix
	float4x4 projectionMatrix = perspectiveMatrix(45.0f, float(w)/float(h), 0.01f, 300.0f); 
	// Send it to the vertex shader
	int loc = glGetUniformLocation(shaderProgram, "projectionMatrix");
	glUniformMatrix4fv(loc, 1, false, &projectionMatrix.c1.x);


	glBindVertexArray(vertexArrayObject);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


	glUseProgram( 0 ); // "unsets" the current shader program. Not really necessary.

	glutSwapBuffers(); // swap front and back buffer. This frame will now be displayed.
	CHECK_GL_ERROR();
}

void handleKeys(unsigned char key, int /*x*/, int /*y*/)
{
	switch( key )
	{
		// Key 27 => Escape. 
		case 27:
		{
			exit( 0 );
			break;
		}
	}
}

int main(int argc, char *argv[])
{
#	if defined(__linux__)
	linux_initialize_cwd();
#	endif // ! __linux__

	glutInit(&argc, argv);

	/* Request a double buffered window, with a RGB color buffer, and a depth
	 * buffer. Also, request the initial window size to be 512 x 512.
	 */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(512,512);

	/* Require at least OpenGL 3.0. Also request a Debug Context, which allows
	 * us to use the Debug Message API for a somewhat more humane debugging
	 * experience.
	 */
	glutInitContextVersion(3,0);
	glutInitContextFlags(GLUT_DEBUG);

	/* Request window
	 */
	glutCreateWindow("OpenGL Lab 2");

	/* Set callbacks that respond to various events. For now, we only redraw
	 * the window on demand (window resized, etc) and look for key presses
	 * so that we can exit the program somewhat gracefully.
	 */
	glutDisplayFunc(display);
	glutKeyboardFunc(handleKeys); // standard key is pressed/released

	/* Now that we should have a valid GL context, perform our OpenGL 
	 * initialization, before we enter glutMainLoop().
	 */
	initGL();

	/* Start the main loop. Note: depending on your GLUT version, glutMainLoop()
	 * may never return, but only exit via std::exit(0) or a similar method.
	 */
	glutMainLoop();
	return 0;          
}
