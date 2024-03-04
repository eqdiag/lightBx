#version 450

//In
layout(location = 0) in vec3 inColor;

//Out
layout(location = 0) out vec4 outColor;

void main()
{
	vec3 color = pow(inColor,vec3(1.0/2.2));
	outColor = vec4(color,1.0);
}
