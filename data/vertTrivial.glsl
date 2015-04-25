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

void main()
{	
  blarg = cameraPosition * worldMatrix * cameraMatrix * projectionMatrix * fourToThree;
	blarg += wPlaneNearFar;
  normal = gl_Normal;
	gl_Position = ftransform() + worldPosition;
}
