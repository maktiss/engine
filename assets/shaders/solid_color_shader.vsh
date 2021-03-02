#version 460

#include "material_shader_common.glsl"


layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;


layout(location = 0) out OutData {
	vec3 position;
	vec2 texCoord;

	vec3 worldPosition;

	mat3 tbnMatrix;
} outData;


layout(push_constant) uniform ModelBlock {
	mat4 transformMatrix;
} uModel;


void main() {
	vec4 position = uModel.transformMatrix * vec4(aPosition, 1.0);

	outData.worldPosition = position.xyz;

	position = uCamera.viewMatrix * position;

	outData.position = position.xyz;
	outData.texCoord = aTexCoord;

	outData.tbnMatrix = mat3(uCamera.viewMatrix * uModel.transformMatrix) * mat3(aTangent, aBitangent, aNormal);

	gl_Position = uCamera.projectionMatrix * position;
}