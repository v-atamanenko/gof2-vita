precision lowp float;
	
varying mediump vec2 v_texCoord;
varying mediump vec3 v_light_dir;
varying mediump vec3 v_eye_dir;
varying mediump vec3 v_lightvec;
	
uniform sampler2D  s_texture_norm;
uniform sampler2D  s_texture_base;
uniform lowp vec4  glColor;
uniform lowp vec4  u_AmbientColor;
	
void main()
{
	lowp vec3 normal = normalize(texture2D(s_texture_norm, v_texCoord).rgb*2.0 - 1.0);
	
	//lowp vec3 lightvec = normalize(v_eye_dir+v_light_dir);
	lowp vec3 lightvec = normalize(v_lightvec);
	
	lowp float specularIntensity = abs( dot(lightvec, normal) );
	//lowp float diffuseIntensity  = 0.0; //max( dot(v_light_dir, normal), 0.0 );
	lowp vec4 textureColor = texture2D( s_texture_base, v_texCoord );// * v_color;

	gl_FragColor =  ( specularIntensity/* + diffuseIntensity*/) * textureColor * glColor;
}