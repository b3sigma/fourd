#version 150
// OpenGL 3.2 apparently

uniform mat4 worldMatrix;
uniform vec4 worldPosition;
uniform vec4 cameraPosition;
uniform mat4 cameraMatrix;
uniform mat4 projectionMatrix;
uniform mat4 fourToThree;
uniform vec4 wPlaneNearFar;

in vec4 vertPosition;
in vec4 vertColor;

out vec4 fragHPos;
out vec4 fragCol0;

void main() {
	//vec4 worldSpace = mul(worldMatrix, vec4(vertposition.xyz, 1.0)); // rotation/scale in 4d around origin
  vec4 worldSpace = worldMatrix * vertPosition; // rotation/scale in 4d around origin
	worldSpace += worldPosition; // final world space position

  //	fragTex0.x = vertposition.x + vertposition.z;
  //	fragTex0.y = vertposition.y + vertposition.w;
	
	vec4 cameraSpace = worldSpace - cameraPosition; // translate to be around camera origin but not transformed
	cameraSpace = cameraMatrix * cameraSpace; // final camera space position
	
	vec4 threeSpace = fourToThree * cameraSpace;
	float fourProjectionScalar = (wPlaneNearFar.y - threeSpace.w) / (wPlaneNearFar.y - wPlaneNearFar.x);
  // TODO: (make a smoother depth calc that doesn't go through a zero singularity)
	//fourProjectionScalar = clamp(fourProjectionScalar, 0.0, 1.0);
	threeSpace.xy = mix(threeSpace.xy, threeSpace.xy * fourProjectionScalar, wPlaneNearFar.z);
	//threeSpace.xy *= clamp(threeSpace.w, 0.5, 1.0);
  //threeSpace.xy *= threeSpace.w;
  //threeSpace.xyz *= vertposition.w; // clamp(vertposition.w, 0.5, 1.0);
  float savedW = threeSpace.w;
	threeSpace.w = 1;
	//threeSpace = cameraSpace;
	
	vec4 homogenousCoords = projectionMatrix * threeSpace; // homogenous clip space position
	//homogenousCoords.w = abs(homogenousCoords.w + savedW);
	//homogenousCoords.z = abs(homogenousCoords.z);
	fragHPos = homogenousCoords;
	//fragHPos = mul(megaMatrix, vertposition);
	//fragHPos = mul(megaMatrix, worldSpace);
	
  fragCol0.a = 0.2;
  fragCol0.g = vertColor.y;
  //fragCol0.r = abs(worldSpace.z / 10);
  //fragCol0.b = abs(worldSpace.w / 10);
  fragCol0.r = abs(savedW / 10);
  fragCol0.b = abs(threeSpace.x / 10);
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
