#version 460

// #define DEBUG

#include "material_shader_common.glsl"


layout(location = 0) in InData {
	vec3 position;
	vec2 texCoord;

	vec3 worldPosition;

	mat3 tbnMatrix;
}
inData;


layout(location = 0) out vec4 outColor;


layout(set = MATERIAL_BLOCK_SET, binding = 0) uniform MaterialBlock {
	vec4 color;
}
uMaterial;

layout(set = MATERIAL_BLOCK_SET, binding = 1) uniform sampler2D uTextures[8];


#define ALBEDO 0
#define NORMAL 1


#ifdef RENDER_PASS_FORWARD
void main() {
	vec3 colorAlbedo = uMaterial.color.rgb;

#ifdef USE_TEXTURE_ALBEDO
	colorAlbedo *= texture(uTextures[ALBEDO], inData.texCoord).rgb;
#endif

#ifdef USE_TEXTURE_NORMAL
	vec3 normal = inData.tbnMatrix * (texture(uTextures[NORMAL], inData.texCoord).rgb * 2.0 - 1.0);
#else
	vec3 normal = inData.tbnMatrix[2];
#endif
#line 46

	normal = normalize(normal);

	vec3 ambientLight = vec3(0.30, 0.33, 0.34);

	vec3 color = colorAlbedo * ambientLight;

	if (uEnvironment.useDirectionalLight) {
		float shadowAmount = 1.0f;

		vec4 baseLightSpaceCoord = uEnvironment.directionalLight.baseLightSpaceMatrix * vec4(inData.worldPosition, 1.0);

		const float directionalLightCascadeBase = 2.0;

		vec4 cameraDir = uEnvironment.directionalLight.baseLightSpaceMatrix *
						 vec4(uCamera.viewMatrix[0][2], uCamera.viewMatrix[1][2], uCamera.viewMatrix[2][2], 0.0) *
						 directionalLightCascadeBase;

		const float offset = 0.75;

		// TODO: more meaningful name?
		vec2 cascadeRelatedPos =
			baseLightSpaceCoord.xy / (cameraDir.xy * (2 * offset * step(0.0, baseLightSpaceCoord.xy) - offset) + 1.0);

		uint cascade = uint(max(0.0, log2(max(abs(cascadeRelatedPos.x), abs(cascadeRelatedPos.y))) + 1.0));

#ifdef DEBUG
		if (cascade == 0) {
			color.r += 0.2;
		} else if (cascade == 1) {
			color.g += 0.2;
		} else if (cascade == 2) {
			color.b += 0.2;
		}
#endif

		vec4 lightSpaceCoord =
			uEnvironment.directionalLight.lightSpaceMatrices[cascade] * vec4(inData.worldPosition, 1.0);

		lightSpaceCoord.xy *= vec2(0.5, -0.5);
		lightSpaceCoord.xy += vec2(0.5);

		// TODO: normal bias
		float bias = 0.00001;

		shadowAmount *=
			texture(uDirectionalShadowMapBuffer, vec4(lightSpaceCoord.xy, cascade, lightSpaceCoord.z - bias)).r;

		color += shadowAmount * colorAlbedo * uEnvironment.directionalLight.color *
				 max(dot(-uEnvironment.directionalLight.direction, normal), 0.0);
	}

	outColor = vec4(color, 1.0);
}


#else
void main() {
}
#endif