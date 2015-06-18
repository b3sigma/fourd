// vertOverdrawRainbow

uniform mat4 projectionMatrix;

in vec4 vertPosition;
//in vec2 vertCoord;
in vec4 vertColor;

out vec4 fragHPos;
out vec4 fragCol0;
out float fragVertDepth;
out vec2 fragTex0;

vec4 getThreeSpace(vec4);

void main() {
  vec4 threeSpace = getThreeSpace(vertPosition); 
  
  float savedW = 1.0 - threeSpace.w;
  threeSpace.w = 1.0;

  //fragTex0.xy = vertCoord.xy;

  vec3 rainbow;
  rainbow.r = mod(abs(vertPosition.x / 10.0), 2.0);
	rainbow.g = mod(abs(vertPosition.z / 10.0), 2.0);
	rainbow.b = mod(abs(vertPosition.w / 10.0), 2.0);
  
  vec4 homogenousCoords = projectionMatrix * threeSpace;

  //float zBufShift = 0.1;
  //homogenousCoords.z -= abs(zBufShift * savedW);
  fragHPos = homogenousCoords;
  gl_Position = fragHPos;
  fragVertDepth = fragHPos.z / fragHPos.w;
  //fragTex0.xy = (fragHPos.xy * 0.5) + 0.5;
  fragTex0.xy = ((fragHPos.xy / max(fragHPos.w, 0.001)) * 0.5 + 0.5);
  fragTex0.x = clamp(fragTex0.x, 0.0, 1.0);
  fragTex0.y = clamp(fragTex0.y, 0.0, 1.0);

  fragCol0.rgb = rainbow;
    
  if (savedW < 0.0) { // clip near
    fragCol0.a = 0.0;
  //} else if (savedW <= 0.33) {
  //  fragCol0.a = 1.0;
  //} else if (savedW <= 0.66) {
  //  fragCol0.a = 0.0;
  } else if(savedW <= 1.0) { // rainbow
    fragCol0.a = 1.0;
  } else { // clip far
    fragCol0.a = 0.0;
  }

  //fragCol0.r = depthTexel.r;
}
