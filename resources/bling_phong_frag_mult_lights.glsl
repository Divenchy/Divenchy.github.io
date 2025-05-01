#version 120

// Help with ChatGPT with setting up lightPos arrays
#define  NUM_LIGHTS 2
uniform vec3 lightPos[NUM_LIGHTS];
uniform vec3 lightsColor[NUM_LIGHTS];

uniform vec3 ka;
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

  vec3 ambient = ka;

  vec3 color = ambient;
  // Compute Light vectors
  for (int i = 0; i < NUM_LIGHTS; i++) {

    vec3 L = normalize(lightPos[i] - vPos);

    // Compute diffusion using Lambert's Law
    vec3 diff = kd * max(0, dot(n, L));

    // Compute the halfway vector
    vec3 h = normalize(L + e);

    // COmputing the specular component
    vec3 spec = ks * pow(max(dot(n, h), 0), s);

    // Color additive
    color += lightsColor[i] * (diff + spec);
  }

  gl_FragColor = vec4(color, 1.0);
}
