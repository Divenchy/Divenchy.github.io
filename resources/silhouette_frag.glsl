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

  // Calculate color
  vec3 color = vec3(1.0f, 1.0f, 1.0f);

  // Take dot of n hat and eye vector, if < 0.3 (threshold) then color black, otherwise white
  if( dot(e, n) < 0.3) {
    color = vec3(0.0f, 0.0f, 0.0f); 
  }

  gl_FragColor = vec4(color, 1.0);
}
