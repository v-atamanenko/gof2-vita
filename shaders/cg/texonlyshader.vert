attribute highp vec4 a_position;
attribute mediump vec2 a_texCoord;

varying mediump vec2 v_texCoord;

uniform highp mat4 u_WorldMatrix;




void main()
{
	gl_Position = u_WorldMatrix * a_position;
	v_texCoord = a_texCoord;
}
