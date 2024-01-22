#version 460

//In
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;


//Out
layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoords;

//Constants

layout(set = 0,binding = 0) uniform CameraBuffer{
	mat4 view_proj; //view_proj = proj * view
} camera;


layout(std140,set = 0,binding = 2) readonly buffer ObjectTransforms{
	mat4 data[];
} objects;


void main()
{
	mat4 model = objects.data[gl_InstanceIndex];
	gl_Position = camera.view_proj  * model * vec4(position,1.0);
	outPosition = (model * vec4(position,1.0)).xyz;
	outNormal = mat3(transpose(inverse(model))) * normal;
	outTexCoords = texCoords;
}