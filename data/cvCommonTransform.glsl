// cvCommonTransform

uniform mat4 worldMatrix;
uniform vec4 worldPosition;
uniform vec4 cameraPosition;
uniform mat4 cameraMatrix;
uniform mat4 fourToThree;

// wNear in x, wFar in y, wFarToNearSizeRatio in z
uniform vec4 wPlaneNearFar;
//// any projection enabled in x, inv proj in y, ratio proj in z
//uniform vec4 wProjectionFlags;

vec4 getThreeSpace(in vec4 vertPosition) {
  vec4 worldSpace = worldMatrix * vertPosition; // rotation/scale in 4d around origin
	worldSpace += worldPosition; // final 4d world space position

	vec4 cameraSpace = worldSpace - cameraPosition; // translate to be around camera origin but not transformed
	cameraSpace = cameraMatrix * cameraSpace; // final camera space position
	
	vec4 threeSpace = fourToThree * cameraSpace;
  // 0=at far plane, 1=at near plane
	float wSpaceFrustrumPos = (wPlaneNearFar.y - threeSpace.w) / (wPlaneNearFar.y - wPlaneNearFar.x);
  float wFrustRatioScalar = mix(wPlaneNearFar.z, 1.0f, wSpaceFrustrumPos);
  threeSpace.xy *= wFrustRatioScalar;
  threeSpace.w = wSpaceFrustrumPos;
  return threeSpace;
}

// takes a model where 0 is in the middle of the w-planes
vec4 getCenteredThreeSpace(in vec4 vertPosition) {
  vec4 worldSpace = worldMatrix * vertPosition; // rotation/scale in 4d around origin
	worldSpace += worldPosition; // final 4d world space position

	vec4 cameraSpace = worldSpace - cameraPosition; // translate to be around camera origin but not transformed
	cameraSpace = cameraMatrix * cameraSpace; // final camera space position
	
	vec4 threeSpace = fourToThree * cameraSpace;
  // 0=at far plane, 1=at near plane
	float wSpaceFrustrumPos = (wPlaneNearFar.y - threeSpace.w) / (wPlaneNearFar.y - wPlaneNearFar.x);
  float wFrustRatioScalar = mix(wPlaneNearFar.z, 1.0f, wSpaceFrustrumPos - 0.5);
  threeSpace.xy *= wFrustRatioScalar;
  threeSpace.w = wSpaceFrustrumPos;
  return threeSpace;
}
