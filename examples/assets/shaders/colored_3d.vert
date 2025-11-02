#version 300 es

// 3D Colored Vertex Shader
// Simple shader for rendering colored 3D geometry with perspective projection

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 u_MVP;
uniform mat4 u_Model;

out vec4 vColor;
out vec3 vNormal;
out vec3 vFragPos;
out vec2 vTexCoord;

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    vNormal = mat3(u_Model) * aNormal;
    vFragPos = vec3(u_Model * vec4(aPos, 1.0));
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
