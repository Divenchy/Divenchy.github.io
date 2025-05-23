// bling_phong_vert.glsl
#version 120
uniform mat4 P;
uniform mat4 MV;

// per‑vertex
attribute vec4 aPos;
attribute vec3 aNor;

// instancing
attribute vec4 aInstMat0, aInstMat1, aInstMat2, aInstMat3;

// how many repeats per world‑unit
uniform float tileScale;

varying vec3 vPos;      // eye‑space position
varying vec3 vNor;      // eye‑space normal
varying vec2 vTileUV;   // our “world‑XY” UV

void main() {
  mat4 M = mat4(aInstMat0, aInstMat1, aInstMat2, aInstMat3);

  // world‑space location of this vertex
  vec3 worldPos = (M * aPos).xyz;

  // classic Blinn‑Phong pass‑through
  vec4 camPos = MV * M * aPos;
  vPos = camPos.xyz;
  vNor = (MV * M * vec4(aNor,0.0)).xyz;

  // **project onto the XY plane** so we tile bricks across
  // width (X) and height (Y) of the wall
  vTileUV = worldPos.xy * tileScale;

  gl_Position = P * camPos;
}

