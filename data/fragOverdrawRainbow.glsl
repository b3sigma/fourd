// fragOverdrawRainbow
#version 330

uniform sampler2D texDepth;

in vec4 fragHPos;
in vec4 fragCol0;
in float fragVertDepth;
in vec2 fragTex0;

out vec4 finalColor;

void main() {
  float depth = texture2D(texDepth, fragTex0).r;
  finalColor.rgb = fragCol0.rgb;
  //finalColor.rgb = fragCol0.rgb * 0.01;
  //finalColor.r += depth * 0.01;
  //finalColor.b += fragVertDepth * 0.01;
  
  //finalColor.r = abs(depth - fragVertDepth) * 10.0;
  //finalColor.b = fragVertDepth;
  //finalColor.r = depth;
  //finalColor.gb = fragTex0.xy;

  //finalColor.rgb = mix(fragCol0.rgb, texture2D(texDiffuse0, fragTex0).rgb, fragTexBlend);
  //finalColor.a = fragCol0.a;
  finalColor.a = fragCol0.a * 0.2;
  //finalColor.a = 1.0;
}