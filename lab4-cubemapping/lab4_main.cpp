#ifdef WIN32
#include <windows.h>
#endif


#include <GL/glew.h>
#include <GL/freeglut.h>

#include <IL/il.h>
#include <IL/ilut.h>

#include <cstdlib>
#include <algorithm>

#include "glutil.h"
#include "float4x4.h"
#include "float3x3.h"

#include <OBJModel.h>

using std::min;
using std::max;
using namespace chag;

GLuint shaderProgram;
GLuint positionBuffer, texcoordBuffer, vertexArrayObject;						

GLuint texture, cubeMapTexture; 

float currentTime = 0.0f;

bool trigSpecialEvent = false;
bool paused = false;

// Camera position in spherical coordinates
float camera_theta = 0.0f; 
float camera_phi = M_PI/2.0f; 
float camera_r = 25.0; 

// Light position in spherical coordinates
float light_theta = 0.0f;
float light_phi = M_PI/4.0f; 
float light_r = 20.0; 


// Helper function to turn spherical coordinates into cartesian (x,y,z)
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
	//		Specifying the object
	//************************************
	float positions[] = {
		-5.0f,   5.0f, 0.0f,	// v0	-		v0	v2
		-5.0f,  -5.0f, 0.0f,	// v1	-		|  /| 
		5.0f,   5.0f, 0.0f,		// v2	-		| / |
		5.0f,  -5.0f, 0.0f		// v3	-		v1	v3
	};
	float texcoords[] = {
		0.0f, 1.0f, 		// (u,v) for v0	
		0.0f, 0.0f,			// (u,v) for v1
		1.0f, 1.0f,			// (u,v) for v2
		1.0f, 0.0f			// (u,v) for v3
	};


	glGenBuffers( 1, &positionBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, positionBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW );

	glGenBuffers( 1, &texcoordBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, texcoordBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW );


	//******* Connect triangle data with the vertex array object *******
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	glBindBuffer( GL_ARRAY_BUFFER, positionBuffer );	
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/ );	

	glBindBuffer( GL_ARRAY_BUFFER, texcoordBuffer );
	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/ );


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(2);

	//************************************
	//			Create Shaders
	//************************************
	shaderProgram = loadShaderProgram("simple.vert", "simple.frag"); 
	glBindAttribLocation(shaderProgram, 0, "position"); 	
	glBindAttribLocation(shaderProgram, 2, "texCoordIn");
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");

	linkShaderProgram(shaderProgram);

	//************************************
	//	  Set uniforms
	//************************************

	glUseProgram( shaderProgram );					

	// set the 0th texture unit to serve the 'diffuse_texture' sampler.
	setUniformSlow(shaderProgram, "diffuse_texture", 0 );

	//************************************
	//			Load Texture
	//************************************

	texture = ilutGLLoadImage("white-marble.ppm"); // This function returns an id, internally generated by calling glGenTextures(1,&texture);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
	//Indicates that the active texture should be repeated, 
	//instead of for instance clamped, for texture coordinates > 1 or <-1.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Sets the type of mipmap interpolation to be used on magnifying and 
	// minifying the active texture. These are the nicest available options.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 



}

void display(void)
{
	glClearColor(0.1,0.1,0.6,1.0);						// Set clear color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the color buffer and the z-buffer
	glEnable(GL_DEPTH_TEST);	// enable Z-buffering 
	glDisable(GL_CULL_FACE);		// disables not showing back faces of triangles 
	int w = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	int h = glutGet((GLenum)GLUT_WINDOW_HEIGHT);
	glViewport(0, 0, w, h);								// Set viewport

	// Shader Program
	glUseProgram( shaderProgram );				// Set the shader program to use for this draw call

	// Set up matrices
	float4x4 modelMatrix = make_identity<float4x4>();
	float3 camera_position = sphericalToCartesian(camera_theta, camera_phi, camera_r);
	float3 camera_lookAt = make_vector(0.0f, 0.0f, 0.0f);
	float3 camera_up = make_vector(0.0f, 1.0f, 0.0f);
	float4x4 viewMatrix = lookAt(camera_position, 
								 camera_lookAt,	
								 camera_up);

	float4x4 projectionMatrix = perspectiveMatrix(45.0f, float(w) / float(h), 0.01f, 300.0f);
	// Concatenate the three matrices and pass the final transform to the vertex shader
	setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * modelMatrix);
	// set light properties in shader.
	setUniformSlow(shaderProgram, "scene_light", make_vector(0.9f, 0.8f, 0.8f));
	setUniformSlow(shaderProgram, "scene_ambient_light", make_vector(0.2f, 0.2f, 0.2f));

	glBindVertexArray(vertexArrayObject);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	// Draw the lights position
	debugDrawLight(viewMatrix, projectionMatrix, sphericalToCartesian(light_theta, light_phi, light_r)); 

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


bool leftDown = false;
bool middleDown = false;
bool rightDown = false;

int prev_x = 0;
int prev_y = 0;

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
	glutCreateWindow("OpenGL Lab 4");

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
