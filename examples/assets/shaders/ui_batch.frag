#version 300 es
precision mediump float;

uniform sampler2D u_Textures;
in vec4 vColor;
in vec2 vTexCoord;
out vec4 FragColor;

void main() {
    vec4 tex = texture(u_Textures, vTexCoord);
    //FragColor = vec4(tex.rgb * vColor.rgb, tex.a * vColor.a);
    FragColor = vColor;
}
