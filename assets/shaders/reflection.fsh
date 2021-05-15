#version 460


#ifdef RENDER_PASS_REFLECTION

#define CAMERA_SET_ID 0

#define SET_ID_OFFSET 1
#include "common.glsl"


layout(location = 0) in InData {
	vec2 texCoord;
}
inData;


layout(location = 0) out vec4 outColor;


layout(set = CAMERA_SET_ID, binding = 0) uniform Camera {
	mat4 invViewMatrix;
	mat4 invProjectionMatrix;
} uCamera;

layout(set = INPUT_TEXTURES_SET_ID, binding = 0) uniform sampler2D uDepthBuffer;
layout(set = INPUT_TEXTURES_SET_ID, binding = 1) uniform sampler2D uNormalBuffer;
layout(set = INPUT_TEXTURES_SET_ID, binding = 2) uniform samplerCube uEnvironmentBuffer;


void main() {
	vec2 screenPos = inData.texCoord * 2.0 - vec2(1.0);
	screenPos.y *= -1.0;

	float depth = texture(uDepthBuffer, inData.texCoord).x;

	vec4 clipSpacePos = vec4(screenPos, depth, 1.0);

	vec4 viewSpacePos = uCamera.invProjectionMatrix * clipSpacePos;
	vec3 viewDir = normalize(viewSpacePos.xyz / viewSpacePos.w);

	vec3 normal = texture(uNormalBuffer, inData.texCoord).xyz;

	vec3 reflectedDir = vec3(uCamera.invViewMatrix * vec4(reflect(viewDir, normal), 0.0));

	outColor = vec4(texture(uEnvironmentBuffer, reflectedDir).rgb, 1.0);

	// outColor.xyz = vec3(screenPos, 0.0);
	// outColor.xyz = vec3(uCamera.invViewMatrix * vec4(normal, 0.0));
}


#else
void main() {
}
#endif