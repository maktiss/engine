#version 460


#ifdef RENDER_PASS_SKYMAP


#define PI 3.14159265359


layout(location = 0) in InData {
	vec3 texCoord;

	vec3 sumR;
	vec3 sumM;
}
inData;


layout(location = 0) out vec4 outColor;


const vec3 uBetaR = vec3(0.0000068, 0.0000114, 0.0000237);
const vec3 uBetaM = vec3(0.0000021, 0.0000021, 0.0000021);

const float uAbsorptionM = 1.1;

const float uInnerRadius = 6360000.0;
const float uOuterRadius = 6420000.0;

const float uSunIntensity = 20.0;

const float uScaleR = 8400.0;
const float uScaleM = 1500.0;

const float uG = 0.9;
const float uG2 = uG * uG;


layout(set = 1, binding = 0) uniform ParamBlock {
	vec3 sunDirection;
} uParams;


void main() {
	outColor = vec4(inData.texCoord, 1.0);


	float cosTheta = dot(uParams.sunDirection, inData.texCoord) / length(inData.texCoord);
	float cosTheta2 = cosTheta * cosTheta;

	float phaseR = (3.0 / (16.0 * PI)) * (1.0 + cosTheta2);
	float phaseM = (3.0 / (8.0 * PI)) * (1.0 - uG2) * (1.0 + cosTheta2) / ((2.0 + uG2) * pow(1.0 + uG2 - 2.0 * uG * cosTheta, 1.5));

	outColor = vec4(uSunIntensity * (inData.sumR * phaseR * uBetaR + inData.sumM * phaseM * uBetaM), 1.0);

	outColor += outColor * uSunIntensity * smoothstep(0.9997, 0.9998, cosTheta);
	outColor.a = 1.0;
}


#else
void main() {
}
#endif