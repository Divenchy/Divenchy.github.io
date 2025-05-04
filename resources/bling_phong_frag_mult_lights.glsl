// bling_phong_frag_mult_lights.glsl
#version 120
uniform sampler2D texture0;
#define NUM_LIGHTS 2
uniform vec3 lightsPos[NUM_LIGHTS];
uniform vec3 lightsColor[NUM_LIGHTS];
uniform vec3 ka, kd, ks;
uniform float s;

varying vec3 vPos;
varying vec3 vNor;
varying vec2 vTileUV;

void main() {
  // tile the UV (fract keeps it in 0–1)
  vec2 uv = fract(vTileUV);

  // sample your brick texture
  vec3 baseColor = texture2D(texture0, uv).rgb;

  // Blinn‑Phong on top
  vec3 n = normalize(vNor);
  vec3 e = normalize(-vPos);
  vec3 color = ka * baseColor;          // ambient
  for(int i=0;i<NUM_LIGHTS;i++){
    vec3 L = normalize(lightsPos[i] - vPos);
    vec3 h = normalize(L+e);
    color += lightsColor[i] * (
      kd * max(dot(n,L),0.0) +
      ks * pow(max(dot(n,h),0.0), s)
    ) * baseColor; // modulate by texture
  }
  gl_FragColor = vec4(color,1.0);
}

