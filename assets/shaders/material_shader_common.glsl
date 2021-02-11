#define FRAME_BLOCK_SET 0
#define CAMERA_BLOCK_SET 1
#define ENVIRONMENT_BLOCK_SET 2
#define MATERIAL_BLOCK_SET 3


layout(constant_id = 0) const int DIRECTIONAL_LIGHT_CASCADE_COUNT = 1;

layout(constant_id = 1) const int MAX_POINT_LIGHTS_PER_CLUSTER = 8;

layout(constant_id = 10) const int CLUSTER_COUNT_X = 1;
layout(constant_id = 11) const int CLUSTER_COUNT_Y = 1;
layout(constant_id = 12) const int CLUSTER_COUNT_Z = 1;


struct DirectionalLight {
	vec3 direction;
	vec3 color;

	int shadowMapIndex;
	mat4 lightSpaceMatrices[DIRECTIONAL_LIGHT_CASCADE_COUNT];
};

struct PointLight {
	vec3 position;
	float radius;

	vec3 color;
	int shadowMapIndex;

	mat4 lightSpaceMatrices[6];
};


layout(set = FRAME_BLOCK_SET, binding = 0) uniform FrameBlock {
	float dt;
} uFrame;


layout(set = CAMERA_BLOCK_SET, binding = 0) uniform CameraBlock {
	mat4 viewMatrix;
	mat4 projectionMatrix;

	mat4 invViewMatrix;
	mat4 invProjectionMatrix;
} uCamera;

layout(set = ENVIRONMENT_BLOCK_SET, binding = 0) uniform EnvironmentBlock {
	DirectionalLight directionalLight;

	int pointLightCount;
	PointLight pointLights[MAX_POINT_LIGHTS_PER_CLUSTER];
} uEnvironment;