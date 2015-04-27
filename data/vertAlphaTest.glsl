// # verasion 150
// OpenGL 3.2 apparently

uniform mat4 worldMatrix;
uniform vec4 worldPosition;
uniform vec4 cameraPosition;
uniform mat4 cameraMatrix;
uniform mat4 projectionMatrix;
uniform mat4 fourToThree;

// wNear in x, wFar in y, wFarToNearSizeRatio in z
uniform vec4 wPlaneNearFar;
// any projection enabled in x, inv proj in y, ratio proj in z
uniform vec4 wProjectionFlags;

in vec4 vertPosition;
in vec4 vertColor;

out vec4 fragHPos;
out vec4 fragCol0;

void main() {
  vec4 worldSpace = worldMatrix * vertPosition; // rotation/scale in 4d around origin
	worldSpace += worldPosition; // final 4d world space position

	vec4 cameraSpace = worldSpace - cameraPosition; // translate to be around camera origin but not transformed
	cameraSpace = cameraMatrix * cameraSpace; // final camera space position
	
	vec4 threeSpace = fourToThree * cameraSpace;
	float wSpaceFrustrumPos = (wPlaneNearFar.y - threeSpace.w) / (wPlaneNearFar.y - wPlaneNearFar.x);
	float wSpaceFrustrumPosInv = 1.0 / wSpaceFrustrumPos;
  float wFrustRatioScalar = mix(wPlaneNearFar.z, 1.0f, wSpaceFrustrumPos);
  float projectionScalar = mix(wSpaceFrustrumPos, wSpaceFrustrumPosInv, wProjectionFlags.y);
  projectionScalar = mix(projectionScalar, wFrustRatioScalar, wProjectionFlags.z);
	threeSpace.xy = mix(threeSpace.xy, threeSpace.xy * projectionScalar, wProjectionFlags.x);
  //threeSpace.z = mix(threeSpace.z, threeSpace.z * wSpaceFrustrumPos, wProjectionFlags.x);
	//threeSpace.xy = mix(threeSpace.xy, threeSpace.xy * fourProjectionScalar, wProjectionFlags.x);
  float savedW = threeSpace.w;
	threeSpace.w = 1;
	//threeSpace = cameraSpace;
	
	vec4 homogenousCoords = projectionMatrix * threeSpace; // homogenous clip space position
	//homogenousCoords.z = homogenousCoords.z * wSpaceFrustrumPos;
	//homogenousCoords.w = abs(homogenousCoords.w + savedW);
	//homogenousCoords.z = abs(homogenousCoords.z);
	fragHPos = homogenousCoords;
	//fragHPos = mul(megaMatrix, vertposition);
	//fragHPos = mul(megaMatrix, worldSpace);
	
  if (wSpaceFrustrumPos > 1.0 || wSpaceFrustrumPos < 0.0) {
    fragCol0.a = 0.0;
  } else {
    fragCol0.a = 1.0;
  }
  //fragCol0.a = 0.2;


  fragCol0.g = vertColor.y;
  //fragCol0.r = abs(worldSpace.z / 10);
  //fragCol0.b = abs(worldSpace.w / 10);
  fragCol0.r = savedW / 10;
  fragCol0.b = 0.0;
  fragCol0.b = homogenousCoords.z / 100;
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
