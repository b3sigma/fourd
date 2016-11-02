//vertVolumeColor
#version 330

uniform mat4 projectionMatrix;
in vec4 vertPosition;
in vec4 vertColor;
out vec4 fragHPos;
out vec4 fragCol0;

////////////////////
// includes from cvCommonTransform.glsl
vec4 getThreeSpace(vec4); 
float smoothClip(float hardMin, float softMin, float softMax, float hardMax, float val);
///////////////////

void main() {
	vec4 threeSpace = getThreeSpace(vertPosition);
	float savedW = threeSpace.w;
	threeSpace.w = 1.0;
	
	vec4 homogenousCoords = projectionMatrix * threeSpace;
	fragHPos = homogenousCoords;

	fragCol0.a = 0.8 * smoothClip(0.0, 0.1, 0.9, 1.0, savedW);

	fragCol0.r = mod(abs(vertPosition.x / 10.0), 2.0);
	fragCol0.g = mod(abs(vertPosition.z / 10.0), 2.0);
	fragCol0.b = mod(abs(vertPosition.w / 10.0), 2.0);
	fragCol0.r = max(fragCol0.r, 0.15);
	fragCol0.g = max(fragCol0.g, 0.05);
	fragCol0.b = max(fragCol0.b, 0.05);
	float colorNorm = sqrt(12) / length(fragCol0);
	fragCol0.rgb = fragCol0.rgb * colorNorm;
	vec3 ycol = vec3(0.3, 0.2, 0.05);
	float ycolor = mod(abs(vertPosition.y / 10.0), 2.0);
	fragCol0.rgb += ycol * ycolor;
	fragCol0.rgb = vertColor.rgb;

	gl_Position = fragHPos;
}

