#version 450

//In
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;


//Out
layout(location = 0) out vec4 outColor;

//Constants

layout(set = 0,binding = 0) uniform CameraBuffer{
	mat4 view_proj; //view_proj = proj * view
	vec4 eye;
} camera;

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

layout(set = 0,binding = 4) uniform sampler2D texDiffuse;
layout(set = 0,binding = 5) uniform sampler2D texSpecular;


void main()
{
	vec3 l = lights.data[0].position.xyz;
	vec3 dx = normalize(l - inPosition);
	vec3 eye_dir = normalize(camera.eye.xyz - inPosition);
	vec3 light_bounce_dir = normalize(reflect(-dx,inNormal));


	//Params
	float ambient_factor = 0.1;
	float diffuse_factor = max(0,dot(inNormal,dx));
	float specular_factor = pow(max(dot(light_bounce_dir,eye_dir),0),64) * material.shiny.x;


	//Ambient term
	//vec3 ambient = lights.data[0].ambient.xyz * material.ambient.xyz * ambient_factor;

	//Diffuse term
	vec3 diffuse = lights.data[0].diffuse.xyz * texture(texDiffuse,inTexCoords).xyz * diffuse_factor;

	//Specular term
	vec3 specular = lights.data[0].specular.xyz * texture(texSpecular,inTexCoords).xyz * specular_factor;

	vec3 color = diffuse + specular;


	outColor = vec4(color,1.0);
}