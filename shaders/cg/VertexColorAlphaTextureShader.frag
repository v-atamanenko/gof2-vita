precision lowp float;
	
varying mediump vec2 v_texCoord;
varying mediump vec3 v_light_dir;
varying mediump vec3 v_eye_dir;
varying mediump vec3 v_lightvec;
	
varying mediump vec3 v_normal;
varying mediump vec3 v_specular_dir;
varying highp    vec4 v_color;

uniform sampler2D  s_texture[8];
//uniform lowp vec4  u_AmbientColor;
//uniform lowp vec4  u_DiffuseColor;
//uniform lowp vec4  u_SpecularColor;
uniform highp vec4  glColor;	

void main()
{
	lowp vec4 colorTex   = texture2D( s_texture[0], v_texCoord );
	if ( colorTex.a < 0.5)
		discard;
	gl_FragColor = colorTex * v_color; // * glColor;  (Color scheint nicht benštigt zu werden)
}