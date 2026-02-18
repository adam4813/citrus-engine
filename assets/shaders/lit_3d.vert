#version 300 es

// Vertex attributes
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aColor;

// Uniforms
uniform mat4 u_Model;
uniform mat4 u_MVP;
uniform mat4 u_NormalMatrix;

// Outputs to fragment shader
out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vColor;

void main() {
    // Transform position to world space
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    
    // Transform normal to world space (using normal matrix to handle non-uniform scaling)
    vNormal = mat3(u_NormalMatrix) * aNormal;
    
    // Pass UV coordinates
    vUV = aUV;
    
    // Pass vertex color
    vColor = aColor;
    
    // Transform to clip space
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
