#version 460


#ifdef RENDER_PASS_BOX_BLUR

#define SET_ID_OFFSET 0
#include "common.glsl"


layout(location = 0) in InData {
	vec2 texCoord;
}
inData;


layout(location = 0) out vec4 outColor;


layout(constant_id = 0) const bool DIRECTION = false;
layout(constant_id = 1) const uint KERNEL_SIZE = 5;


layout(push_constant) uniform Params {
	uint layer;
}
uParams;


layout(set = INPUT_TEXTURES_SET_ID, binding = 0) uniform sampler2DArray uInput;


void main() {
	vec4 color = vec4(0.0);

	if (DIRECTION) {
		if (KERNEL_SIZE >= 9) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, -4));
		if (KERNEL_SIZE >= 7) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, -3));
		if (KERNEL_SIZE >= 5) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, -2));
		if (KERNEL_SIZE >= 3) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, -1));
		if (KERNEL_SIZE >= 1) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, +0));
		if (KERNEL_SIZE >= 3) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, +1));
		if (KERNEL_SIZE >= 5) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, +2));
		if (KERNEL_SIZE >= 7) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, +3));
		if (KERNEL_SIZE >= 9) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(0, +4));

	} else {
		if (KERNEL_SIZE >= 9) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(-4, 0));
		if (KERNEL_SIZE >= 7) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(-3, 0));
		if (KERNEL_SIZE >= 5) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(-2, 0));
		if (KERNEL_SIZE >= 3) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(-1, 0));
		if (KERNEL_SIZE >= 1) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(+0, 0));
		if (KERNEL_SIZE >= 3) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(+1, 0));
		if (KERNEL_SIZE >= 5) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(+2, 0));
		if (KERNEL_SIZE >= 7) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(+3, 0));
		if (KERNEL_SIZE >= 9) color += textureOffset(uInput, vec3(inData.texCoord, uParams.layer), ivec2(+4, 0));
	}

	color /= KERNEL_SIZE;

	outColor = color;
}


#else
void main() {
}
#endif