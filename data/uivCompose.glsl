// uivCompose
#version 330

in vec2 vertPosition;
//in vec2 vertCoord;

out vec2 fragTex0;

void main() {
  // vertPosition is from -1 to 1
  fragTex0.xy = vertPosition * vec2(0.5, 0.5) + vec2(0.5, 0.5);
  gl_Position = vec4(vertPosition.xy, 0.0f, 1.0f);
}
