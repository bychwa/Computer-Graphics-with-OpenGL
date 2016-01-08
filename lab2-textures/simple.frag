#version 130

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

in vec3 outColor;


out vec4 fragmentColor;

void main() 
{
	fragmentColor = vec4(outColor, 1.0); 
}