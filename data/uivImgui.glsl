
// uivImgui

uniform mat4 projectionMatrix;

in vec2 vertPosition;
in vec2 vertCoord;
in vec4 vertColor;

out vec4 fragCol0;
out vec2 fragTex0;

void main() {
  fragTex0.xy = vertCoord.xy;
  fragCol0 = vertColor;
  gl_Position = projectionMatrix * vec4(vertPosition.xy, 0.0f, 1.0f);
}
