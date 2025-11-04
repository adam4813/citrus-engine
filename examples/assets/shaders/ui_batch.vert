#version 300 es

// Vertex attributes for UI batch rendering
layout(location = 0) in vec2 aPos;          // Screen-space position (x, y)
layout(location = 1) in vec2 aTexCoord;     // Texture coordinates (u, v)
layout(location = 2) in vec4 aColor;        // Vertex color (packed as vec4)
layout(location = 3) in float aTexIndex;    // Texture slot index (0-7)

// Output to fragment shader
out vec2 vTexCoord;
out vec4 vColor;
out float vTexIndex;

// Uniforms
uniform mat4 u_Projection;  // Orthographic projection matrix

void main() {
    vTexCoord = aTexCoord;
    vColor = aColor;
    vTexIndex = aTexIndex;
    
    // Transform to clip space
    gl_Position = u_Projection * vec4(aPos, 0.0, 1.0);
}
