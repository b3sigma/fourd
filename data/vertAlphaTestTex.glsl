// vertAlphaTestTex
#version 330

uniform mat4 projectionMatrix;

in vec4 vertPosition;
in vec2 vertCoord;
in vec4 vertColor;

out vec4 fragHPos;
out vec4 fragCol0;
out vec2 fragTex0;

vec4 getThreeSpace(vec4);

void main() {
 	vec4 threeSpace = getThreeSpace(vertPosition); 

  fragTex0.xy = vertCoord.xy;
  //fragTex0.x = sin(0.45 * vertPosition.y + -1.2 * vertPosition.x + 0.71 * vertPosition.z);
  //fragTex0.y = sin(1.4 * vertPosition.z + 0.69 * vertPosition.y + -0.34 * vertPosition.w);
  //fragTex0.x = sin(vertPosition.x);
  //fragTex0.y = sin(vertPosition.y);
  //fragTex0.x = sin(threeSpace.x + threeSpace.z);
  //fragTex0.y = sin(threeSpace.y + threeSpace.w);

  float savedW = threeSpace.w;
	threeSpace.w = 1.0;
	//threeSpace = cameraSpace;
	
	vec4 homogenousCoords = projectionMatrix * threeSpace; // homogenous clip space position
	//homogenousCoords.z = homogenousCoords.z * wSpaceFrustrumPos;
	//homogenousCoords.w = abs(homogenousCoords.w + savedW);
	//homogenousCoords.z = abs(homogenousCoords.z);
	fragHPos = homogenousCoords;
	//fragHPos = mul(megaMatrix, vertposition);
	//fragHPos = mul(megaMatrix, worldSpace);
	
  //if (wSpaceFrustrumPos > 1.0 || wSpaceFrustrumPos < 0.0) {
  if (savedW > 1.0 || savedW < 0.0) {
    fragCol0.a = 0.0;
  } else {
    fragCol0.a = 1.0;
  }
  //fragCol0.a = 0.2;


  fragCol0.g = vertColor.y;
  //fragCol0.r = abs(worldSpace.z / 10);
  //fragCol0.b = abs(worldSpace.w / 10);
  fragCol0.r = 1.0 - abs((savedW - 0.5) * 2.0);
  fragCol0.b = 1.0;
  //fragCol0.b = homogenousCoords.z / 100;
  //fragCol0.b = abs(threeSpace.x / 10);
	//fragCol0.xyz = vertcolor.xyz;
	//fragCol0.xyz = vec3(savedW, savedW, savedW);
	//fragCol0.xyz = vertposition.xyz;
	//fragCol0.xyz = worldSpace.xyz;
	//fragCol0.xyz = homogenousCoords.xyz;
	//fragCol0.xyz = fragHPos.xyz;
	fragCol0.rgb = max(fragCol0.rgb, vec3(0,0,0));
	fragCol0.rgb += vec3(0.1,0.1,0.1);
	//fragCol0.r = vertposition.w;

  gl_Position = fragHPos;
}
