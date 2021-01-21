#version 460

#include "material_shader_common.glsl"


layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;


layout(set = MATERIAL_BLOCK_SET, binding = 0) uniform MaterialBlock {
	vec4 color;
} uMaterial;

layout(set = MATERIAL_BLOCK_SET, binding = 1) uniform sampler2D uTextures[8];


#ifdef RENDER_PASS_FORWARD
void main() {
	outColor = vec4(uMaterial.color.rgb, 1.0);

	#ifdef USE_TEXTURE
	outColor *= texture(uTextures[0], inTexCoord);
	#endif
}
#elif defined(RED)
void main() {
	outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
#elif defined(GREEN)
void main() {
	outColor = vec4(0.0, 1.0, 0.0, 1.0);
}
#elif defined(BLUE)
void main() {
	outColor = vec4(0.0, 0.0, 1.0, 1.0);
}
#else
void main() {
}
#endif