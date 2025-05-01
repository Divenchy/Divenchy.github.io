#version 120

uniform mat4 P;
uniform mat4 MV;

attribute vec4 aPos; // in object space
attribute vec3 aNor; // in object space

// In camera space
varying vec3 vPos;
varying vec3 vNor;

void main()
{
  // Compute camera-space 
  vec4 cameraPos = MV * aPos;
  vPos = cameraPos.xyz;


  // Transform normal into camera space
  // (Assumes MV does not contain non-uniform scaling)
  vNor = (MV * vec4(aNor, 0.0)).xyz;
    
  // Compute the final clip-space position.
  gl_Position = P * cameraPos;
}
