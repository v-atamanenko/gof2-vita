attribute highp vec4 	a_position;   					

	
uniform highp mat4 u_ModelViewProjectionMatrix;

	
void main()
{
   	gl_Position = u_ModelViewProjectionMatrix * a_position;
}
