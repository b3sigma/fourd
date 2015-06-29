// uifCompose
#version 330

uniform sampler2D texSolid;
uniform sampler2D texOverdraw;

in vec2 fragTex0;

out vec4 finalColor;

void main() {
  // TODO: track down why we need * 2.0 for it to look normal.
  vec4 solid = texture2D(texSolid, fragTex0);
  
  vec4 overdraw = texture2D(texOverdraw, fragTex0);
  //finalColor = solid + (overdraw * 0.01);
  //finalColor = (solid * 0.01) + (overdraw * 0.9);

  vec3 colorDot = vec3(0.4, 0.6, 0.3);
  float overdrawAlpha = dot(overdraw.rgb, colorDot);
  float solidAlpha = dot(solid.rgb, colorDot);
  //finalColor = (solid * 0.01) + (overdraw * 0.99);
  //finalColor = (solid * 0.99) + (overdraw * 0.01);
  //finalColor = (solid) + (overdraw);
  //finalColor = mix(solid, overdraw, overdrawAlpha);
  vec4 blendySolid = solid + (overdraw * 0.5);
  finalColor = mix(overdraw, blendySolid, solid.a);
  
  //finalColor.rgb = vec3(0.0, 1.0, 0.0);
}