#version 130

in vec3		position;
in vec3		color;
out vec3	outColor;
uniform mat4 projectionMatrix; 


void main() 
{
	gl_Position = projectionMatrix * vec4(position, 1);
	outColor = color; 
}