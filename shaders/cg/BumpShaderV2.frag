precision lowp float;
	
varying mediump vec2 v_texCoord;
varying mediump vec3 v_light_dir;
varying mediump vec3 v_eye_dir;
varying mediump vec3 v_lightvec;
	
varying mediump vec3 v_normal;
varying mediump vec3 v_specular_dir;

uniform sampler2D  s_texture[8];
uniform lowp vec4  u_AmbientColor;
//uniform lowp vec4  u_DiffuseColor;
//uniform lowp vec4  u_SpecularColor;
uniform lowp vec4  glColor;	

/*
void main()
{
	vec4 textureColor = texture2D( s_texture[0], v_texCoord );
	gl_FragColor = textureColor; // * vec4(4.0,4.0,4.0,1.0);
}
*/

void main()
{
	lowp vec4 colorTex   = texture2D( s_texture[0], v_texCoord, -1.0 );
	lowp vec4 normalTex  = texture2D( s_texture[1], v_texCoord, -1.0 );
	lowp vec3 normalTexX = normalTex.rgb*2.0 - 1.0;
	lowp vec4 specTex = vec4(vec3(normalTex.a), 0.0); //texture2D(s_texture[1], v_texCoord);


	float specularIntensity = pow( dot( v_specular_dir,normalTexX ), 10.0 );
	float diffuseIntensity  = max( dot( normalTexX    ,v_lightvec ), 0.0 ) + 0.26;


	
	
	//float r = colorTex.r;
	//colorTex.r = colorTex.b * 2.0;
	//colorTex.b = r * 2.0;
	//colorTex.g = 0.0;
	
	//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	gl_FragColor = diffuseIntensity * colorTex * u_AmbientColor + specularIntensity * specTex;
	
}
//*/
