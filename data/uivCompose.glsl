// uivCompose

in vec2 vertPosition;
in vec2 vertCoord;

out vec2 fragTex0;

void main() {
  fragTex0.xy = vertCoord.xy;
  gl_Position = vec4(vertPosition.xy, 0.0f, 1.0f);
}
