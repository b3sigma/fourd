#version 150
// OpenGL 3.2 apparently

in vec4 fragHPos;
in vec4 fragColor;

out vec4 finalColor;

void main() {

  finalColor = vec4(1.0f, 0.0f, 0.0f, 0.2f);
  finalColor += 0.01 * fragColor;

}