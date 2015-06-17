
uniform sampler2D texOverdraw;

in vec2 fragTex0;

out vec4 finalColor;

void main() {
  finalColor.rgb = texture2D(texOverdraw, fragTex0).rgb;
  finalColor.a = 0.2;
}