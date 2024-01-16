#version 450

//In
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

//Out
layout(location = 0) out vec3 outColor;

//Uniforms
layout(set = 0,binding = 0) uniform CameraBuffer{
	mat4 view_proj; //view_proj = proj * view
} camera;

void main()
{
	gl_Position = camera.view_proj * vec4(position,1.0);
	outColor = color;
}