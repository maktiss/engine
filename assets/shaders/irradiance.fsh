#version 460


#define PI 3.14159265358979323846


#ifdef RENDER_PASS_IRRADIANCE_MAP


layout(location = 0) in InData {
	vec3 texCoord;
}
inData;


layout(location = 0) out vec4 outColor;


const uint sampleCount = 64;


layout(set = 0, binding = 0) uniform samplerCube uEnvironmentMap;

layout(set = 2, binding = 0) uniform SampleBlock {
	vec3 samples[sampleCount];
} uSample;


void main() {
	vec3 color = vec3(0.0);

	vec3 normal = normalize(inData.texCoord);
	vec3 tangent = normalize(cross(normal, vec3(0.0, 1.0, 0.0)));
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