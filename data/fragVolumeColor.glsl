// fragVolumeColor
#version 330

uniform sampler2D texDiffuse0;
in vec4 fragHPos;
in vec4 fragCol0;
out vec4 finalColor;
void main() {
	//finalColor = vec4(1.0f, 0.0f, 0.0f, 0.2f);
	finalColor = fragCol0;
	//finalColor.rgb = vec3(0,0,1);
	finalColor.a = fragCol0.a;
}
