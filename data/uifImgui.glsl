// uifImgui

uniform sampler2D texDiffuse0;

in vec4 fragCol0;
in vec2 fragTex0;

out vec4 finalColor;

void main() {
  finalColor = fragCol0 * texture2D(texDiffuse0, fragTex0);
}