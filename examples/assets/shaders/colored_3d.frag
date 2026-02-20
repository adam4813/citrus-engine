#version 300 es
precision mediump float;

// 3D Colored Fragment Shader
// Lighting with vertex colors, optional base color and albedo texture from material

in vec4 vColor;
in vec3 vNormal;
in vec3 vFragPos;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 u_LightDir;
uniform vec3 u_ViewPos;
uniform vec4 u_BaseColor;
uniform sampler2D u_AlbedoMap;
uniform int u_HasAlbedoMap;

void main() {
    // Normalize the normal
    vec3 norm = normalize(vNormal);
    
    // Directional light
    vec3 lightDir = normalize(-u_LightDir);
    
    // Ambient
    float ambient = 0.3;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Specular (simple)
    vec3 viewDir = normalize(u_ViewPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    float specular = 0.3 * spec;
    
    // Surface color: albedo texture * base color * vertex color
    vec4 surface = vColor * u_BaseColor;
    if (u_HasAlbedoMap > 0) {
        surface *= texture(u_AlbedoMap, vTexCoord);
    }
    
    // Combine lighting with surface color
    vec3 result = (ambient + diff + specular) * surface.rgb;
    FragColor = vec4(result, surface.a);
}
