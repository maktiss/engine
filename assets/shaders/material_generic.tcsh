#version 460

layout(vertices = 4) out;

#if defined(RENDER_PASS_FORWARD) || defined(RENDER_PASS_DEPTH_NORMAL) || defined(RENDER_PASS_SHADOW_MAP)

#include "common.glsl"
#line 9

void main() {
	if (gl_InvocationID == 0) {
		gl_TessLevelInner[0] = 16.0;
		gl_TessLevelInner[1] = 16.0;

		gl_TessLevelOuter[0] = 16.0 * uModel.terrainTessFactors.x; // right
		gl_TessLevelOuter[1] = 16.0 * uModel.terrainTessFactors.y; // bottom
		gl_TessLevelOuter[2] = 16.0 * uModel.terrainTessFactors.z; // left
		gl_TessLevelOuter[3] = 16.0 * uModel.terrainTessFactors.w; // top
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}


#else
void main() {
}
#endif