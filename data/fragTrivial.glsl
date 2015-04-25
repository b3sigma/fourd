varying vec3 normal;
varying vec4 blarg;
void main()
{
	vec4 color;
  color = vec4(1.0,0.2,0.1,1.0);
  color.xyz += normal; 
	color.xyz += 0.1 * blarg.xyz;
	gl_FragColor = color;
}
