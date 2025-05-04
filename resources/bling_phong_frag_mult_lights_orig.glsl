#version 120

#define NUM_LIGHTS 2
uniform vec3 lightsPos[NUM_LIGHTS];
uniform vec3 lightsColor[NUM_LIGHTS];

uniform vec3 ke;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;

varying vec3 vPos;
varying vec3 vNor;
void main()
{

  // Norm intrepolated normal
  vec3 n = normalize(vNor);

  // compute the eye vector, in camera space, eye is at (0, 0, 0)
  vec3 e = normalize(-vPos);

  vec3 fragColor = ke;
  // Compute Light vectors
  for (int i = 0; i < NUM_LIGHTS; i++) {

    vec3 L = normalize(lightsPos[i] - vPos);

    // Compute diffusion using Lambert's Law
    vec3 diff = kd * max(0, dot(n, L));

    // Compute the halfway vector
    vec3 h = normalize(L + e);

    // COmputing the specular component
    vec3 spec = ks * pow(max(dot(n, h), 0), s);

    vec3 color = lightsColor[i] * (diff + spec);
    float r = length(lightsPos[i] - vPos);
    // Using 10% strength of light at r = 3,  1% at r = 10 resulting in:
    // A0 = 1.0, A1 = 0.0429, A2 = 0.9857
    float Attenuation = 1.0 / (1.0 + (0.0429 * r) + (0.9857 * r * r));

    fragColor += color * Attenuation;

  }

  gl_FragColor = vec4(fragColor, 1.0);
}
