#version 300 es
precision mediump float;

// Inputs from vertex shader
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vColor;

// Output
out vec4 FragColor;

// Material properties
uniform vec4 u_Color;
uniform float u_Shininess;
uniform sampler2D u_Texture;

// Camera position
uniform vec3 u_CameraPos;

// Light properties (supporting up to 4 lights)
#define MAX_LIGHTS 4

uniform int u_NumLights;
uniform int u_LightTypes[MAX_LIGHTS];        // 0 = directional, 1 = point, 2 = spot
uniform vec3 u_LightPositions[MAX_LIGHTS];   // Position for point/spot, direction for directional
uniform vec3 u_LightColors[MAX_LIGHTS];
uniform float u_LightIntensities[MAX_LIGHTS];
uniform float u_LightRanges[MAX_LIGHTS];     // Range for point/spot lights
uniform float u_LightAttenuations[MAX_LIGHTS]; // Attenuation factor

// Ambient lighting
uniform vec3 u_AmbientColor;
uniform float u_AmbientIntensity;

// Calculate Blinn-Phong lighting for a single light
vec3 CalculateLight(
    int lightType,
    vec3 lightPos,
    vec3 lightColor,
    float lightIntensity,
    float lightRange,
    float lightAttenuation,
    vec3 normal,
    vec3 worldPos,
    vec3 viewDir,
    float shininess
) {
    vec3 lightDir;
    float attenuation = 1.0;
    
    if (lightType == 0) {
        // Directional light
        lightDir = normalize(-lightPos); // For directional, lightPos is actually direction
    } else {
        // Point or Spot light
        vec3 toLightVec = lightPos - worldPos;
        float distance = length(toLightVec);
        lightDir = normalize(toLightVec);
        
        // Attenuation based on distance
        if (distance > lightRange) {
            return vec3(0.0);
        }
        attenuation = 1.0 / (1.0 + lightAttenuation * distance * distance);
    }
    
    // Diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * lightIntensity;
    
    // Specular component (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = spec * lightColor * lightIntensity * 0.5; // Scale down specular
    
    return (diffuse + specular) * attenuation;
}

void main() {
    // Normalize interpolated normal
    vec3 normal = normalize(vNormal);
    
    // Calculate view direction
    vec3 viewDir = normalize(u_CameraPos - vWorldPos);
    
    // Sample texture
    vec4 texColor = texture(u_Texture, vUV);
    
    // Material color (combine texture, material color, and vertex color)
    vec4 baseColor = texColor * u_Color * vColor;
    
    // Ambient lighting
    vec3 ambient = u_AmbientColor * u_AmbientIntensity;
    
    // Accumulate lighting from all lights
    vec3 lighting = ambient;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (i >= u_NumLights) break;
        
        lighting += CalculateLight(
            u_LightTypes[i],
            u_LightPositions[i],
            u_LightColors[i],
            u_LightIntensities[i],
            u_LightRanges[i],
            u_LightAttenuations[i],
            normal,
            vWorldPos,
            viewDir,
            u_Shininess
        );
    }
    
    // Final color
    FragColor = vec4(baseColor.rgb * lighting, baseColor.a);
}
