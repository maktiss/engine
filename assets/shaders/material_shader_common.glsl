#if defined(RENDER_PASS_FORWARD) || defined(RENDER_PASS_DEPTH_NORMAL) || defined(RENDER_PASS_SHADOW_MAP)


#define FRAME_BLOCK_SET 0
#define CAMERA_BLOCK_SET 1
#define ENVIRONMENT_BLOCK_SET 2
#define MATERIAL_BLOCK_SET 3


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


#endif