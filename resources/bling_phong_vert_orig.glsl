#version 120

uniform mat4 P;
uniform mat4 MV;

attribute vec4 aPos; // in object space
attribute vec3 aNor; // in object space


attribute vec4 aInstMat0;
attribute vec4 aInstMat1;
attribute vec4 aInstMat2;
attribute vec4 aInstMat3;

// In camera space
varying vec3 vPos;
varying vec3 vNor;

void main()
{
  mat4 M = mat4(
      aInstMat0,
      aInstMat1,
      aInstMat2,
      aInstMat3
      );

  vec4 cameraPos = MV * M * aPos;
  vPos = cameraPos.xyz;
  vNor = (MV * M * vec4(aNor, 0.0)).xyz;
  gl_Position = P * cameraPos;
}
