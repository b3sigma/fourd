// fragAlphaTest
#version 330

uniform sampler2D texDiffuse0;

in vec4 fragHPos;
in vec4 fragCol0;

out vec4 finalColor;

void main() {

  //finalColor = vec4(1.0f, 0.0f, 0.0f, 0.2f);
  finalColor = fragCol0;

}