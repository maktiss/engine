#version 460

#if defined(RENDER_PASS_FORWARD) || defined(RENDER_PASS_DEPTH_NORMAL) || defined(RENDER_PASS_SHADOW_MAP)

#include "common.glsl"
#line 7


layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;


layout(location = 0) out OutData {
	vec3 position;
	vec2 texCoord;

	vec3 worldPosition;
	vec3 worldNormal;
	vec4 screenPosition;

	mat3 tangentMatrix;
}
outData;


void main() {
	vec4 position = uModel.transformMatrix * vec4(aPosition, 1.0);

	outData.worldPosition = position.xyz;

	position = uCamera.viewMatrix * position;

	outData.position = position.xyz;
	outData.texCoord = aTexCoord;

	outData.tangentMatrix =
		mat3(uCamera.viewMatrix) * mat3(uModel.transformMatrix) * mat3(aTangent, aBitangent, aNormal);

	outData.screenPosition = uCamera.projectionMatrix * position;

	gl_Position = outData.screenPosition;
}


#else
void main() {
}
#endif