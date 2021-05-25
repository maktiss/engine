#version 460


#ifdef RENDER_PASS_VOLUMETRIC_LIGHT

#define CAMERA_SET_ID 0
#define ENVIRONMENT_SET_ID 1

#define SET_ID_OFFSET 2
#include "common.glsl"


layout(location = 0) in InData {
	vec2 texCoord;
}
inData;


layout(location = 0) out vec4 outColor;


layout(set = CAMERA_SET_ID, binding = 0) uniform Camera {
	mat4 invViewMatrix;
	mat4 invProjectionMatrix;
}
uCamera;

layout(set = ENVIRONMENT_SET_ID, binding = 0) uniform DirectionalLightMatricesBlock {
	mat4 uDirectionalLightMatrices[DIRECTIONAL_LIGHT_CASCADE_COUNT];
};

layout(set = ENVIRONMENT_SET_ID, binding = 1) uniform uSunDirectionBlock {
	vec3 uSunDirection;
};

layout(set = INPUT_TEXTURES_SET_ID, binding = 0) uniform sampler2D uDepthBuffer;
layout(set = INPUT_TEXTURES_SET_ID, binding = 1) uniform sampler2DArray uDirectionalShadowMap;


layout(constant_id = 0) const uint NUM_SAMPLES = 32;
#define INV_NUM_SAMPLES 1.0 / NUM_SAMPLES

#define NUM_SAMPLES_LIGHT	  1
#define NUM_INV_SAMPLES_LIGHT 1.0 / NUM_SAMPLES_LIGHT


const vec3 uBetaR = vec3(0.0000068, 0.0000114, 0.0000237);
const vec3 uBetaM = vec3(0.0000021, 0.0000021, 0.0000021) * 10;

const float uAbsorptionM = 1.1;

const float uInnerRadius = 6360000.0;
const float uOuterRadius = 6420000.0;

const float uSunIntensity = 20.0;

const float uScaleR = 8400.0;
const float uScaleM = 1500.0;

const float uG	= 0.6;
const float uG2 = uG * uG;

// const vec3 uSunDirection = vec3(0.0, 1.0, 0.0);


void main() {
	vec2 screenPos = inData.texCoord * 2.0 - vec2(1.0);
	screenPos.y *= -1.0;

	float depth = texture(uDepthBuffer, inData.texCoord).x;

	vec4 clipSpacePos = vec4(screenPos, depth, 1.0);

	vec4 viewSpacePos = uCamera.invProjectionMatrix * clipSpacePos;
	viewSpacePos /= viewSpacePos.w;

	vec3 worldSpacePos = (uCamera.invViewMatrix * viewSpacePos).xyz;

	float dist	   = length(viewSpacePos.xyz);
	// vec3 direction = worldSpacePos / dist;
	vec3 direction = (uCamera.invViewMatrix * vec4(normalize(viewSpacePos.xyz), 0.0)).xyz;

	vec3 sumR = vec3(0.0);
	vec3 sumM = vec3(0.0);

	vec2 opticalDepth = vec2(0.0);

	vec3 segmentRay		= direction * dist * INV_NUM_SAMPLES;
	float segmentLength = length(segmentRay);

	vec3 samplePos = worldSpacePos - direction * dist - segmentRay * 0.5 + vec3(0.0, uInnerRadius, 0.0);
	vec3 samplePos2 = worldSpacePos - direction * dist - segmentRay * 0.5;

	

	for (int i = 0; i < NUM_SAMPLES; i++) {
		samplePos += segmentRay;
		samplePos2 += segmentRay;


		float shadowAmount = 1.0;

		for (uint cascade = 0; cascade < DIRECTIONAL_LIGHT_CASCADE_COUNT; cascade++) {
			mat4 lightSpaceMatrix = uDirectionalLightMatrices[cascade];

			shadowAmount *=
				calcDirectionalShadow(samplePos2, uDirectionalShadowMap, cascade, lightSpaceMatrix);
		}


		float height = length(samplePos) - uInnerRadius;

		vec2 expScale = exp(-height / vec2(uScaleR, uScaleM)) * segmentLength;
		opticalDepth += expScale;

		vec3 p	   = samplePos + dot(uSunDirection, -samplePos) * uSunDirection;
		float dist = sqrt(uOuterRadius * uOuterRadius - dot(p, p)) - length(samplePos - p);

		vec2 opticalDepthLight = vec2(0.0);

		vec3 segmentLightRay	 = (uSunDirection * dist) * NUM_INV_SAMPLES_LIGHT;
		float segmentLightLength = length(segmentLightRay);

		vec3 sampleLightPos = samplePos - segmentLightRay * 0.5;

		for (int j = 0; j < NUM_SAMPLES_LIGHT; j++) {
			sampleLightPos += segmentLightRay;

			float heightLight = length(sampleLightPos) - uInnerRadius;

			opticalDepthLight += exp(-heightLight / vec2(uScaleR, uScaleM)) * segmentLightLength;
		}

		vec3 attenuation = exp(-uBetaR * (opticalDepth.x + opticalDepthLight.x) +
							   -uBetaM * uAbsorptionM * (opticalDepth.y + opticalDepthLight.y));

		attenuation *= shadowAmount;

		sumR += expScale.x * attenuation;
		sumM += expScale.y * attenuation;
	}

	float cosTheta	= dot(uSunDirection, direction);
	float cosTheta2 = cosTheta * cosTheta;

	float phaseR = (3.0 / (16.0 * PI)) * (1.0 + cosTheta2);
	float phaseM = (3.0 / (8.0 * PI)) * (1.0 - uG2) * (1.0 + cosTheta2) /
				   ((2.0 + uG2) * pow(1.0 + uG2 - 2.0 * uG * cosTheta, 1.5));

	outColor = vec4(uSunIntensity * (sumR * phaseR * uBetaR + sumM * phaseM * uBetaM), 1.0);

	// outColor = vec4(vec3(0.2, 0.0, 0.0) * shadowAmount, 1.0);

	// outColor = vec4(worldSpacePos, 1.0);

	// outColor = vec4(vec3(length(worldSpacePos) * 0.0005), 1.0);
}


#else
void main() {
}
#endif