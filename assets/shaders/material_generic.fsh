#version 460

// #define DEBUG

#include "material_common.glsl"


#if defined(RENDER_PASS_FORWARD) || defined(RENDER_PASS_DEPTH_NORMAL)


layout(location = 0) in InData {
	vec3 position;
	vec2 texCoord;

	vec3 worldPosition;
	vec4 screenPosition;

	mat3 tbnMatrix;
}
inData;


layout(set = MATERIAL_BLOCK_SET, binding = 0) uniform MaterialBlock {
	vec4 color;

	float metallic;
	float roughness;
	float ambientOcclusion;

	uint textureAlbedo;
	uint textureNormal;
	uint textureMRA;
}
uMaterial;

layout(set = TEXTURE_BLOCK_SET, binding = 0) uniform sampler uSampler;
layout(set = TEXTURE_BLOCK_SET, binding = 1) uniform texture2D uTextures[1024];


#endif


#ifdef RENDER_PASS_FORWARD


layout(location = 0) out vec4 outColor;


void main() {
	vec2 screenCoord = inData.screenPosition.xy / inData.screenPosition.w;
	screenCoord = screenCoord * vec2(0.5, -0.5) + vec2(0.5);

	vec3 viewDir = -normalize(inData.position);

	vec3 colorAlbedo = uMaterial.color.rgb;

#ifdef USE_TEXTURE_ALBEDO
	colorAlbedo *= texture(sampler2D(uTextures[uMaterial.textureAlbedo], uSampler), inData.texCoord).rgb;
#endif

	vec3 normal = texture(uNormalBuffer, screenCoord).xyz;

	float metallic = uMaterial.metallic;
	float roughness = uMaterial.roughness;
	float ambientOcclusion = uMaterial.ambientOcclusion;

#ifdef USE_TEXTURE_MRA
	vec3 mra = texture(sampler2D(uTextures[uMaterial.textureMRA], uSampler), inData.texCoord).rgb;

	metallic *= mra.r;
	roughness *= mra.g;
	ambientOcclusion *= mra.b;
#endif

	normal = normalize(normal);

	vec3 ambientLight = vec3(0.30, 0.33, 0.34);

	vec3 color = colorAlbedo * ambientOcclusion * ambientLight;

	vec3 reflectedColor = texture(uReflectionBuffer, screenCoord).rgb;
	vec3 reflectedDir = normalize(reflect(-viewDir, normal));

	reflectedColor *= calcBRDF(normal, reflectedDir, viewDir, colorAlbedo, metallic, roughness);
	color += reflectedColor * max(dot(normal, reflectedDir), 0.0);

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

		vec3 lightDir = -uEnvironment.directionalLight.direction;

		vec3 lightColor = calcBRDF(normal, lightDir, viewDir, colorAlbedo, metallic, roughness);

		color += shadowAmount * lightColor * 4 * uEnvironment.directionalLight.color * max(dot(normal, lightDir), 0.0);

		// color += shadowAmount * colorAlbedo * uEnvironment.directionalLight.color *
		// 		 max(dot(-uEnvironment.directionalLight.direction, normal), 0.0);
	}

	outColor = vec4(color, 1.0);
}


#elif defined(RENDER_PASS_DEPTH_NORMAL)


layout(location = 0) out vec4 outNormal;


void main() {
#ifdef USE_TEXTURE_NORMAL
	vec3 normal = inData.tbnMatrix *
				  (texture(sampler2D(uTextures[uMaterial.textureNormal], uSampler), inData.texCoord).rgb * 2.0 - 1.0);
#else
	vec3 normal = inData.tbnMatrix[2];
#endif

	outNormal = vec4(normalize(normal), 1.0);
}


#else
void main() {
}
#endif