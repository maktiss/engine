#version 460

#include "material_shader_common.glsl"


layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;


layout(set = MATERIAL_BLOCK_SET, binding = 0) uniform MaterialBlock {
	vec4 color;
} uMaterial;


void main() {
	outColor = vec4(uMaterial.color.rgb * vec3(inTexCoord.x, 0.0, inTexCoord.y), 1.0);

	outColor = vec4((vec3(1.0) - inTexCoord.xxx) * vec3(0.3, 0.8, 1.0), 1.0);
}