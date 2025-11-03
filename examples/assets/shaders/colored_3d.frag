#version 300 es
precision mediump float;

// 3D Colored Fragment Shader
// Simple lighting calculation with vertex colors

in vec4 vColor;
in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec3 u_LightDir;
uniform vec3 u_ViewPos;

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
    
    // Combine
    vec3 result = (ambient + diff + specular) * vColor.rgb;
    FragColor = vec4(result, vColor.a);
}
