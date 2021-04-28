#define PI 3.14159265358979323846


#if defined(RENDER_PASS_FORWARD) || defined(RENDER_PASS_DEPTH_NORMAL) || defined(RENDER_PASS_SHADOW_MAP)


#define FRAME_BLOCK_SET 0
#define CAMERA_BLOCK_SET 1
#define ENVIRONMENT_BLOCK_SET 2
#define MATERIAL_BLOCK_SET 3
#define TEXTURE_BLOCK_SET 4


layout(constant_id = 0) const uint DIRECTIONAL_LIGHT_CASCADE_COUNT = 3;

layout(constant_id = 10) const uint CLUSTER_COUNT_X = 1;
layout(constant_id = 11) const uint CLUSTER_COUNT_Y = 1;
layout(constant_id = 12) const uint CLUSTER_COUNT_Z = 1;


struct DirectionalLight {
	vec3 direction;
	
	vec3 color;
	int shadowMapIndex;

	mat4 baseLightSpaceMatrix;
	mat4 lightSpaceMatrices[DIRECTIONAL_LIGHT_CASCADE_COUNT];
};

struct PointLight {
	vec3 position;
	float radius;

	vec3 color;
	uint shadowMapIndex;
};

// TODO: spot lights
struct SpotLight {
	vec3 position;
	float innerAngle;

	vec3 direction;
	float outerAngle;

	vec3 color;
	uint shadowMapIndex;

	mat4 lightSpaceMatrix;
};


struct LightCluster {
	uint start;
	uint endShadow;
	uint end;
};


layout(set = FRAME_BLOCK_SET, binding = 0) uniform FrameBlock {
	float dt;
} uFrame;

layout(set = FRAME_BLOCK_SET, binding = 1) uniform sampler2DArrayShadow uDirectionalShadowMapBuffer;
// layout(set = FRAME_BLOCK_SET, binding = 2) uniform samplerCubeArrayShadow uPointShadowMapBuffer;


layout(set = CAMERA_BLOCK_SET, binding = 0) uniform CameraBlock {
	mat4 viewMatrix;
	mat4 projectionMatrix;

	mat4 invViewMatrix;
	mat4 invProjectionMatrix;
} uCamera;


layout(set = ENVIRONMENT_BLOCK_SET, binding = 0) uniform EnvironmentBlock {
	bool useDirectionalLight;
	DirectionalLight directionalLight;
	
	LightCluster pointLightClusters[CLUSTER_COUNT_X * CLUSTER_COUNT_Y * CLUSTER_COUNT_Z];
	LightCluster spotLightClusters[CLUSTER_COUNT_X * CLUSTER_COUNT_Y * CLUSTER_COUNT_Z];
} uEnvironment;

layout(set = ENVIRONMENT_BLOCK_SET, binding = 1) readonly buffer PointLightsBlock {
	PointLight uPointLights[];
};

layout(set = ENVIRONMENT_BLOCK_SET, binding = 2) readonly buffer SpotLightsBlock {
	SpotLight uSpotLights[];
};


uint getClusterIndex(uint x, uint y, uint z) {
	return x * CLUSTER_COUNT_Y * CLUSTER_COUNT_Z + y * CLUSTER_COUNT_Z + z;
}


float calcNormalDistributionTrowbridgeReitz(vec3 normal, vec3 halfway, float roughness) {
	float roughness2 = roughness * roughness;
	float normalDotHalfway = max(dot(normal, halfway), 0.0);
	float normalDotHalfway2 = normalDotHalfway * normalDotHalfway;

	float numerator = roughness2;
	float denominator = normalDotHalfway2 * (roughness2 - 1.0) + 1.0;
	denominator = PI * denominator * denominator;

	return numerator / denominator;
}

float calcGeometrySchlickBeckman(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float normalDotViewDir = max(dot(normal, viewDir), 0.001);
	float normalDotLightDir = max(dot(normal, lightDir), 0.001);

	float smithL = normalDotViewDir / (normalDotViewDir * (1.0 - k) + k);
	float smithV = normalDotLightDir / (normalDotLightDir * (1.0 - k) + k);
	return smithL * smithV;
}

float calcGeometrySchlickGGX(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
	float k = roughness / 2;

	float normalDotViewDir = max(dot(normal, viewDir), 0.001);
	float normalDotLightDir = max(dot(normal, lightDir), 0.001);

	float smithL = normalDotViewDir / (normalDotViewDir * (1.0 - k) + k);
	float smithV = normalDotLightDir / (normalDotLightDir * (1.0 - k) + k);
	return smithL * smithV;
}

vec3 calcFresnelSchlick(vec3 F0, float halfwayDotViewDir) {
	// return F0 + (1.0 - F0) * pow(1.0 - halfwayDotViewDir, 5.0);
	return mix(F0, vec3(1.0), pow(1.0 - halfwayDotViewDir, 5.0));
}

vec3 calcBRDF(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 albedo, float metallic, float roughness) {
	vec3 halfway = normalize(viewDir + lightDir);
	float normalDotLightDir = max(dot(normal, lightDir), 0.0);
	float halfwayDotViewDir = max(dot(halfway, viewDir), 0.0);
	float normalDotViewDir = max(dot(normal, viewDir), 0.0);

	float NDF = calcNormalDistributionTrowbridgeReitz(normal, halfway, roughness);
	float G = calcGeometrySchlickGGX(normal, viewDir, lightDir, roughness);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F = calcFresnelSchlick(F0, halfwayDotViewDir);

	vec3 specular = (NDF * G * F) / max(4.0 * normalDotViewDir * normalDotLightDir, 0.001);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	return kD * albedo / PI + specular;
}


#endif