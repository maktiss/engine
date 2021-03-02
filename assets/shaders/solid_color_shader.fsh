#version 460

#include "material_shader_common.glsl"


layout(location = 0) in InData {
	vec3 position;
	vec2 texCoord;

	vec3 worldPosition;

	mat3 tbnMatrix;
} inData;


layout(location = 0) out vec4 outColor;



layout(set = MATERIAL_BLOCK_SET, binding = 0) uniform MaterialBlock {
	vec4 color;
} uMaterial;

layout(set = MATERIAL_BLOCK_SET, binding = 1) uniform sampler2D uTextures[8];


#define ALBEDO 0
#define NORMAL 1


#ifdef RENDER_PASS_FORWARD
void main() {
	vec3 colorAlbedo = uMaterial.color.rgb;

#ifdef USE_TEXTURE_ALBEDO
	colorAlbedo *= texture(uTextures[ALBEDO], inData.texCoord).rgb;
#endif

	vec3 normal = inData.tbnMatrix[2];

#ifdef USE_TEXTURE_NORMAL
	normal = inData.tbnMatrix * (texture(uTextures[NORMAL], inData.texCoord).rgb * 2.0 - 1.0);
#endif

	normal = normalize(normal);

	vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));

	vec3 color = colorAlbedo * 0.5;

	for (int cascadeIndex = 0; cascadeIndex < 1; cascadeIndex++) {
		vec4 lightSpaceCoord = uEnvironment.directionalLight.lightSpaceMatrices[cascadeIndex] * vec4(inData.worldPosition, 1.0);
		lightSpaceCoord.xy *= vec2(0.5, -0.5);
		lightSpaceCoord.xy += vec2(0.5);
		float shadowAmount = texture(uShadowMapBuffer, vec4(lightSpaceCoord.xy, cascadeIndex, lightSpaceCoord.z - 0.01)).r;

		color += shadowAmount * colorAlbedo * uEnvironment.directionalLight.color *
				max(dot(-uEnvironment.directionalLight.direction, normal), 0.0);

		// color = lightSpaceCoord.xyz;
		// color = vec3(max(0.5, shadowAmount));
	}



	outColor = vec4(color, 1.0);
}


#else
void main() {
}
#endif