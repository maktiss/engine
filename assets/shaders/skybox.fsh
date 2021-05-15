#version 460


#ifdef RENDER_PASS_SKYBOX

#define CAMERA_SET_ID 0

#define SET_ID_OFFSET 1
#include "common.glsl"


layout(location = 0) in InData {
	vec3 texCoord;
}
inData;


layout(location = 0) out vec4 outColor;


layout(set = INPUT_TEXTURES_SET_ID, binding = 0) uniform samplerCube uCubeMap;


void main() {
	// outColor = vec4(0.30, 0.33, 0.34, 1.0);
	outColor = vec4(texture(uCubeMap, inData.texCoord).rgb, 1.0);
	// outColor = vec4(0.70, 0.65, 0.50, 1.0);
	// outColor = vec4(0.70, 0.0, 0.0, 1.0);
	// outColor = vec4(inData.texCoord, 1.0);
}


#else
void main() {
}
#endif