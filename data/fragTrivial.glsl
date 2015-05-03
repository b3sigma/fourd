
uniform sampler2D texDiffuse0;

in vec2 fragTex0;

varying vec3 normal;
varying vec4 blarg;
void main()
{
	vec4 color;
  color = vec4(0.1,0.2,0.1,1.0);
  color.xyz += 0.1 * normal; 
	color.xyz += 0.1 * blarg.xyz;
	//color.xy += fragTex0;
  color.xyz += texture2D(texDiffuse0, fragTex0).xyz;
  gl_FragColor = color;
}
