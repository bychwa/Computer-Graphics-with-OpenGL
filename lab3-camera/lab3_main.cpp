#ifdef WIN32
#include <windows.h>
#endif


#include <GL/glew.h>
#include <GL/freeglut.h>

#include <IL/il.h>
#include <IL/ilut.h>

#include <sstream>
#include <cstdlib>
#include <algorithm>

#include "glutil.h"
#include "float4x4.h"
#include "float3x3.h"
#include "float2.h"

#include "Box.h"

using std::min;
using std::max;
using namespace chag; 

GLuint shaderProgram;

float currentTime = 0.0f;

bool trigSpecialEvent = false;
bool paused = false;


Box *myBox; 


float3 sphericalToCartesian(const float &theta, const float &phi, const float &r)
{
	return make_vector( r * sinf(theta)*sinf(phi),
					 	r * cosf(phi), 
						r * cosf(theta)*sinf(phi) );
}


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

	//************************************
	//			Create Shaders
	//************************************
	// The loadShaderProgram and linkShaderProgam functions are defined in glutil.cpp and 
	// do exactly what we did in lab1 but are hidden for convenience
	shaderProgram = loadShaderProgram("simple.vert", "simple.frag"); 
	glBindAttribLocation(shaderProgram, 0, "position"); 	
	glBindAttribLocation(shaderProgram, 1, "color");
	glBindAttribLocation(shaderProgram, 2, "texCoordIn");
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");
	linkShaderProgram(shaderProgram); 

	//************************************
	//	  Set uniforms
	//************************************
	glUseProgram( shaderProgram );					

	// Get the location in the shader for uniform tex0
	int texLoc = glGetUniformLocation( shaderProgram, "colortexture" );	
	// Set colortexture to 0, to associate it with texture unit 0
	glUniform1i( texLoc, 0 );									

	///////////////////////////////////////////////////////////////////////////
	// Create the box. All bufferobjects and texture loading are done in the
	// constructor. 
	///////////////////////////////////////////////////////////////////////////
	myBox = new Box(); 
}

void display(void)
{
	// Set up
	glClearColor(0.2,0.2,0.8,1.0);						// Set clear color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the color buffer and the z-buffer
	glEnable(GL_DEPTH_TEST);							// enable Z-buffering 
	glDisable(GL_CULL_FACE);							// disables not showing back faces of triangles 
	int w = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int h = glutGet((GLenum)GLUT_WINDOW_HEIGHT);
	glViewport(0, 0, w, h);								// Set viewport

	// Set the shader program to use for this draw call
	glUseProgram( shaderProgram );				

	// Set up the matrices
	// The model matrix can be used to move the object around in the world,
	// currently we set it to identity.
	float4x4 modelMatrix = make_identity<float4x4>();
	// The view matrix defines where the viewer is looking
	// currently we set it to identity.
	float4x4 viewMatrix = make_identity<float4x4>();

	float4x4 projectionMatrix = perspectiveMatrix(45.0f, float(w) / float(h), 0.01f, 300.0f);

	// Concatenate the three matrices and pass the final transform to the vertex shader
	float4x4 modelViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;
	int loc = glGetUniformLocation(shaderProgram, "modelViewProjectionMatrix");
	glUniformMatrix4fv(loc, 1, false, &modelViewProjectionMatrix.c1.x);

	// Draw the box
	myBox->draw();


	glUseProgram( 0 );	

	// swap front and back buffer. This frame will now be displayed.
	glutSwapBuffers();  
	CHECK_GL_ERROR();
}

void handleKeys(unsigned char key, int /*x*/, int /*y*/)
{
	switch(key)
	{
	case 27:    /* ESC */
		exit(0); /* dirty exit */
		break;   /* unnecessary, I know */
	case 32:    /* space */
 		break;
	}
}

void handleSpecialKeys(int key, int /*x*/, int /*y*/)
{
	switch(key)
	{
	case GLUT_KEY_LEFT:
		printf("Left arrow\n");
		break;
	case GLUT_KEY_RIGHT:
		printf("Right arrow\n");
		break;
	case GLUT_KEY_UP:
	case GLUT_KEY_DOWN:
		break;
	}
}

bool leftDown = false;
bool middleDown = false;
bool rightDown = false;

int prev_x = 0;
int prev_y = 0;

void mouse(int button, int state, int x, int y)
{
	// Log mouse interaction. Be careful about the value of `button', there's
	// people with significantly more than three buttons on their mice...
	{
		static const char* namedButtonStr[] = {
			"GLUT_LEFT_BUTTON",
			"GLUT_MIDDLE_BUTTON",
			"GLUT_RIGHT_BUTTON"
		};
		
		std::stringstream buttonStr;
		if( button >= 0 && button < int(sizeof(namedButtonStr)/sizeof(namedButtonStr[0])) )
			buttonStr << namedButtonStr[button];
		else
			buttonStr << "glut-button-id(" << button << ")";

		static const char* namedButtonState[] = { "GLUT_DOWN", "GLUT_UP" };

		std::stringstream buttonState;
		if( state >= 0 && state < int(sizeof(namedButtonState)/sizeof(namedButtonState[0])) )
			buttonState << namedButtonState[state];
		else
			buttonState << "glut-button-state(" << state << ")";

		printf( "mouse(): button=%s, state=%s, pos=(%d,%d)\n",
			buttonStr.str().c_str(),
			buttonState.str().c_str(),
			x, y
		);
	}

	// reset the previous position, such that we only get movement performed after the button
	// was pressed.
	prev_x = x;
	prev_y = y;

	bool buttonDown = state == GLUT_DOWN;

	switch(button)
	{
	case GLUT_LEFT_BUTTON:
		if(leftDown != buttonDown)
			trigSpecialEvent = !trigSpecialEvent;
		leftDown = buttonDown;
		break;
	case GLUT_MIDDLE_BUTTON:
		middleDown = buttonDown;
		break;
	case GLUT_RIGHT_BUTTON: 
		rightDown = buttonDown;
	default:
		break;
	}
}


void motion(int x, int y)
{
	int delta_x = x - prev_x;
	int delta_y = y - prev_y;

	printf("Motion: %d %d\n", x, y);

	prev_x = x;
	prev_y = y;
}

void idle( void )
{
	// glutGet(GLUT_ELAPSED_TIME) returns the time since application start in milliseconds.

	// this is updated the first time we enter this function, otherwise we will take the
	// time from the start of the application, which can sometimes be long.
  static float startTime = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
	// update the global time if the application is not paused.
	if (!paused)
  {
    currentTime = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f - startTime;
  }

	// Here is a good place to put application logic.

	glutPostRedisplay(); 
	// Uncommenting the line above tells glut that the window 
	// needs to be redisplayed again. This forces the display to be redrawn
	// over and over again. 
}

int main(int argc, char *argv[])
{
#	if defined(__linux__)
	linux_initialize_cwd();
#	endif // ! __linux__


#	if 0
	glutInit(&argc, argv);
	/* open window of size 800x600 with double buffering, RGB colors, and Z-buffering */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(512,512);
	glutCreateWindow("OpenGL Lab 3");
	glutKeyboardFunc(handleKeys);
	glutSpecialFunc(handleSpecialKeys);
	/* the display function is called once when the gluMainLoop is called,
    * but also each time the window has to be redrawn due to window 
    * changes (overlap, resize, etc). It can also be forced to be called
    * by making a glutPostRedisplay() call 
    */
	glutDisplayFunc(display);	// This is the main redraw function
	glutMouseFunc(mouse);		// callback function on mouse buttons
	glutMotionFunc(motion);		// callback function on mouse movements
	glutIdleFunc( idle );
	
	glutDisplayFunc(display);	// Set the main redraw function
	
	initGL();

	glutMainLoop();  /* start the program main loop */
#	endif
	
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
	glutCreateWindow("OpenGL Lab 3");

	/* Set callbacks that respond to various events. Most of these should be
	 * rather self-explanatory (i.e., the MouseFunc is called in response to
	 * a mouse button press/release). The most important callbacks are however
	 *
	 *   - glutDisplayFunc : called whenever the window is to be redrawn
	 *   - glutIdleFunc : called repeatedly
	 *
	 * The window is redrawn once at startup (at the beginning of
	 * glutMainLoop()), and whenever the window changes (overlap, resize, ...).
	 * To repeatedly redraw the window, we need to manually request that via
	 * glutPostRedisplay(). We call this from the glutIdleFunc.
	 */
	glutIdleFunc(idle);
	glutDisplayFunc(display);

	glutKeyboardFunc(handleKeys); // standard key is pressed/released
	glutSpecialFunc(handleSpecialKeys); // "special" key is pressed/released
	glutMouseFunc(mouse); // mouse button pressed/released
	glutMotionFunc(motion); // mouse moved *while* any button is pressed

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
