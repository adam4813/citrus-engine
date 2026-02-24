#version 300 es
precision mediump float;

in vec2 vUV;
in vec4 vColor;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec4 u_Color;

void main() {
    vec4 texColor = texture(u_Texture, vUV);
    FragColor = texColor * u_Color * vColor;
}
