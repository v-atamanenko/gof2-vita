precision lowp float;

varying mediump vec2 v_texCoord;
uniform sampler2D  s_texture[8];
uniform highp float u_timevalue;

void main()
{
	vec4 textureColor = texture2D(  s_texture[0], v_texCoord );
	gl_FragColor = textureColor * vec4(vec3( u_timevalue ), 1.0); // * vec4(4.0,4.0,4.0,1.0);
}