#define FRAME_BLOCK_SET 0
#define CAMERA_BLOCK_SET 1
#define ENVIRONMENT_BLOCK_SET 2
#define MATERIAL_BLOCK_SET 3


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
	vec3 lightPos;
} uEnvironment;