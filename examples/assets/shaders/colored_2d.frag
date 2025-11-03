#version 300 es
precision mediump float;

// 2D Colored Fragment Shader
// Outputs interpolated vertex colors

in vec4 vColor;
out vec4 FragColor;

void main() {
    FragColor = vColor;
}
