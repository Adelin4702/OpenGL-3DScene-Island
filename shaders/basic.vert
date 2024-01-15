#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec4 fragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform	mat3 normalMatrix;
uniform mat4 lightSpaceTrMatrix;
uniform float lightOn;

vec3 lightPos = vec3(-0.7249f, 2.845f, 2.433f);
vec3 lightPos1 = vec3(0.697f, 2.822f, 2.356f);
out vec4 fPosEye;
out vec4 lightPosEye;
out vec4 lightPosEye1;

void main() 
{
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
	fPosition = vPosition;
	fNormal = vNormal;
	fTexCoords = vTexCoords;

	vec3 lightPos = vec3(-0.7249f, 2.845f, 2.433f);
	vec3 lightPos1 = vec3(0.697f, 2.822f, 2.356f);

	fPosEye = view * model * vec4(fPosition, 1.0f);
	lightPosEye = view * model * vec4(lightPos, 1.0f);
	lightPosEye1 = view * model * vec4(lightPos1, 1.0f);
	
 	fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
}
