
uniform mat4 projectionMatrix;

in vec2 vertPosition;
in vec2 vertCoord;
in vec4 vertColor;

out vec4 fragCol0;
out vec2 fragTex0;

//vec4 getThreeSpace(vec4);

void main() {

  fragTex0.xy = vertCoord.xy;
  fragCol0 = vertColor;
  gl_Position = projectionMatrix * vec4(vertPosition.xy, 0, 1);

 //	vec4 threeSpace = getThreeSpace(vertPosition); 
 // float savedW = threeSpace.w;
	//threeSpace.w = 1;
	
	//vec4 homogenousCoords = projectionMatrix * threeSpace;
	//fragHPos = homogenousCoords;
	
 // if (savedW > 1.0 || savedW < 0.0) {
 //   fragCol0.a = 0.0;
 // } else {
 //   fragCol0.a = 1.0;
 // }


 // fragCol0.g = vertColor.y;
 // fragCol0.r = 1.0 - abs((savedW - 0.5) * 2);
 // fragCol0.b = 1.0;
	//fragCol0.rgb = max(fragCol0.rgb, vec3(0,0,0));
	//fragCol0.rgb += vec3(0.1,0.1,0.1);

 // gl_Position = fragHPos;
}
