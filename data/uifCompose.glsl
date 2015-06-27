// uifCompose
#version 330

uniform sampler2D texSolid;
uniform sampler2D texOverdraw;

in vec2 fragTex0;

out vec4 finalColor;

void main() {
  vec4 solid = texture2D(texSolid, fragTex0);
  vec4 overdraw = texture2D(texOverdraw, fragTex0);
  //finalColor = solid + (overdraw * 0.01);
  //finalColor = (solid * 0.01) + (overdraw * 0.9);

  // TODO: track down why we need * 1.5 for it to look normal.
  finalColor = (solid * 1.5) + (overdraw * 0.2);
  //finalColor.rgb = vec3(0.0, 1.0, 0.0);
}