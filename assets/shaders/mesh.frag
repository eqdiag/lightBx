#version 450

//In
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;


//Out
layout(location = 0) out vec4 outColor;

//Constants
struct LightEntity{ 
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

layout(std140,set = 0,binding = 1) readonly buffer LightsBuffer{
	LightEntity data[];
} lights;

layout(std140,set = 0,binding = 3) uniform Material{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 shiny;
} material;

void main()
{
	vec3 l = lights.data[0].position.xyz;

	vec3 dx = l-inPosition;

	float v = 1.0 / length(dx);

	outColor = vec4(vec3(0,1,0)*v,1.0);
}