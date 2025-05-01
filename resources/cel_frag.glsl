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

  // Blinn-phong color
  vec3 color = ambient + diff + spec;

  // Quantize colors

  // From silhouette
  // Take dot of n hat and eye vector, if < 0.3 (threshold) then color black, otherwise white
  if( dot(e, n) < 0.3) {
    color = vec3(0.0f, 0.0f, 0.0f);   // Black (shadow)
  } else {
    // Inspired by ChatGPT
    float step = 0.25f;
    color = floor(color/ step) * step;
  }
  gl_FragColor = vec4(color, 1.0);
}
