
uniform sampler2D texDiffuse0;

in vec4 fragHPos;
in vec4 fragCol0;
in vec2 fragTex0;

out vec4 finalColor;

void main() {

  //finalColor = vec4(1.0f, 0.0f, 0.0f, 0.2f);
  finalColor = fragCol0 * 0.1;
  finalColor += texture2D(texDiffuse0, fragTex0);
  //finalColor.r = 0.5 + (fragCol0.r * 0.1);
  finalColor.a = fragCol0.a;
}