#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cstdlib>

#include "glutil.h"

// `vertexArrayObject' holds the data for each vertex. Data for each vertex
// consists of positions (from positionBuffer) and color (from colorBuffer)
// in this example.
GLuint vertexArrayObject;	

// The shaderProgram combines a vertex shader (vertexShader) and a
// fragment shader (fragmentShader) into a single GLSL program that can
// be activated (glUseProgram()).
GLuint shaderProgram;

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

	// Define the positions for each of the three vertices of the triangle
	const float positions[] = {
		//	 X      Y     Z
		0.0f,   0.5f, 1.0f,		// v0
		-0.5f,  -0.5f, 1.0f,	// v1
		0.5f,  -0.5f, 1.0f		// v2
	};

	// Define the colors for each of the three vertices of the triangle
	const float colors[] = {
		//  R     G		B
		1.0f, 1.0f, 1.0f,		// White
		1.0f, 1.0f, 1.0f,		// White
		1.0f, 1.0f, 1.0f		// White
	};

	// Create a handle for the position vertex buffer object
	// See OpenGL Spec §2.9 Buffer Objects 
	// - http://www.cse.chalmers.se/edu/course/TDA361/glspec30.20080923.pdf#page=54
	GLuint positionBuffer; 
	glGenBuffers( 1, &positionBuffer );
	// Set the newly created buffer as the current one
	glBindBuffer( GL_ARRAY_BUFFER, positionBuffer );
	// Send the vertex position data to the current buffer
	glBufferData( GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW );

	// Create a handle for the vertex color buffer
	GLuint colorBuffer; 
	glGenBuffers( 1, &colorBuffer );
	// Set the newly created buffer as the current one
	glBindBuffer( GL_ARRAY_BUFFER, colorBuffer );	
	// Send the vertex color data to the current buffer
	glBufferData( GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW );

	//******* Connect triangle data with the vertex array object *******
	//
	// Connect the vertex buffer objects to the vertex array object
	// See OpenGL Spec §2.10 
	// - http://www.cse.chalmers.se/edu/course/TDA361/glspec30.20080923.pdf#page=64
	glGenVertexArrays(1, &vertexArrayObject);

	// Bind the vertex array object
	// The following calls will affect this vertex array object.
	glBindVertexArray(vertexArrayObject);
	// Makes positionBuffer the current array buffer for subsequent calls.
	glBindBuffer( GL_ARRAY_BUFFER, positionBuffer );
	// Attaches positionBuffer to vertexArrayObject, in the 0th attribute location
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/ );	

	// Makes colorBuffer the current array buffer for subsequent calls.
	glBindBuffer( GL_ARRAY_BUFFER, colorBuffer );
	// Attaches colorBuffer to vertexArrayObject, in the 1st attribute location
	glVertexAttribPointer(1, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/ );

	glEnableVertexAttribArray(0); // Enable the vertex position attribute
	glEnableVertexAttribArray(1); // Enable the vertex color attribute 


	///////////////////////////////////////////////////////////////////////////
	// Create shaders
	///////////////////////////////////////////////////////////////////////////	

	// See OpenGL spec §2.20 http://www.cse.chalmers.se/edu/course/TDA361/glspec30.20080923.pdf#page=104&zoom=75
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Invoke helper functions (in glutil.h/cpp) to load text files for vertex and fragment shaders.
	const char *vs = textFileRead("simple.vert");
	const char *fs = textFileRead("simple.frag");

	glShaderSource(vertexShader, 1, &vs, NULL);
	glShaderSource(fragmentShader, 1, &fs, NULL);

	// we are now done with the source and can free the file data, textFileRead uses new [] to.
  // allocate the memory so we must free it using delete [].
	delete [] vs;
	delete [] fs;

	// Compile the shader, translates into internal representation and checks for errors.
	glCompileShader(vertexShader);
	int compileOK;
	// check for compiler errors in vertex shader.
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compileOK);
	if(!compileOK) {
		std::string err = GetShaderInfoLog(vertexShader);
		fatal_error( err );
		return;
	}

	// Compile the shader, translates into internal representation and checks for errors.
	glCompileShader(fragmentShader);
	// check for compiler errors in fragment shader.
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileOK);
	if(!compileOK) {
		std::string err = GetShaderInfoLog(fragmentShader);
		fatal_error( err );
		return;
	}

	// Create a program object and attach the two shaders we have compiled, the program object contains
	// both vertex and fragment shaders as well as information about uniforms and attributes common to both.
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, fragmentShader);
	glAttachShader(shaderProgram, vertexShader);

	// Now that the fragment and vertex shader has been attached, we no longer need these two separate objects and should delete them.
	// The attachment to the shader program will keep them alive, as long as we keep the shaderProgram.
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );

	// We have previously (in the glVertexAttribPointer calls) decided that our 
	// vertex position data will be the 0th attribute. Bind the attribute with 
	// name "position" to the 0th stream
	glBindAttribLocation(shaderProgram, 0, "position"); 
	// And bind the attribute called "color" in the shader to the 1st attribute
	// stream. 
	glBindAttribLocation(shaderProgram, 1, "color");

	// This tells OpenGL which draw buffer the fragment shader out variable 'fragmentColor' will end up in.
	// Since we only use one output and draw buffer this is actually redundant, as the default will be correct.
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");

	// Link the different shaders that are bound to this program, this creates a final shader that 
	// we can use to render geometry with.
	glLinkProgram(shaderProgram);

	// Check for linker errors, many errors, such as mismatched in and out variables between 
	// vertex/fragment shaders,  do not appear before linking.
	{
		GLint linkOk = 0;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkOk);
		if(!linkOk) 
		{
			std::string err = GetShaderInfoLog(shaderProgram);
			fatal_error( err );
			return;
		}
	}
}


void display(void)
{
	glClearColor(0.2,0.2,0.8,1.0);						// Set clear color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the color buffer and the z-buffer

	int w = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int h = glutGet((GLenum)GLUT_WINDOW_HEIGHT);
	glViewport(0, 0, w, h);						// Set viewport

	// We disable backface culling for this tutorial, otherwise care must be taken with the winding order
	// of the vertices. It is however a lot faster to enable culling when drawing large scenes.
	glDisable(GL_CULL_FACE);

	// Shader Program
	glUseProgram( shaderProgram );			// Set the shader program to use for this draw call
	// Bind the vertex array object that contains all the vertex data.
	glBindVertexArray(vertexArrayObject);
	// Submit triangles from currently bound vertex array object.
	glDrawArrays( GL_TRIANGLES, 0, 3 );				// Render 1 triangle


	glUseProgram( 0 );						// "unsets" the current shader program. Not really necessary.

	glutSwapBuffers();  // swap front and back buffer. This frame will now been displayed.
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
	glutCreateWindow("OpenGL Lab 1");

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
