#version 130

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

// inputs from vertex shader.
in vec2 texCoord;
in vec3 viewSpacePosition; 
in vec3 viewSpaceNormal; 

// output to frame buffer.
out vec4 fragmentColor;

// global uniforms; same for the whole scene
uniform vec3 scene_light = vec3(0.6, 0.6, 0.6);
uniform vec3 viewSpaceLightPosition; 

// matrial properties; change with the material
uniform int has_diffuse_texture; 
uniform sampler2D diffuse_texture;

uniform vec3 material_diffuse_color; 
uniform vec3 material_emissive_color; 

// helper: compute diffuse lighting term
vec3 calculateDiffuse(vec3 lightDiffuse, vec3 materialDiffuse, vec3 normal, 
	vec3 directionToLight)
{
    return lightDiffuse * materialDiffuse * max(0, dot(normal, directionToLight));
}

void main() 
{
	// prepare lighting computations
	vec3 diffuse = material_diffuse_color;
	vec3 emissive = material_emissive_color;
	
	if( has_diffuse_texture == 1 )
	{
		// if we've got textures, modulate with the texture color
		diffuse *= texture(diffuse_texture, texCoord.xy).xyz; 
		emissive *= texture(diffuse_texture, texCoord.xy).xyz; 
	}

	// precalculate some terms
	vec3 normal = normalize(viewSpaceNormal);
	vec3 directionToLight = normalize(viewSpaceLightPosition - viewSpacePosition);
	
	// apply diffuse + emissive terms
	vec3 shading = calculateDiffuse( scene_light, diffuse, normal, directionToLight )
		+ emissive;
				 
	// write output color
	fragmentColor = vec4(shading, 1.0);
}
