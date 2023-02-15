attribute highp vec4 	a_position;   					
attribute mediump vec2  a_texCoord;   					
attribute highp vec3  	a_normal; 						
attribute lowp vec3 	a_tangent;							
attribute lowp vec3 	a_bitangent;						
	
varying mediump vec2 v_texCoord;
varying mediump vec3 v_light_dir;
varying mediump vec3 v_eye_dir;
varying mediump vec3 v_lightvec;
	
uniform highp mat4 u_WorldMatrix;
uniform highp vec3 u_light_dir;
uniform highp vec3 u_eye_pos;
	
void main()
{
   	gl_Position = u_WorldMatrix * a_position;		
   	v_texCoord  = a_texCoord;  

    //mediump vec3 bitangent = normalize(cross(a_normal, a_tangent)); 
	highp mat3 tangentSpaceXform = mat3(a_tangent, a_bitangent, a_normal);
    
   	//v_eye_dir   = tangentSpaceXform * normalize(u_eye_pos - a_position.xyz);
    //v_light_dir = tangentSpaceXform * u_light_dir;
    
    vec3 shininesDirection = normalize(normalize(u_eye_pos - a_position.xyz) + u_light_dir) ;
	v_lightvec = tangentSpaceXform * shininesDirection;
	
	//u_lightposmodel

	//v_DiffuseLight.rgb = vec3(max(dot(a_normal, u_LightDirection), 0.0));
	//v_DiffuseLight.a = 1.0;
}
