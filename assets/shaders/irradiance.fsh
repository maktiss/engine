#version 460


#ifdef RENDER_PASS_IRRADIANCE_MAP

#define CAMERA_SET_ID 0
#define SAMPLE_SET_ID 1

#define SET_ID_OFFSET 2
#include "common.glsl"


layout(location = 0) in InData {
	vec3 texCoord;
}
inData;


layout(location = 0) out vec4 outColor;


// FIXME: spec constant
const uint sampleCount = 64;

layout(set = SAMPLE_SET_ID, binding = 0) uniform SampleBlock {
	vec3 samples[sampleCount];
}
uSample;

layout(set = INPUT_TEXTURES_SET_ID, binding = 0) uniform samplerCube uEnvironmentMap;


void main() {
	vec3 color = vec3(0.0);

	vec3 normal	   = normalize(inData.texCoord);
	vec3 tangent   = normalize(cross(normal, vec3(0.0, 1.0, 0.0)));
	vec3 bitangent = normalize(cross(normal, tangent));

	mat3 tangentSpaceMatrix = mat3(tangent, normal, bitangent);

	for (uint i = 0; i < sampleCount; i++) {
		vec3 sampleDir = tangentSpaceMatrix * uSample.samples[i];
		color += textureLod(uEnvironmentMap, sampleDir, 8).rgb;
	}

	color = color / sampleCount;

	outColor = vec4(color, 1.0);
}


#else
void main() {
}
#endif