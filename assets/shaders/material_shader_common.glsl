#define FRAME_BLOCK_LOCATION 0
#define CAMERA_BLOCK_LOCATION 1
#define MODEL_BLOCK_LOCATION 2
#define ENVIRONMENT_BLOCK_LOCATION 3
#define MATERIAL_BLOCK_LOCATION 4


layout(binding = FRAME_BLOCK_LOCATION) uniform FrameBlock {
	float dt;
} uFrame;


layout(binding = CAMERA_BLOCK_LOCATION) uniform CameraBlock {
	mat4 viewMatrix;
	mat4 projectionMatrix;

	mat4 invViewMatrix;
	mat4 invProjectionMatrix;
} uCamera;


// layout(binding = MODEL_BLOCK_LOCATION) uniform ModelBlock {
// 	mat4 modelMatrix;
// } uModel;


layout(binding = ENVIRONMENT_BLOCK_LOCATION) uniform EnvironmentBlock {
	vec3 lightPos;
} uEnvironment;