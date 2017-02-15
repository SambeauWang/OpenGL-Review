#version 130

smooth in vec3 vVaryingNormal;
smooth in vec3 vVaryingLightDir;
smooth in vec2 vTexCoords;

out vec4 vFragColor;

uniform vec4    ambientColor;
uniform vec4    diffuseColor;   
uniform vec4    specularColor;
uniform sampler2D colorMap;
void main(void){
	float diff = max(0.0, dot(normalize(vVaryingNormal), normalize(vVaryingLightDir)) );
	vFragColor = diff * diffuseColor;

	vFragColor += ambientColor;

	vFragColor *= texture(colorMap ,vTexCoords);
	
	vec3 mreflect = normalize( reflect(-normalize(vVaryingLightDir),normalize(vVaryingNormal) ) );
	//vec3 vReflection = normalize(reflect(-normalize(vVaryingLightDir), normalize(vVaryingNormal)));
	//float spec = max(0.0, dot(normalize(vVaryingNormal), vReflection));
	float spec = max(0.0, dot(mreflect, normalize(vVaryingNormal)));
	if (diff != 0){
		float fspec = pow(spec, 128.0);
		vFragColor.rgb += vec3(fspec, fspec, fspec);
	}
}