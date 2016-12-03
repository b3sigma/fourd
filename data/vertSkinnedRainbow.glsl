//vertColorBlendClipped
#version 330

uniform mat4 projectionMatrix;
uniform mat4 boneRotations[20];
uniform vec4 bonePositions[20];

in vec4 vertPosition;
in vec4 vertColor;
in int vertBoneIndex;

out vec4 fragHPos;
out vec4 fragCol0;

////////////////////
// includes from cvCommonTransform.glsl
vec4 getThreeSpace(vec4); 
float smoothClip(float hardMin, float softMin, float softMax, float hardMax, float val);
///////////////////

vec4 getObjectSpace(in vec4 vertPosition, in int vertBoneIndex) {
  vec4 objectSpace = boneRotations[vertBoneIndex] * vertPosition; // rotation/scale in 4d around origin
  objectSpace += bonePositions[vertBoneIndex]; // final 4d world space position
  return objectSpace;
}

void main() {
	vec4 objectSpace = getObjectSpace(vertPosition, vertBoneIndex);
	vec4 threeSpace = getThreeSpace(objectSpace); 
	float savedW = threeSpace.w;
	threeSpace.w = 1.0;
	
	vec4 homogenousCoords = projectionMatrix * threeSpace; // homogenous clip space position
	fragHPos = homogenousCoords;

	fragCol0.a = 0.2 * smoothClip(0.0, 0.1, 0.9, 1.0, savedW);
	fragCol0.r = mod(abs(vertPosition.x / 10.0), 2.0); //, 0.1);
	fragCol0.g = mod(abs(vertPosition.z / 10.0), 2.0); //, 0.1);
	fragCol0.b = mod(abs(vertPosition.w / 10.0), 2.0); //, 0.1);

	fragCol0.r = max(fragCol0.r, 0.15);
	fragCol0.g = max(fragCol0.g, 0.05);
	fragCol0.b = max(fragCol0.b, 0.05);

	gl_Position = fragHPos;
}
