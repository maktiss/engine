#version 460

#if defined(RENDER_PASS_FORWARD) || defined(RENDER_PASS_DEPTH_NORMAL) || defined(RENDER_PASS_SHADOW_MAP)

#include "common.glsl"
#line 7


#if defined(MESH_TYPE_STATIC)

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

#endif // defined(MESH_TYPE_STATIC)


#if defined(MESH_TYPE_TERRAIN)

layout(location = 0) in vec3 aPosition;


void main() {
	// vec4 position = uModel.transformMatrix * vec4(aPosition, 1.0);

	// outData.worldPosition = position.xyz;
	// outData.texCoord	  = position.xz * 0.5;

	// position = uCamera.viewMatrix * position;

	// outData.position = position.xyz;

	// outData.tangentMatrix = mat3(uCamera.viewMatrix) * mat3(uModel.transformMatrix) *
	// 						mat3(vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0));

	// outData.screenPosition = uCamera.projectionMatrix * position;

	// gl_Position = outData.screenPosition;

	gl_Position = vec4(aPosition, 1.0);
}

#endif // defined(MESH_TYPE_TERRAIN)


#else
void main() {
}
#endif