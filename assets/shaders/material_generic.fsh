#version 460

// #define DEBUG

#include "material_common.glsl"


#if defined(RENDER_PASS_FORWARD) || defined(RENDER_PASS_DEPTH_NORMAL)


layout(location = 0) in InData {
	vec3 position;
	vec2 texCoord;

	vec3 worldPosition;
	vec3 worldNormal;
	vec4 screenPosition;

	mat3 tangentMatrix;
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
	screenCoord		 = screenCoord * vec2(0.5, -0.5) + vec2(0.5);

	vec3 viewDir = -normalize(inData.position);

	vec3 colorAlbedo = uMaterial.color.rgb;

#ifdef USE_TEXTURE_ALBEDO
	colorAlbedo *= texture(sampler2D(uTextures[uMaterial.textureAlbedo], uSampler), inData.texCoord).rgb;
#endif

	vec3 normal = texture(uNormalBuffer, screenCoord).xyz;

	float metallic		   = uMaterial.metallic;
	float roughness		   = uMaterial.roughness;
	float ambientOcclusion = uMaterial.ambientOcclusion;

#ifdef USE_TEXTURE_MRA
	vec3 mra = texture(sampler2D(uTextures[uMaterial.textureMRA], uSampler), inData.texCoord).rgb;

	metallic *= mra.r;
	roughness *= mra.g;
	ambientOcclusion *= mra.b;
#endif

	normal			 = normalize(normal);
	vec3 worldNormal = vec3(uCamera.invViewMatrix * vec4(normal, 0.0));

	vec3 irradiance = texture(uIrradianceMap, worldNormal).rgb;

	vec3 color = colorAlbedo * ambientOcclusion * irradiance;

	vec3 reflectedColor = texture(uReflectionBuffer, screenCoord).rgb;
	vec3 reflectedDir	= normalize(reflect(-viewDir, normal));

	reflectedColor *= calcBRDF(normal, reflectedDir, viewDir, colorAlbedo, metallic, roughness);
	color += reflectedColor * max(dot(normal, reflectedDir), 0.0);

	if (uEnvironment.useDirectionalLight) {
		float shadowAmount = calcDirectionalShadow(inData.worldPosition, normalize(inData.worldNormal));

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
	vec3 normal = inData.tangentMatrix *
				  (texture(sampler2D(uTextures[uMaterial.textureNormal], uSampler), inData.texCoord).rgb * 2.0 - 1.0);
#else
	vec3 normal = inData.tangentMatrix[2];
#endif

	outNormal = vec4(normalize(normal), 1.0);
}


#elif defined(RENDER_PASS_SHADOW_MAP)


layout(location = 0) out vec4 outMoments;


void main() {
	float depth = gl_FragCoord.z;

	float dx = dFdx(depth);
	float dy = dFdy(depth);

	outMoments = vec4(depth, depth * depth + 0.25 * (dx * dx + dy * dy), 0.0, 1.0);
}


#else
void main() {
}
#endif