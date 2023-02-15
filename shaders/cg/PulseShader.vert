attribute highp vec4 a_position;
attribute mediump vec2 a_texCoord;

varying mediump vec2 v_texCoord;

uniform highp mat4 u_ModelViewProjectionMatrix;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * a_position;
	v_texCoord = a_texCoord;
}
