#version 300 es

// 2D Colored Vertex Shader
// Simple shader for rendering colored geometry with orthographic projection

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 u_MVP;

out vec4 vColor;
out vec2 vTexCoord;

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
