#version 460
#extension GL_EXT_multiview : enable


layout(location = 0) in vec3 aPosition;


#ifdef RENDER_PASS_SKYMAP


#define PI 3.14159265359

#define NUM_SAMPLES		64
#define INV_NUM_SAMPLES 0.015625

#define NUM_SAMPLES_LIGHT	  3
#define NUM_INV_SAMPLES_LIGHT 1


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


layout(location = 0) out OutData {
	vec3 texCoord;

	vec3 sumR;
	vec3 sumM;
}
outData;


layout(set = 0, binding = 0) uniform CameraBlock {
	mat4 viewProjectionMatrices[6];
}
uCamera;

layout(set = 1, binding = 0) uniform ParamBlock {
	vec3 sunDirection;
}
uParams;


void main() {
	vec3 direction = aPosition;

	vec3 origin = vec3(0.0, uInnerRadius, 0.0);
	vec3 proj	= dot(direction, -origin) * direction;

	vec3 point = origin + proj;

	float dist = sqrt(pow(uOuterRadius, 2) - dot(point, point)) - length(proj);

	vec3 sumR = vec3(0.0);
	vec3 sumM = vec3(0.0);

	vec2 opticalDepth = vec2(0.0);

	vec3 segmentRay		= direction * dist * INV_NUM_SAMPLES;
	float segmentLength = length(segmentRay);

	vec3 samplePos = -segmentRay * 0.5 + vec3(0.0, uInnerRadius, 0.0);
	for (int i = 0; i < NUM_SAMPLES; i++) {
		samplePos += segmentRay;
		float height = length(samplePos) - uInnerRadius;

		vec2 expScale = exp(-height / vec2(uScaleR, uScaleM)) * segmentLength;
		opticalDepth += expScale;

		vec3 p	   = samplePos + dot(uParams.sunDirection, -samplePos) * uParams.sunDirection;
		float dist = sqrt(uOuterRadius * uOuterRadius - dot(p, p)) - length(samplePos - p);

		vec2 opticalDepthLight = vec2(0.0);

		vec3 segmentLightRay	 = (uParams.sunDirection * dist) * NUM_INV_SAMPLES_LIGHT;
		float segmentLightLength = length(segmentLightRay);

		vec3 sampleLightPos = samplePos - segmentLightRay * 0.5;

		for (int j = 0; j < NUM_SAMPLES_LIGHT; j++) {
			sampleLightPos += segmentLightRay;

			float heightLight = length(sampleLightPos) - uInnerRadius;

			opticalDepthLight += exp(-heightLight / vec2(uScaleR, uScaleM)) * segmentLightLength;
		}

		vec3 attenuation = exp(-uBetaR * (opticalDepth.x + opticalDepthLight.x) +
							   -uBetaM * uAbsorptionM * (opticalDepth.y + opticalDepthLight.y));

		sumR += expScale.x * attenuation;
		sumM += expScale.y * attenuation;
	}

	outData.texCoord = aPosition;

	outData.sumR = sumR;
	outData.sumM = sumM;


	vec4 position = uCamera.viewProjectionMatrices[gl_ViewIndex] * vec4(aPosition, 0.0);
	gl_Position	  = position.xyzz;
}


#else
void main() {
}
#endif