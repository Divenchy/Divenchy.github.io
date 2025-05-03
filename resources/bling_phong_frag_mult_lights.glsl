// #version 120
//
// // Help with ChatGPT with setting up lightPos arrays
// #define  NUM_LIGHTS 2
// uniform vec3 lightPos[NUM_LIGHTS];
// uniform vec3 lightsColor[NUM_LIGHTS];
//
// uniform sampler2D texture0;
//
// uniform vec3 ka;
// uniform vec3 kd;
// uniform vec3 ks;
// uniform float s;
//
// varying vec3 vPos;
// varying vec3 vNor;
// varying vec2 vTex;
// void main()
// {
//
//   // Get texture
//   vec3 baseColor = texture2D(texture0, vTex).rgb;
//   // Norm intrepolated normal
//   vec3 n = normalize(vNor);
//
//   // compute the eye vector, in camera space, eye is at (0, 0, 0)
//   vec3 e = normalize(-vPos);
//
//   vec3 color = baseColor;
//   // Compute Light vectors
//   for (int i = 0; i < NUM_LIGHTS; i++) {
//
//     vec3 L = normalize(lightPos[i] - vPos);
//
//     // Compute diffusion using Lambert's Law
//     vec3 diff = kd * max(0, dot(n, L));
//
//     // Compute the halfway vector
//     vec3 h = normalize(L + e);
//
//     // COmputing the specular component
//     vec3 spec = ks * pow(max(dot(n, h), 0), s);
//
//     // Color additive
//     color += lightsColor[i] * (diff + spec);
//   }
//
//   gl_FragColor = vec4(color, 1.0);
// }

// bling_phong_frag_mult_lights.glsl
#version 120
uniform sampler2D texture0;
#define NUM_LIGHTS 2
uniform vec3 lightPos[NUM_LIGHTS];
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
    vec3 L = normalize(lightPos[i] - vPos);
    vec3 h = normalize(L+e);
    color += lightsColor[i] * (
      kd * max(dot(n,L),0.0) +
      ks * pow(max(dot(n,h),0.0), s)
    ) * baseColor; // modulate by texture
  }
  gl_FragColor = vec4(color,1.0);
}

