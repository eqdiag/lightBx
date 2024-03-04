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
	float constant;
	float linear;
	float quad;
	float dummy;
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

layout( push_constant ) uniform constants
{
	int num_lights;
} push_constants;


void main()
{

	vec3 color = vec3(0);
	for(int i = 0;i < push_constants.num_lights;i++){
	
		vec3 l = lights.data[i].position.xyz;
		vec3 dl = l - inPosition;
		float len = length(dl);
		vec3 dx = dl / len;
		vec3 eye_dir = normalize(camera.eye.xyz - inPosition);
		vec3 light_bounce_dir = normalize(reflect(-dx,inNormal));


		//Params
		float ambient_factor = 0.3;
		float diffuse_factor = max(0,dot(inNormal,dx));
		float specular_factor = pow(max(dot(light_bounce_dir,eye_dir),0),64) * material.shiny.x;


		//Ambient term
		vec3 ambient = lights.data[i].ambient.xyz * texture(texDiffuse,inTexCoords).xyz * ambient_factor;

		//Diffuse term
		vec3 diffuse = lights.data[i].diffuse.xyz * texture(texDiffuse,inTexCoords).xyz * diffuse_factor;

		//Specular term
		vec3 specular = lights.data[i].specular.xyz * texture(texSpecular,inTexCoords).xyz * specular_factor;

		float falloff = 1.0 / (lights.data[i].constant + lights.data[i].linear * len + lights.data[i].quad * len * len);
		//float falloff = 1.0 / ( lights.data[i].quad * len * len);


		color = color + falloff*(ambient + diffuse + specular);


	}
	
	color = pow(color,vec3(1.0/2.2));
	outColor = vec4(color,1.0);
}
