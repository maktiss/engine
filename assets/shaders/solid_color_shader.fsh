#version 460

#include "material_shader_common.glsl"


layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;


layout(set = MATERIAL_BLOCK_SET, binding = 0) uniform MaterialBlock {
	vec4 color;
} uMaterial;

layout(set = MATERIAL_BLOCK_SET, binding = 1) uniform sampler2D uTextures[8];


void main() {
	outColor = vec4(uMaterial.color.rgb, 1.0);

	#ifdef USE_TEXTURE
	outColor = texture(uTextures[0], inTexCoord);
	#endif

	// outColor = vec4((vec3(1.0) - inTexCoord.xxx) * vec3(0.3, 0.8, 1.0), 1.0);
}