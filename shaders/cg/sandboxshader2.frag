precision lowp float;
	
varying mediump vec2 v_texCoord;
varying lowp vec4  v_DiffuseLight;
varying mediump vec3 v_lightvec;
	
uniform sampler2D s_texture_norm;
uniform sampler2D s_texture_base;
uniform lowp vec4 glColor;
uniform lowp vec4 u_AmbientColor;
uniform highp vec3 u_LightDirection;
	
void main()
{
	lowp vec3 normal = normalize(texture2D(s_texture_norm, v_texCoord).rgb*2.0 - 1.0);
	lowp vec3 lightvec = normalize(v_lightvec);

	//mediump float lightIntensity = max(dot(lightvec, normal), 0.0);
	lowp float lightIntensity = pow(dot(lightvec, normal), 20.0 );
//	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0) * lightIntensity;
//	}
	lowp vec4 textureColor = texture2D( s_texture_base, v_texCoord );// * v_color;

	lowp vec4 specular = vec4(vec3(lightIntensity * 0.7), 1.0);
	lowp vec4 ambient = 0.3 * textureColor;
	gl_FragColor = specular + ambient;
}