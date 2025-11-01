#version 300 es

// 2D Colored Vertex Shader
// Simple shader for rendering colored geometry with orthographic projection

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in vec4 aColor;

uniform mat4 u_MVP;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
