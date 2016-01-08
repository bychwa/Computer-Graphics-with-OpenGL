#ifdef WIN32
#include <windows.h>
#endif


#include <GL/glew.h>
#include <GL/freeglut.h>

#include <IL/il.h>
#include <IL/ilut.h>

#include <cstdlib>
#include <algorithm>

#include "float4x4.h"
#include "float3x3.h"

#include <glutil.h>
#include <OBJModel.h>


using std::min;
using std::max;
using namespace chag;

GLuint shaderProgram;
GLuint postFxShader;


bool leftDown = false;
bool middleDown = false;
bool rightDown = false;

float currentTime = 0.0f;


float camera_theta = M_PI / 4.0f; 
float camera_phi = M_PI / 4.0f; 
float camera_r = 17.0; 

float3 sphericalToCartesian(float theta, float phi, float r)
{
	return make_vector( r * sinf(theta)*sinf(phi),
					 	r * cosf(phi), 
						r * cosf(theta)*sinf(phi) );
}


float3 securityCamPos = { 7.0f, 7.0f, 2.0f };
float3 securityCamTarget = { 0.0f, 1.0f, 0.0f };

// Scene up orientation, used when creating view matrices, handy to define somewhere...
const float3 up = {0.0f, 1.0f, 0.0f};

const float3 lightPosition = { 10.0f, 10.0f, 10.0f };


OBJModel *boxModel = 0;
OBJModel *cameraModel = 0;
OBJModel *securityConsoleModel = 0;
OBJModel *fighterModel = 0;

// Used to draw the geometry on the security console screens. 
// Implemented at the bottom of this file. 
void drawSecurityScreenQuad();

// Used to draw a full-screen quad, used in post processing effects.
void drawFullScreenQuad();






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

	// enable Z-buffering 
	glEnable(GL_DEPTH_TEST);	

	// Load some models. Uses OBJModel - you don't have to look at the OBJModel
	// implementation at this point.
	boxModel = new OBJModel;
	boxModel->load("../scenes/lab5.obj");

	cameraModel = new OBJModel;
	cameraModel->load("../scenes/camera.obj");

	securityConsoleModel = new OBJModel;
	securityConsoleModel->load("../scenes/security_console.obj");

	fighterModel = new OBJModel;
	fighterModel->load("../scenes/fighter.obj");
  
	// load and set up default shader
	shaderProgram = loadShaderProgram("shaders/simple.vert", "shaders/simple.frag");
	
	glBindAttribLocation(shaderProgram, 0, "position");
	glBindAttribLocation(shaderProgram, 2, "texCoordIn");

	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");

	linkShaderProgram(shaderProgram);

	// load and set up post processing shader
 	postFxShader = loadShaderProgram("shaders/postFx.vert", "shaders/postFx.frag");
	
	glBindAttribLocation(postFxShader, 0, "position");	
	glBindFragDataLocation(postFxShader, 0, "fragmentColor");

	linkShaderProgram(postFxShader);

	// create the Frame Buffer Object (FBO) that we render to, and later
	// use as a texture.
	int w = glutGet( (GLenum)GLUT_WINDOW_WIDTH );
	int h = glutGet( (GLenum)GLUT_WINDOW_HEIGHT );

}



void drawSecurityConsole(const float3 &position, float orientation, const float4x4 &view, const float4x4 &projection)
{
	float4x4 viewProjection = projection * view;
  float4x4 consoleModelMatrix = make_translation(position)
    * make_rotation_y<float4x4>(orientation);
	
	setUniformSlow(shaderProgram, "modelViewProjectionMatrix", viewProjection * consoleModelMatrix);
	setUniformSlow(shaderProgram, "modelViewMatrix", view * consoleModelMatrix);
	setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * consoleModelMatrix)));

	setUniformSlow(shaderProgram, "has_diffuse_texture", true);
	setUniformSlow(shaderProgram, "diffuse_texture", 0);
	setUniformSlow(shaderProgram, "material_diffuse_color", 
		make_vector(0.0f, 0.0f, 0.0f));
	setUniformSlow(shaderProgram, "material_emissive_color", 
		make_vector(1.5f, 1.5f, 1.5f));


  // insert texture binding here...
  // draw security screen here...
  securityConsoleModel->render();
}


void drawScene(GLuint shaderProgram, const float4x4 &view, const float4x4 &projection)
{
	float3 lightPosition = make_vector(-2.8f, 6.0f, 0.2f);
	float4 viewSpaceLightPosition = view * make_vector4(lightPosition, 1.0f);
	setUniformSlow(shaderProgram, "viewSpaceLightPosition", 
		make_vector3(viewSpaceLightPosition)
	);

	// set the 0th texture unit to serve the 'diffuse_texture' sampler.
	setUniformSlow(shaderProgram, "diffuse_texture", 0 );
	glActiveTexture(GL_TEXTURE0); 
   
	float4x4 viewProjection = projection * view;

	setUniformSlow(shaderProgram, "modelViewProjectionMatrix", viewProjection);
	setUniformSlow(shaderProgram, "modelViewMatrix", view);
	setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view)));

	boxModel->render();

	// lift out of floor, rotate and scale down as it is quite big.
	float4x4 fighterModelMatrix = make_translation(make_vector(0.0f, 1.5f, 0.0f))
						  * make_rotation_y<float4x4>(currentTime * -M_PI / 4.0f)
						  * make_scale<float4x4>(0.2f);

	setUniformSlow(shaderProgram, "modelViewProjectionMatrix", viewProjection * fighterModelMatrix);
	setUniformSlow(shaderProgram, "modelViewMatrix", view * fighterModelMatrix);
	setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * fighterModelMatrix)));

	fighterModel->render();

	//
	// Position and draw a few copies of the security console model: To make
	// this simple we have created a helper function that takes as arguments
	// the position and orientation of the console.
	drawSecurityConsole(make_vector(4.0f, 0.5f, 4.0f), -M_PI / 4.0f, view, projection);
	drawSecurityConsole(make_vector(2.0f, 0.5f, 4.0f), -M_PI / 3.0f, view, projection);
	drawSecurityConsole(make_vector(4.0f, 0.5f, 1.5f), -M_PI / 5.0f, view, projection);

	//
	// Draw the other camera.
	float4x4 cameraModelMatrix = make_matrix_from_zAxis(securityCamPos, securityCamTarget -  securityCamPos, up);
	setUniformSlow(shaderProgram, "modelViewProjectionMatrix", viewProjection * cameraModelMatrix);
	setUniformSlow(shaderProgram, "modelViewMatrix", view * cameraModelMatrix);
	setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * cameraModelMatrix)));

	cameraModel->render();
}



void display(void)
{
	// Update time in PostFX Shader (required by the 'shrooms effect)
	glUseProgram(postFxShader);	
	setUniformSlow(postFxShader, "time", currentTime);
	glUseProgram(0);

	// Shader Program
	glUseProgram(shaderProgram);

	int w = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int h = glutGet((GLenum)GLUT_WINDOW_HEIGHT);

	// Insert FBO rendering here

	// Set the viewport of the default frame buffer. Since the default frame
	// buffer is shown in the main window, we use the window size.

	// clear 
	glClearColor(0.0,0.0,0.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

 	// setup matrices and draw scene
	float4x4 viewMatrix = lookAt(
		sphericalToCartesian(camera_theta, camera_phi, camera_r), 
		make_vector(0.0f, 0.0f, 0.0f),	
		up
	);	
	float4x4 projectionMatrix = perspectiveMatrix(
		45.0f, float(w) / float(h), 0.01f, 300.0f
	);



	drawScene(shaderProgram, viewMatrix, projectionMatrix);  


	glUseProgram( 0 );	

	glutSwapBuffers();  // swap front and back buffer. This frame will now be displayed.
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

int prev_x = 0;
int prev_y = 0;

void mouse(int button, int state, int x, int y)
{
	prev_x = x;
	prev_y = y;

	bool buttonDown = state == GLUT_DOWN;

	switch(button)
	{
	case GLUT_LEFT_BUTTON:
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
	static float startTime = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
	currentTime = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f - startTime;

	// Here is a good place to put application logic.


	// Tells glut that the window needs to be redisplayed again. This forces
	// the display to be redrawn over and over again. 
	glutPostRedisplay();  
}



int main(int argc, char *argv[])
{
#	if defined(__linux__)
	linux_initialize_cwd();
#	endif // ! __linux__

	glutInit(&argc, argv);

	/* Request a double buffered window, with a sRGB color buffer, and a depth
	 * buffer. Also, request the initial window size to be 512 x 512.
	 *
	 * Note: not all versions of GLUT define GLUT_SRGB; fall back to "normal"
	 * RGB for those versions.
	 */
#	if defined(GLUT_SRGB)
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_SRGB | GLUT_DEPTH);
#	else // !GLUT_SRGB
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	printf( "--\n" );
	printf( "-- WARNING: your GLUT doesn't support sRGB / GLUT_SRGB\n" );
#	endif // ~ GLUT_SRGB
	glutInitWindowSize(512,512);

	/* Require at least OpenGL 3.0. Also request a Debug Context, which allows
	 * us to use the Debug Message API for a somewhat more humane debugging
	 * experience.
	 */
	glutInitContextVersion(3,0);
	glutInitContextFlags(GLUT_DEBUG);

	/* Request window
	 */
	glutCreateWindow("OpenGL Lab 5");

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

	/* If sRGB is available, enable rendering in sRGB. Note: we should do
	 * this *after* initGL(), since initGL() initializes GLEW.
	 */
	glEnable(GL_FRAMEBUFFER_SRGB);

	/* Start the main loop. Note: depending on your GLUT version, glutMainLoop()
	 * may never return, but only exit via std::exit(0) or a similar method.
	 */
	glutMainLoop();
	return 0;          
}



void drawSecurityScreenQuad()
{
	static GLuint vertexArrayObject = 0; 
	static int nofVertices = 6; 

  // do this initialization first time the function is called...
	if (vertexArrayObject == 0)
	{
    // Note: this data (positions, tex coords, and normals) has been pulled out of the scene "../scenes/security_console.obj".
    //       Should the scene be modified, the screen may not align correctly.
		glGenVertexArrays(1, &vertexArrayObject); 
    static const float3 positions[] = {
      { -0.318522, 1.920508, 0.357336 },
      { 0.102965, 1.589634, 0.357336 },
      { 0.102965, 1.589634, -0.357336 },
      { -0.318522, 1.920508, 0.357336 },
      { 0.102965, 1.589634, -0.357336 },
      { -0.318522, 1.920508, -0.357336 },
    };
		createAddAttribBuffer(vertexArrayObject, positions, sizeof(positions), 0, 3, GL_FLOAT);
    static const float2 texCoords[] = {
      { 0.0f, 1.0f },
      { 0.0f, 0.0f },
      { 1.0f, 0.0f },
      { 0.0f, 1.0f },
      { 1.0f, 0.0f },
      { 1.0f, 1.0f },
    };
		createAddAttribBuffer(vertexArrayObject, texCoords, sizeof(texCoords), 2, 2, GL_FLOAT);
    static const float3 normals[] = {
      { 0.779114, 0.626882, 0.000000 },
      { 0.556536, 0.830824, 0.000000 },
      { 0.556536, 0.830824, 0.000000 },
      { 0.779114, 0.626882, 0.000000 },
      { 0.556536, 0.830824, 0.000000 },
      { 0.779114, 0.626882, 0.000000 },
    };
		createAddAttribBuffer(vertexArrayObject, normals, sizeof(normals), 3, 3, GL_FLOAT);
	}

	glBindVertexArray(vertexArrayObject); 
	glDrawArrays(GL_TRIANGLES, 0, nofVertices); 
}




void drawFullScreenQuad()
{
	static GLuint vertexArrayObject = 0; 
	static int nofVertices = 4; 

  // do this initialization first time the function is called... somewhat dodgy, but works for demonstration purposes
	if (vertexArrayObject == 0)
	{
		glGenVertexArrays(1, &vertexArrayObject); 
    static const float2 positions[] = {
      {-1.0f, -1.0f},
      { 1.0f, -1.0f},
      { 1.0f,  1.0f},
      {-1.0f,  1.0f},
    };
		createAddAttribBuffer(vertexArrayObject, positions, sizeof(positions), 0, 2, GL_FLOAT);
	}

	glBindVertexArray(vertexArrayObject); 
	glDrawArrays(GL_QUADS, 0, nofVertices); 
}




