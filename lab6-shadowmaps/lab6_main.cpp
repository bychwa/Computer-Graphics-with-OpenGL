#ifdef WIN32
#include <windows.h>
#endif


#include <GL/glew.h>
#include <GL/freeglut.h>

#include <IL/il.h>
#include <IL/ilut.h>

#include <cmath>
#include <cstdlib>
#include <algorithm>

#include "glutil.h"
#include "float4x4.h"
#include "float3x3.h"

#include <OBJModel.h>


using std::min;
using std::max;
using namespace chag;

// Shader for rendering the final image
GLuint shaderProgram;
// Shader used to draw the shadow map (and some other simple objects)
GLuint simpleShaderProgram; 


float currentTime = 0.0f;

bool trigSpecialEvent = false;
bool paused = false;

// Scene up orientation, used when creating view matrices, handy to define somewhere...
const float3 up = {0.0f, 1.0f, 0.0f};

float camera_theta = M_PI / 4.0f; 
float camera_phi = M_PI / 6.0f; 
float camera_r = 30.0; 

float3 sphericalToCartesian(float theta, float phi, float r)
{
	return make_vector( r * sinf(theta)*sinf(phi),
					 	r * cosf(phi), 
						r * cosf(theta)*sinf(phi) );
}


// lights angle around y-axis
float3 lightPosition; 

// Model matrices
float4x4 roomModelMatrix;
float4x4 fighterModelMatrix;

// And models
OBJModel *fighterModel = 0;
OBJModel *boxModel = 0;


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
	//		Load Shaders
	//************************************
	shaderProgram = loadShaderProgram("shading.vert", "shading.frag");
		glBindAttribLocation(shaderProgram, 0, "position"); 	
		glBindAttribLocation(shaderProgram, 1, "normalIn");
		glBindAttribLocation(shaderProgram, 2, "texCoordIn");
		glBindFragDataLocation(shaderProgram, 0, "fragmentColor");
	linkShaderProgram(shaderProgram);

	simpleShaderProgram = loadShaderProgram("simple.vert", "simple.frag");
		glBindAttribLocation(simpleShaderProgram, 0, "position"); 	
		glBindFragDataLocation(simpleShaderProgram, 0, "fragmentColor");
	linkShaderProgram(simpleShaderProgram);

	//************************************
	// Load models and set up model matrices
	//************************************
	fighterModel = new OBJModel;
	fighterModel->load("../scenes/fighter.obj");

	boxModel = new OBJModel;
	boxModel->load("../scenes/lab5.obj");
  
	roomModelMatrix = make_scale<float4x4>(make_vector(3.5f, 1.0f, 3.5f));
	fighterModelMatrix = make_translation(make_vector(0.0f, 3.5f, 0.0f))
                          * make_rotation_y<float4x4>(currentTime * -M_PI / 4.0f)
                          * make_scale<float4x4>(0.2f);


	glEnable(GL_DEPTH_TEST);	// enable Z-buffering 
	glEnable(GL_CULL_FACE);		// enables backface culling
}


/**
 * Helper function to set the matrices used for transform an lighting in our
 * shaders, this code is factored into its own function as we use it more than
 * once (needed once for each model drawn).
 */
void setLightingMatrices(GLuint shaderProgram, const float4x4 &viewMatrix, const float4x4 &projectionMatrix, const float4x4 &modelMatrix)
{
	float4x4 modelViewMatrix = viewMatrix * modelMatrix;	
	float4x4 modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
	float4x4 normalMatrix = transpose(inverse(modelViewMatrix));
	// Update the matrices used in the vertex shader
	setUniformSlow(shaderProgram, "modelViewMatrix", modelViewMatrix);
	setUniformSlow(shaderProgram, "modelViewProjectionMatrix", modelViewProjectionMatrix);
	setUniformSlow(shaderProgram, "normalMatrix", normalMatrix);
}


/** In this function, we draw all scene elements that should cast shadow. This
 * function is called twice, once for rendering the shadow map, and once when
 * drawing the visible scene.
 */
void drawShadowCasters(GLuint shaderProgram, const float4x4 &viewMatrix, const float4x4 &projectionMatrix)
{
	// Draw "room"
	setLightingMatrices(shaderProgram, viewMatrix, projectionMatrix, roomModelMatrix);
	boxModel->render();

	// Draw space ship
	setLightingMatrices(shaderProgram, viewMatrix, projectionMatrix, fighterModelMatrix);
	fighterModel->render();
}


void drawScene(const float4x4 &viewMatrix, const float4x4 &projectionMatrix, const float4x4 &lightViewMatrix, const float4x4 &lightProjectionMatrix)
{
	int w = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int h = glutGet((GLenum)GLUT_WINDOW_HEIGHT);

	// Set viewport
	glViewport(0, 0, w, h);								

	// Set clear color and clear color+depth buffer
	glClearColor(0.2,0.2,0.8,1.0);						
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw light for reference
	debugDrawLight(viewMatrix, projectionMatrix, lightPosition); 

	// Use default shader for rendering
	glUseProgram( shaderProgram );


	// set the 0th texture unit to serve the 'diffuse_texture' sampler.
	// Note: this must match the texture unit that OBJModel::render() attempts
	// to use. (See OBJModel.cpp around line 520.)
	setUniformSlow(shaderProgram, "diffuse_texture", 0 );

	// Set the lights view space coordinates to the shaders
	float3 viewSpaceLightPos = transformPoint(viewMatrix, lightPosition); 
	setUniformSlow(shaderProgram, "viewSpaceLightPosition", viewSpaceLightPos);


	// draw objects in scene
	drawShadowCasters( shaderProgram, viewMatrix, projectionMatrix );
	
	// clean up
	glUseProgram( 0 );	
}


void drawShadowMap(const float4x4 &viewMatrix, const float4x4 &projectionMatrix)
{

	glClearColor( 1.0, 1.0, 1.0, 1.0 );
	glClearDepth( 1.0 );
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );


	// Get current shader, so we can restore it afterwards. Also, switch to
	// the simple shader used to draw the shadow map.
	GLint currentProgram; 
	glGetIntegerv( GL_CURRENT_PROGRAM, &currentProgram );
	glUseProgram( simpleShaderProgram );

	// draw shadow casters
	drawShadowCasters( simpleShaderProgram, viewMatrix, projectionMatrix );

	// Restore old shader
	glUseProgram( currentProgram );	

}


void display(void)
{
	// construct light matrices
	float4x4 lightViewMatrix = lookAt(lightPosition, make_vector(0.0f, 0.0f, 0.0f), up);
	float4x4 lightProjMatrix = perspectiveMatrix(45.0f, 1.0, 5.0f, 100.0f);


	int w = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int h = glutGet((GLenum)GLUT_WINDOW_HEIGHT);

	// construct camera matrices
	float4x4 viewMatrix = lookAt(
		sphericalToCartesian(camera_theta, camera_phi, camera_r), 
		make_vector(0.0f, 0.0f, 0.0f),	
		up );	
	float4x4 projMatrix = perspectiveMatrix(
		45.0f, 
		float(w) / float(h), 
		0.01f, 300.0f
	);

	// draw scene
	drawScene( viewMatrix, projMatrix, lightViewMatrix, lightProjMatrix );

	// Swap buffers. Eventually displays the current frame.
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
			break;
		case GLUT_KEY_RIGHT:
			break;
		case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:
			break;
	}
}


static bool leftDown = false;
static bool middleDown = false;
static bool rightDown = false;

static int prev_x = 0;
static int prev_y = 0;

void mouse(int button, int state, int x, int y)
{
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
	
	if(middleDown)
	{
		camera_r -= float(delta_y) * 0.3f;
		// make sure cameraDistance does not become too small
		camera_r = max(0.1f, camera_r);
	}
	if(leftDown)
	{
		camera_phi	-= float(delta_y) * 0.3f * float(M_PI) / 180.0f;
		camera_phi = min(max(0.01f, camera_phi), float(M_PI) - 0.01f);
		camera_theta -= float(delta_x) * 0.3f * float(M_PI) / 180.0f;
	}
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
	float lightYAngle = 0.5f * currentTime; 
	lightPosition = make_rotation_y<float3x3>(lightYAngle) * make_vector(10.0f, 10.0f, 0.0f); 

	glutPostRedisplay(); 
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
	glutCreateWindow("OpenGL Lab 6");

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
