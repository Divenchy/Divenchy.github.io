// text_frag.glsl
#version 120
uniform sampler2D text;
uniform vec3 textColor;
varying vec2 TexCoords;
void main() {
    float alpha = texture2D(text, TexCoords).r;
    gl_FragColor = vec4(textColor, alpha);
}

