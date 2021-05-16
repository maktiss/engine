// #define DEBUG

#define PI 3.14159265358979323846


// ====================================

layout(constant_id = 100) const uint DIRECTIONAL_LIGHT_CASCADE_COUNT   = 3;
layout(constant_id = 101) const float DIRECTIONAL_LIGHT_CASCADE_BASE   = 2.0;
layout(constant_id = 102) const float DIRECTIONAL_LIGHT_CASCADE_OFFSET = 0.75;

layout(constant_id = 110) const uint CLUSTER_COUNT_X = 1;
layout(constant_id = 111) const uint CLUSTER_COUNT_Y = 1;
layout(constant_id = 112) const uint CLUSTER_COUNT_Z = 1;

const uint CLUSTER_COUNT = CLUSTER_COUNT_X * CLUSTER_COUNT_Y * CLUSTER_COUNT_Z;

layout(constant_id = 200) const uint MAX_TEXTURES = 1024;


// ====================================

struct DirectionalLight {
	vec3 direction;
	bool enabled;

	vec3 color;
	int shadowMapIndex;
};

struct PointLight {
	vec3 position;
	float radius;

	vec3 color;
	uint shadowMapIndex;
};

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


// ====================================

// #define FRAME_SET_ID		  (SET_ID_OFFSET + 0)
#define INPUT_TEXTURES_SET_ID (SET_ID_OFFSET + 0)
#define TEXTURES_SET_ID		  (SET_ID_OFFSET + 1)
#define MATERIAL_SET_ID		  (SET_ID_OFFSET + 2)


// ====================================

#if defined(RENDER_PASS_FORWARD)

#define CAMERA_SET_ID	   0
#define ENVIRONMENT_SET_ID 1

#define SET_ID_OFFSET 2


layout(push_constant) uniform ModelBlock {
	mat4 transformMatrix;
}
uModel;


layout(set = INPUT_TEXTURES_SET_ID, binding = 0) uniform sampler2DArray uDirectionalShadowMapBuffer;
layout(set = INPUT_TEXTURES_SET_ID, binding = 1) uniform sampler2D uNormalBuffer;
layout(set = INPUT_TEXTURES_SET_ID, binding = 2) uniform sampler2D uReflectionBuffer;
layout(set = INPUT_TEXTURES_SET_ID, binding = 3) uniform samplerCube uIrradianceMap;


layout(set = CAMERA_SET_ID, binding = 0) uniform CameraBlock {
	mat4 viewMatrix;
	mat4 projectionMatrix;

	mat4 invViewMatrix;
	mat4 invProjectionMatrix;
}
uCamera;


layout(set = ENVIRONMENT_SET_ID, binding = 0) uniform DirectionalLightBlock {
	DirectionalLight uDirectionalLight;
};

layout(set = ENVIRONMENT_SET_ID, binding = 1) uniform DirectionalLightMatricesBlock {
	mat4 uDirectionalLightMatrices[DIRECTIONAL_LIGHT_CASCADE_COUNT];
};

layout(set = ENVIRONMENT_SET_ID, binding = 2) uniform PointLightClusterBlock {
	LightCluster uPointLightClusters[CLUSTER_COUNT];
};

layout(set = ENVIRONMENT_SET_ID, binding = 3) uniform SpotLightClusterBlock {
	LightCluster uSpotLightClusters[CLUSTER_COUNT];
};

layout(set = ENVIRONMENT_SET_ID, binding = 4) readonly buffer PointLightsBlock {
	PointLight uPointLights[];
};

layout(set = ENVIRONMENT_SET_ID, binding = 5) readonly buffer SpotLightsBlock {
	SpotLight uSpotLights[];
};


#endif // defined(RENDER_PASS_FORWARD)


#if defined(RENDER_PASS_DEPTH_NORMAL) | defined(RENDER_PASS_SHADOW_MAP)

#define CAMERA_SET_ID 0

#define SET_ID_OFFSET 1


layout(push_constant) uniform ModelBlock {
	mat4 transformMatrix;
}
uModel;


layout(set = CAMERA_SET_ID, binding = 0) uniform CameraBlock {
	mat4 viewMatrix;
	mat4 projectionMatrix;

	mat4 invViewMatrix;
	mat4 invProjectionMatrix;
}
uCamera;


#endif // defined(RENDER_PASS_DEPTH_NORMAL) | defined(RENDER_PASS_SHADOW_MAP)


// ====================================

// layout(set = FRAME_SET_ID, binding = 0) uniform FrameBlock {
// 	float deltaTime;
// }
// uFrame;

layout(set = TEXTURES_SET_ID, binding = 0) uniform sampler uSampler;
layout(set = TEXTURES_SET_ID, binding = 1) uniform texture2D uTextures[MAX_TEXTURES];


// ====================================

uint getClusterIndex(uint x, uint y, uint z) {
	return x * CLUSTER_COUNT_Y * CLUSTER_COUNT_Z + y * CLUSTER_COUNT_Z + z;
}


float calcChebyshevUpperBound(vec2 moments, float sampleDepth) {
	float p = float(sampleDepth <= moments.x);

	// TODO: calculate biase based on frustum size
	float bias	   = 0.00001;
	float variance = max(bias, moments.y - (moments.x * moments.x));

	float d	   = sampleDepth - moments.x;
	float pMax = variance / (variance + d * d);

	return max(p, pMax);
}


// float calcDirectionalShadow(vec3 worldPosition) {
// 	float shadowAmount = 1.0;

// 	for (uint cascade = 0; cascade < DIRECTIONAL_LIGHT_CASCADE_COUNT; cascade++) {
// 		vec4 lightSpaceCoord = uEnvironment.directionalLight.lightSpaceMatrices[cascade] * vec4(worldPosition, 1.0);

// 		lightSpaceCoord.xy *= vec2(0.5, -0.5);
// 		lightSpaceCoord.xy += vec2(0.5);

// 		vec3 texCoord = vec3(lightSpaceCoord.xy, cascade);
// 		vec2 moments  = texture(uDirectionalShadowMapBuffer, texCoord).xy;

// 		shadowAmount *= max(step(1.0, lightSpaceCoord.z), calcChebyshevUpperBound(moments, lightSpaceCoord.z));
// 	}

// 	return shadowAmount;
// }


float calcDirectionalShadow(vec3 worldPosition, sampler2DArray shadowMapBuffer, uint cascade, mat4 lightSpaceMatrix) {
	vec4 lightSpaceCoord = lightSpaceMatrix * vec4(worldPosition, 1.0);

	lightSpaceCoord.xy *= vec2(0.5, -0.5);
	lightSpaceCoord.xy += vec2(0.5);

	vec3 texCoord = vec3(lightSpaceCoord.xy, cascade);
	vec2 moments  = texture(shadowMapBuffer, texCoord).xy;

	return max(step(1.0, lightSpaceCoord.z), calcChebyshevUpperBound(moments, lightSpaceCoord.z));
}


float calcNormalDistributionTrowbridgeReitz(vec3 normal, vec3 halfway, float roughness) {
	float roughness2		= roughness * roughness;
	float normalDotHalfway	= max(dot(normal, halfway), 0.0);
	float normalDotHalfway2 = normalDotHalfway * normalDotHalfway;

	float numerator	  = roughness2;
	float denominator = normalDotHalfway2 * (roughness2 - 1.0) + 1.0;
	denominator		  = PI * denominator * denominator;

	return numerator / denominator;
}

float calcGeometrySchlickBeckman(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float normalDotViewDir	= max(dot(normal, viewDir), 0.00001);
	float normalDotLightDir = max(dot(normal, lightDir), 0.00001);

	float smithL = normalDotViewDir / (normalDotViewDir * (1.0 - k) + k);
	float smithV = normalDotLightDir / (normalDotLightDir * (1.0 - k) + k);

	return smithL * smithV;
}

float calcGeometrySchlickGGX(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
	float k = roughness / 2;

	float normalDotViewDir	= max(dot(normal, viewDir), 0.00001);
	float normalDotLightDir = max(dot(normal, lightDir), 0.00001);

	float smithL = normalDotViewDir / (normalDotViewDir * (1.0 - k) + k);
	float smithV = normalDotLightDir / (normalDotLightDir * (1.0 - k) + k);

	return smithL * smithV;
}

vec3 calcFresnelSchlick(vec3 F0, float halfwayDotViewDir) {
	// return F0 + (1.0 - F0) * pow(1.0 - halfwayDotViewDir, 5.0);
	return mix(F0, vec3(1.0), pow(1.0 - halfwayDotViewDir, 5.0));
}

vec3 calcBRDF(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 albedo, float metallic, float roughness) {
	vec3 halfway			= normalize(viewDir + lightDir);
	float normalDotLightDir = max(dot(normal, lightDir), 0.0);
	float halfwayDotViewDir = max(dot(halfway, viewDir), 0.0);
	float normalDotViewDir	= max(dot(normal, viewDir), 0.0);

	float NDF = calcNormalDistributionTrowbridgeReitz(normal, halfway, roughness);
	float G	  = calcGeometrySchlickGGX(normal, viewDir, lightDir, roughness);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F	= calcFresnelSchlick(F0, halfwayDotViewDir);

	vec3 specular = (NDF * G * F) / max(4.0 * normalDotViewDir * normalDotLightDir, 0.00001);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	return kD * albedo / PI + specular;
}