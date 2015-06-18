// vertBlendNoTex

uniform mat4 projectionMatrix;

in vec4 vertPosition;
in vec4 vertColor;

out vec4 fragHPos;
out vec4 fragCol0;

vec4 getThreeSpace(vec4);

void main() {
	vec4 threeSpace = getThreeSpace(vertPosition); 
  float savedW = threeSpace.w;
	threeSpace.w = 1.0;
	//threeSpace = cameraSpace;
	
	vec4 homogenousCoords = projectionMatrix * threeSpace; // homogenous clip space position
	//homogenousCoords.w = abs(homogenousCoords.w + savedW);
	//homogenousCoords.z = abs(homogenousCoords.z);
	fragHPos = homogenousCoords;
	//fragHPos = mul(megaMatrix, vertposition);
	//fragHPos = mul(megaMatrix, worldSpace);
	
  fragCol0.a = 0.2;
  fragCol0.rgb = vertColor.xyz;
  
  //fragCol0.r = 1.0 - abs((savedW - 0.5) * 2);
  //fragCol0.b = abs(threeSpace.x / 10);
	//fragCol0.g = vertColor.y;
  //fragCol0.r = abs(worldSpace.z / 10);
  //fragCol0.b = abs(worldSpace.w / 10);
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
