#version 460

//In
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

//Out
layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outColor;


//Constants
layout(set = 0,binding = 0) uniform CameraBuffer{
	mat4 view_proj; //view_proj = proj * view
} camera;


layout(std140,set = 0,binding = 2) readonly buffer ObjectTransforms{
	mat4 data[];
} objects;


void main()
{
	gl_Position = camera.view_proj  * objects.data[gl_InstanceIndex] * vec4(position,1.0);
	outPosition = (objects.data[gl_InstanceIndex] * vec4(position,1.0)).xyz;
	outColor = color;
}