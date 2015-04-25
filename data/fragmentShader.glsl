#version 150
// OpenGL 3.2 apparently

in vec4 fragHPos;
in vec4 fragColor;

out vec4 finalColor;

void main() {

  finalColor = fragColor;

}