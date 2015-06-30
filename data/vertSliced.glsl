// vertSliced
#version 330

uniform mat4 projectionMatrix;
uniform vec4 sliceRange;

in vec4 vertPosition;
in vec2 vertCoord;
in vec4 vertColor;

out vec4 fragHPos;
out float fragTexBlend;
out vec2 fragTex0;

vec4 getThreeSpace(vec4);
vec4 getCenteredThreeSpace(vec4);

void main() {
  //vec4 threeSpace = getThreeSpace(vertPosition); 
  vec4 threeSpace = getCenteredThreeSpace(vertPosition); 

  fragTex0.xy = vertCoord.xy;

  float savedW = 1.0 - threeSpace.w;
  threeSpace.w = 1.0;
  
  vec4 homogenousCoords = projectionMatrix * threeSpace;

  //float zBufShift = 0.1;
  //homogenousCoords.z -= abs(zBufShift * savedW);
  fragHPos = homogenousCoords;
  gl_Position = fragHPos;
  
  //fragCol0.rgb = rainbow;

  if (savedW < sliceRange.x) { // clip near
    fragTexBlend = 0.0;
  } else if(savedW <= sliceRange.y) { // solid slice
    fragTexBlend = 1.0;
  } else { // clip far
    fragTexBlend = 0.0;
  }

  //if (savedW < 0.0) { // clip near
  //  fragTexBlend = 0.0;
  ////} else if (savedW <= 0.33) {
  ////  fragTexBlend = 1.0;
  ////} else if (savedW <= 0.66) {
  ////  fragTexBlend = 0.0;
  //} else if(savedW <= 1.0) { // solid slice
  //  fragTexBlend = 1.0;
  //} else { // clip far
  //  fragTexBlend = 0.0;
  //}
}
