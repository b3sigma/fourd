// uifCompose

uniform sampler2D texSolid;
uniform sampler2D texOverdraw;

in vec2 fragTex0;

out vec4 finalColor;

void main() {
  vec4 solid = texture2D(texSolid, fragTex0);
  vec4 overdraw = texture2D(texOverdraw, fragTex0);
  finalColor = mix(solid, overdraw, 0.2f);
  finalColor.r = 0.0f;
  finalColor.g = 1.0f;
  finalColor.b = 0.0f;
}