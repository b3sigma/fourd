//vertRedShift
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

  float savedW = 1.0 - threeSpace.w;
  threeSpace.w = 1;
  
  vec4 homogenousCoords = projectionMatrix * threeSpace;

  float zBufShift = 0.1;
  homogenousCoords.z += abs(zBufShift * savedW);
  fragHPos = homogenousCoords;
  gl_Position = fragHPos;
  
  if (savedW < 0.0) { // clip near
    fragCol0.r = 0.0;
    fragCol0.g = 0.0;
    fragCol0.b = 1.0;
    fragCol0.a = 0.0;
  } else if (savedW <= 0.33) { // blueshift
    fragCol0.r = savedW * 3;
    fragCol0.g = savedW * 3;
    fragCol0.b = 1.0;
    fragCol0.a = 1.0;
  } else if (savedW <= 0.66) { // normal
    fragCol0.r = 1.0;
    fragCol0.g = 1.0;
    fragCol0.b = 1.0;
    fragCol0.a = 1.0;
  } else if(savedW <= 1.0) { // redshift
    fragCol0.r = 1.0;
    fragCol0.g = 1.0 - (savedW * 3);
    fragCol0.b = 1.0 - (savedW * 3);
    fragCol0.a = 1.0;
  } else { // clip far
    fragCol0.r = 1.0;
    fragCol0.g = 0.0;
    fragCol0.b = 0.0;
    fragCol0.a = 0.0;
  }


}
