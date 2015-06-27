// fragRedShift
#version 330

uniform sampler2D texDiffuse0;

in vec4 fragHPos;
in vec4 fragCol0;
in vec2 fragTex0;

out vec4 finalColor;

void main() {
  finalColor.rgb = texture2D(texDiffuse0, fragTex0).rgb * fragCol0.rgb;
  finalColor.a = fragCol0.a;
}