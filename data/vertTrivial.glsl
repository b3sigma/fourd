// vertTrivial
#version 330

varying vec3 normal;
varying vec4 blarg;
uniform vec4 worldPosition;

uniform mat4 worldMatrix;
//uniform vec4 worldPosition;
uniform vec4 cameraPosition;
uniform mat4 cameraMatrix;
uniform mat4 projectionMatrix;
uniform mat4 fourToThree;
uniform vec4 wPlaneNearFar;

in vec4 vertPosition;
in vec4 vertColor;

out vec2 fragTex0;

void main()
{	
  blarg = cameraPosition * worldMatrix * cameraMatrix * projectionMatrix * fourToThree;
	blarg += wPlaneNearFar;

  //fragTex0.xy = gl_MultiTexCoord0.xy;
  normal = gl_Normal;
	gl_Position = ftransform() + worldPosition;
  fragTex0.x = abs(sin(gl_Position.x));
  fragTex0.y = abs(sin(gl_Position.y));
}
