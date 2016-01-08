#version 130

// inputs from OpenGL
in vec3	position;
in vec2 texCoordIn;	
in vec3 normalIn;

// outputs to the fragment shader
out	vec2 texCoord;
out vec3 viewSpaceNormal; 
out vec3 viewSpacePosition; 

// transforms
uniform mat4 normalMatrix;	 
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix; 

void main() 
{
	// copy texture coordinates
	texCoord = texCoordIn;

	// transform position and normal to view space
	viewSpacePosition = (modelViewMatrix * vec4(position, 1.0)).xyz;
	viewSpaceNormal = normalize( (normalMatrix * vec4(normalIn, 0.0)).xyz );

	// project 
	gl_Position = modelViewProjectionMatrix * vec4(position,1.0);
}
