attribute highp   vec4 	a_position;   					
attribute mediump vec2  a_texCoord;   					
attribute highp   vec3  a_normal; 						
attribute lowp    vec3 	a_tangent;							
attribute lowp    vec3 	a_bitangent;	
attribute highp    vec4  a_vertexColor;

varying mediump vec2 v_texCoord;
varying mediump vec3 v_light_dir;
varying mediump vec3 v_eye_dir;
varying mediump vec3 v_lightvec;

varying mediump vec3 v_normal;
varying mediump vec3 v_specular_dir;
varying mediump vec4 v_color;
	
uniform highp mat4 u_ModelViewProjectionMatrix;
uniform highp vec3 u_lightdirmodel;
uniform highp vec3 u_eyeposmodel;
//uniform highp mat3 u_ModelMatrix;
	
void main()
{
   	gl_Position = u_ModelViewProjectionMatrix * a_position;
   	v_color     = a_vertexColor;
   	v_texCoord  = a_texCoord;
}
