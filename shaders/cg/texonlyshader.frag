precision lowp float;

varying mediump vec2 v_texCoord;
uniform sampler2D s_texture_base;

/*

	if (textureColor.x > 0.5)
		textureColor.x = 1.0-(1.0-textureColor.x)*(1.0-textureColor.x);
	else textureColor.x = textureColor.x*textureColor.x;
	if (textureColor.y > 0.5)
		textureColor.y = 1.0-(1.0-textureColor.y)*(1.0-textureColor.y);
	else textureColor.y = textureColor.y*textureColor.y;
	if (textureColor.z > 0.5)
		textureColor.z = 1.0-(1.0-textureColor.z)*(1.0-textureColor.z);
	else textureColor.z = textureColor.z*textureColor.z;
*/

	//vec4 textureColor = texture2D( s_texture_base, v_texCoord ) + vec4(0.5,0.5,0.5);
	//gl_FragColor = textureColor * textureColor - vec4(0.5,0.5,0.5);


void main()
{
	vec4 textureColor = texture2D( s_texture_base, v_texCoord );
	gl_FragColor = textureColor; // * vec4(4.0,4.0,4.0,1.0);
}