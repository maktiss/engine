#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(0.2, 0.8, 0.5, 1.0);
}