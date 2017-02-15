#version 330
varying vec3 vVaryingTexCoord;
uniform samplerCube cubeMap;
out vec4 vFragColor;
void main(void){
	vFragColor = texture(cubeMap, vVaryingTexCoord);
}