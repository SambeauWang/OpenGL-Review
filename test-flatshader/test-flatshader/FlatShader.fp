#version 130

uniform vec4 vColorValue;
out vec4 vFragColor;

void main(void){
	vFragColor = vColorValue;
}