#version 460

#include "material_shader_common.glsl"


layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;


layout(binding = MATERIAL_BLOCK_LOCATION) uniform MaterialBlock {
	vec4 color;
} uMaterial;


void main() {
	outColor = vec4(uMaterial.color.rgb, 1.0);
}