#version 460

//In
layout(location = 0) in vec3 position;


//Out
layout(location = 0) out vec3 outColor;

//Constants
layout(set = 0,binding = 0) uniform CameraBuffer{
	mat4 view_proj; //view_proj = proj * view
} camera;

struct LightEntity{ 
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 falloff;
};

layout(std140,set = 0,binding = 1) readonly buffer LightsBuffer{
	LightEntity data[];
} lights;

void main()
{
	vec3 light_position = lights.data[gl_InstanceIndex].position.xyz;
	gl_Position = camera.view_proj  * vec4(position + light_position,1.0);
	outColor = lights.data[gl_InstanceIndex].diffuse.xyz;
}