#version 460

#include "material_shader_common.glsl"


layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;


layout(push_constant) uniform ModelBlock {
	mat4 transformMatrix;
} uModel;


void main() {
	gl_Position = uModel.transformMatrix * vec4(aPosition, 1.0);
}