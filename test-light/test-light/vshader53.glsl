#version 150 


in  vec4 vPosition;
in  vec3 vNormal;
out vec4 color;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition;
uniform float Shininess;
void main(){
    //vec3 pos = (ModelView * vPosition).xyz;
    vec3 pos = (ModelView * vPosition).xyz;

    vec3 N = normalize(ModelView * vec4(vNormal,0.0)).xyz;
    vec3 L = normalize( (ModelView * LightPosition).xyz - pos);
    vec3 E = normalize(-pos);
    vec3 h = normalize(E + L);

    vec4 ambient = AmbientProduct;
    float d = max(dot(L,N),0.0);
    vec4 diffuse = DiffuseProduct * d;

    float Sd = pow(max(dot(h,N),0.0),Shininess);
    vec4 specular = SpecularProduct * Sd;
    if (dot(L, N) < 0.0){
        specular = vec4(0.0,0.0,0.0,1.0);
    }

    gl_Position = Projection * ModelView * vPosition;
    color = ambient + diffuse + specular;
    color.a = 1.0;
}
