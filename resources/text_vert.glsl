// text_vert.glsl
#version 120
attribute vec4 aPos;    // <vec2 pos, vec2 uv>
attribute vec2 aTex;
uniform mat4 projection;
varying vec2 TexCoords;
void main() {
    gl_Position = projection * vec4(aPos.xy, 0.0, 1.0);
    TexCoords = aPos.zw;
}

