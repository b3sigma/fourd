
uniform sampler2D texDiffuse0;

in vec4 fragHPos;
in vec4 fragCol0;
in float fragTexBlend;
in vec2 fragTex0;

out vec4 finalColor;

void main() {
  finalColor.rgb = mix(fragCol0.rgb, texture2D(texDiffuse0, fragTex0).rgb, fragTexBlend);
  finalColor.a = fragCol0.a;
}