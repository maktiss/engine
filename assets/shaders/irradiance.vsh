#version 460
#extension GL_EXT_multiview : enable


layout(location = 0) in vec3 aPosition;


#ifdef RENDER_PASS_IRRADIANCE_MAP

#define CAMERA_SET_ID 0

#define SET_ID_OFFSET 1
#include "common.glsl"


layout(location = 0) out OutData {
	vec3 texCoord;
} outData;


layout(set = CAMERA_SET_ID, binding = 0) uniform CameraBlock {
	mat4 viewProjectionMatrices[6];
} uCamera;


void main() {
	outData.texCoord = aPosition;

	vec4 position = uCamera.viewProjectionMatrices[gl_ViewIndex] * vec4(aPosition, 0.0);

	gl_Position = position.xyzz;
}


#else
void main() {
}
#endif