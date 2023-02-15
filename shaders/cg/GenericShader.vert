attribute highp vec4 	a_position;   					
attribute mediump vec2  a_texCoord;   					
attribute highp vec3  	a_normal; 						
attribute lowp vec3 	a_tangent;							
attribute lowp vec3 	a_bitangent;						
	
varying mediump vec2 v_texCoord;
varying mediump vec3 v_light_dir;
varying mediump vec3 v_eye_dir;
varying mediump vec3 v_lightvec;

varying mediump vec3 v_normal;
varying mediump vec3 v_specular_dir;
	
uniform highp mat4 u_ModelViewProjectionMatrix;
uniform highp vec3 u_lightdirmodel;
uniform highp vec3 u_eyeposmodel;
//uniform highp mat3 u_ModelMatrix;
	
void main()
{
   	gl_Position = u_ModelViewProjectionMatrix * a_position;
   	v_texCoord  = a_texCoord;  
	v_normal = a_normal;

      //mediump vec3 bitangent = normalize(cross(a_normal, a_tangent)); 
	highp mat3 tangentSpaceXform = mat3(a_tangent.x, a_bitangent.x, a_normal.x,
										a_tangent.y, a_bitangent.y, a_normal.y,
										a_tangent.z, a_bitangent.z, a_normal.z
										);
    
   	//v_eye_dir   = tangentSpaceXform * normalize(u_eyeposmodel - a_position.xyz);
    	//v_light_dir = tangentSpaceXform * u_lightdirmodel;
    
    	vec3 shininesDirection = normalize(normalize(u_eyeposmodel - a_position.xyz) + u_lightdirmodel) ;
	v_specular_dir = tangentSpaceXform * shininesDirection;
	v_lightvec = tangentSpaceXform * u_lightdirmodel; //tangentSpaceXform * shininesDirection;
	
	//u_lightposmodel

	//v_DiffuseLight.rgb = vec3(max(dot(a_normal, u_LightDirection), 0.0));
	//v_DiffuseLight.a = 1.0;
}
