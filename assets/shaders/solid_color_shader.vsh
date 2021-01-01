#version 460

#include "material_shader_common.glsl"


layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec3 outColor;


vec3 colors[3] = vec3[](
	vec3(0.6, 0.1, 1.0),
	vec3(0.1, 0.6, 1.0),
	vec3(1.0, 0.1, 0.6)
);


void main() {
	outColor = colors[gl_VertexIndex];

	gl_Position = vec4(aPosition, 1.0);
}