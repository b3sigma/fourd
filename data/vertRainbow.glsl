// vertRainbow
#version 330

uniform mat4 projectionMatrix;

in vec4 vertPosition;
in vec2 vertCoord;
in vec4 vertColor;

out vec4 fragHPos;
out vec4 fragCol0;
out float fragTexBlend;
out vec2 fragTex0;

////////////////////
// includes from cvCommonTransform.glsl
vec4 getThreeSpace(vec4); 
float smoothClip(float hardMin, float softMin, float softMax, float hardMax, float val);
///////////////////

void main() {
	vec4 threeSpace = getThreeSpace(vertPosition); 

	fragTex0.xy = vertCoord.xy;

	float savedW = 1.0 - threeSpace.w;
	threeSpace.w = 1.0;

	vec3 rainbow;
	rainbow.r = mod(abs(vertPosition.x / 10.0), 2.0);
	rainbow.g = mod(abs(vertPosition.z / 10.0), 2.0);
	rainbow.b = mod(abs(vertPosition.w / 10.0), 2.0);
  
	vec4 homogenousCoords = projectionMatrix * threeSpace;

	float zBufShift = 0.1;
	homogenousCoords.z -= abs(zBufShift * savedW);
	fragHPos = homogenousCoords;
	gl_Position = fragHPos;
  
	fragCol0.rgb = rainbow;

	float smoo = 0.1;
	float near = 0.0;
	float solidnear = 0.33; 
	float rainbowfar = 1.0;
	float rainbowsmoo = 0.1;
	fragTexBlend = smoothClip(-smoo, near, solidnear - smoo, solidnear + smoo, savedW);
	fragCol0.a = smoothClip(-smoo, near, solidnear - smoo, solidnear + smoo, savedW);
			   + 0.2 * smoothClip(solidnear - smoo, solidnear + smoo, 
								  rainbowfar, rainbowfar + rainbowsmoo,
								  savedW);
    
	//if (savedW < 0.0) { // clip near
	//	fragCol0.a = 0.0;
	//	fragTexBlend = 0.0;
	//} else if (savedW <= 0.33) {
	//	fragCol0.a = 1.0;
	//	fragTexBlend = 1.0;
	//} else if(savedW <= 1.0) { // rainbow
	//	fragCol0.a = 0.2;
	//	fragTexBlend = 0.0;
	//} else { // clip far
	//	fragCol0.a = 0.0;
	//	fragTexBlend = 0.0;
	//}
}
