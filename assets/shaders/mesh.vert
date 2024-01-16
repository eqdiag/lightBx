#version 460

//In
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

//Out
layout(location = 0) out vec3 outColor;

//Uniforms
layout(set = 0,binding = 0) uniform CameraBuffer{
	mat4 view_proj; //view_proj = proj * view
} camera;

struct ModelData{
	mat4 model;
};

layout(std140,set = 1,binding = 0) readonly buffer ModelBuffer{
	ModelData models[];
} modelBuffer;

void main()
{
	gl_Position = camera.view_proj * modelBuffer.models[gl_BaseInstance].model * vec4(position,1.0);
	//vec3 dz = vec3(0,0,gl_BaseInstance);
	//gl_Position = camera.view_proj * vec4(position - dz,1.0);
	outColor = color;
}