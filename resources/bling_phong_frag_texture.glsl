#version 120

uniform vec3 lightPos;
uniform sampler2D texture0; // Diffuse map
uniform sampler2D texture1; // Specular map
uniform sampler2D texture2; // Clouds map
uniform bool useCloudTexture;

varying vec3 vPos;
varying vec3 vNor;
varying vec2 vTex0;
void main()
{
  if (useCloudTexture) {
    vec4 cloudColor = texture2D(texture2, vTex0);
    // If cloudColor.r is greater than 0.5 then alpha is 1.0, otherwise 0.0.
    float alpha;
    if (cloudColor.r > 0.5f) {
      alpha = 1.0f;
    } else {
      discard;
    }
    gl_FragColor = vec4(cloudColor.rgb, alpha);
  } else {
  // Norm intrepolated normal
  vec3 n = normalize(vNor);

  // compute the eye vector, in camera space, eye is at (0, 0, 0)
  vec3 e = normalize(-vPos);

  // Compute Light vector
  vec3 L = normalize(lightPos - vPos);

  // Compute diffusion using Lambert's Law
	vec3 kd = texture2D(texture0, vTex0).rgb;
  vec3 diff = kd * max(0, dot(n, L));

  // Compute the halfway vector
  vec3 ks = texture2D(texture1, vTex0).rgb;
  vec3 h = normalize(L + e);

  // Computing the specular component
  // s = 50.0f
  vec3 spec = ks * pow(max(dot(n, h), 0), 50.0f);

  // Final color
  vec3 color = diff + spec;
  gl_FragColor = vec4(color, 1.0);
    }
}
