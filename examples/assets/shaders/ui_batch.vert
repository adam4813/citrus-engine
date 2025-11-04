#version 300 es

// 2D Colored Vertex Shader
// Simple shader for rendering colored geometry with orthographic projection

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in uint aColor;
layout(location = 3) in float textureIndex;

uniform mat4 uProjection;
out vec4 vColor;
out vec2 vTexCoord;

void main() {
	// Unpack color from uint to vec4
	vColor =
			vec4(float((aColor >> 24) & 0xFFu) / 255.0,
				 float((aColor >> 16) & 0xFFu) / 255.0,
				 float((aColor >> 8) & 0xFFu) / 255.0,
				 float(aColor & 0xFFu) / 255.0);
	vTexCoord = aTexCoord;
	gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
