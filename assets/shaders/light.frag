#version 450

//In
layout(location = 0) in vec3 inColor;

//Out
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(inColor,1.0);
}