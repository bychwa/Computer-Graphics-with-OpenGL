#version 130

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

uniform float material_shininess = 25.0;
uniform vec3 material_diffuse_color = vec3(1.0); 
uniform vec3 material_specular_color = vec3(0.0); 
uniform vec3 material_emissive_color = vec3(0.0); 

uniform int has_diffuse_texture = 1;
uniform sampler2D diffuse_texture;


in vec4 outColor;
in vec2 texCoord;

uniform vec3 viewSpaceLightPosition;
uniform vec3 scene_ambient_light = vec3(0.05, 0.05, 0.05);
uniform vec3 scene_light = vec3(0.6, 0.6, 0.6);


out vec4 fragmentColor;


vec3 calculateAmbient(vec3 ambientLight, vec3 materialAmbient)
{
	return vec3(0.0);// TODO #1: replace with ambient light calculations
}

vec3 calculateDiffuse(vec3 diffuseLight, vec3 materialDiffuse, vec3 normal, vec3 directionToLight)
{
	return materialDiffuse; // TODO #2: replace with diffuse light calculations
}

vec3 calculateSpecular(vec3 specularLight, vec3 materialSpecular, float materialShininess, vec3 normal, vec3 directionToLight, vec3 directionFromEye)
{
	return vec3(0.0); // TODO #3: insert specular calculations
}

vec3 calculateFresnel(vec3 materialSpecular, vec3 normal, vec3 directionFromEye)
{
	return materialSpecular; // TODO #4: calculate fresnel term
}

/**
 * Helper function to sample the diffuse texture, if it is present. This needed is to support materials
 * which do not supply a texture, e.g. the space ship in lab 4 and 5. Another way would be to make 
 * OBJModel supply a default texture which is all-white, but this was deemed somewhat more obscure.
 */
vec3 sampleDiffuseTexture()
{
	if (has_diffuse_texture == 1) 
	{
		return texture(diffuse_texture, texCoord).xyz;
	}
	return vec3(1.0);
}

void main() 
{
	fragmentColor = vec4( texture( diffuse_texture, texCoord ).xyz, 1.0 );
}
