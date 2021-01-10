#version 460

#include "material_shader_common.glsl"


layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 outTexCoord;


layout(push_constant) uniform ModelBlock {
	mat4 transformMatrix;
} uModel;


void main() {
	outTexCoord = aTexCoord;

	outTexCoord.x = length(aPosition) * 0.1;

	gl_Position = uCamera.projectionMatrix * uCamera.viewMatrix * uModel.transformMatrix * vec4(aPosition, 1.0);
}