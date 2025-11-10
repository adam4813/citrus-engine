#version 300 es
precision mediump float;

// Input from vertex shader
in vec2 vTexCoord;
in vec4 vColor;
in float vTexIndex;

// Output color
out vec4 FragColor;

// Texture samplers (up to 8 texture units)
uniform sampler2D u_Textures[8];

void main() {
    // Sample from the correct texture based on texture index
    vec4 texColor;
    int texIdx = int(vTexIndex + 0.5);// Round to nearest int

    // Manual texture selection (GLSL ES doesn't support dynamic indexing easily)
    if (texIdx == 0) {
        texColor = texture(u_Textures[0], vTexCoord);
    } else if (texIdx == 1) {
        texColor = texture(u_Textures[1], vTexCoord);
    } else if (texIdx == 2) {
        texColor = texture(u_Textures[2], vTexCoord);
    } else if (texIdx == 3) {
        texColor = texture(u_Textures[3], vTexCoord);
    } else if (texIdx == 4) {
        texColor = texture(u_Textures[4], vTexCoord);
    } else if (texIdx == 5) {
        texColor = texture(u_Textures[5], vTexCoord);
    } else if (texIdx == 6) {
        texColor = texture(u_Textures[6], vTexCoord);
    } else {
        texColor = texture(u_Textures[7], vTexCoord);
    }

    // Multiply texture by vertex color (standard)
    FragColor = texColor * vColor;
}
