#version 120

uniform vec3 lightPos;
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

  // Compute Light vector
  vec3 L = normalize(lightPos - vPos);

  // Compute diffusion using Lambert's Law
  vec3 diff = kd * max(0, dot(n, L));

  // Compute the halfway vector
  vec3 h = normalize(L + e);

  // Computing the specular component
  vec3 spec = ks * pow(max(dot(n, h), 0), s);

  // Final color
  vec3 color = ambient + diff + spec;
  color = 1.0f - color;
  gl_FragColor = vec4(color, 1.0);
}
