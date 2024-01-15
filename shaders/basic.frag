#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;


// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform float fogDensity;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

//my components
float constant = 1.0f;
float linear = 0.9f;
float quadratic = 3.5f;
float shininess = 32.0f;


uniform vec3 lightPos;
uniform vec3 lightPos1;

uniform float lightOn;

vec3 pointLightColor = vec3(1.0f,0.8f,0.05f);

in vec4 fPosEye;
in vec4 lightPosEye;
in vec4 lightPosEye1;

uniform sampler2D shadowMap;
in vec4 fragPosLightSpace;

float computeShadow()
{
	// perform perspective divide
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	// Transform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;

	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;

	// Get depth of current fragment from light's perspective
	float currentDepth = normalizedCoords.z;

	// Check whether current frag pos is in shadow
	// Check whether current frag pos is in shadow
	float bias = 0.001f;
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;


	return shadow;
}

void computeDirLight()
{
    //compute eye space coordinates
    //fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}

void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
}

void computePointLight()
{
	float dist = length(lightPosEye.xyz - vec3(fPosEye));
    float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));
	
	//pozitia camerei
	vec3 cameraPosEye = vec3(0.0f);
	
	vec3 normalEye = normalize(fNormal);
	
	vec3 lightDirN = normalize(lightPosEye.xyz - vec3(fPosEye.x,fPosEye.y,fPosEye.z));
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
	
	
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);
	
	ambient += att * ambientStrength * pointLightColor;
	
	diffuse += att * max(dot(normalEye, lightDirN), 0.0f) * pointLightColor;
	
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
	specular += att * specularStrength * specCoeff * pointLightColor;	
}

void computePointLight1()
{
	float dist = length(lightPosEye1.xyz - vec3(fPosEye));
    float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));
	
	//pozitia camerei
	vec3 cameraPosEye = vec3(0.0f);
	
	vec3 normalEye = normalize(fNormal);
	
	vec3 lightDirN = normalize(lightPosEye1.xyz - vec3(fPosEye.x,fPosEye.y,fPosEye.z));
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
	
	
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);
	
	ambient += att * ambientStrength * pointLightColor;
	
	diffuse += att * max(dot(normalEye, lightDirN), 0.0f) * pointLightColor;
	
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
	specular += att * specularStrength * specCoeff * pointLightColor;	
}


float computeFog()
{

 float fragDistance = length(fPosEye);
 float fogFactor = exp(-pow(fragDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}



void main() 
{
    //computeDirLight();
	computeLightComponents();
	computePointLight();
	computePointLight1();


	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;
	
	vec3 color= vec3(0.0f);
	
	if(lightOn < 0.2f){
		color = min(ambient + diffuse + specular, 1.0f);
	}
	else {
		float shadow = computeShadow() - 1 + lightOn;
		color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);
	}

    //compute final vertex color
    //vec3 color = min(ambient + diffuse + specular, 1.0f);

	
    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    fColor = mix(fogColor, vec4(color, 1.0f), fogFactor);

}
