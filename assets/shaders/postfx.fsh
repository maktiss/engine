#version 460


#ifdef RENDER_PASS_POSTFX


layout(location = 0) in InData {
	vec2 texCoord;
}
inData;


layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 0) uniform Params {
	mat4 viewProjectionMatrix;
} uParams;

layout(set = 0, binding = 1) uniform sampler2D uInput;


vec3 ACESFilm(vec3 x) {
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return (x * (a * x + b)) / (x * (c * x + d) + e);
}


void main() {
	vec3 color = texture(uInput, inData.texCoord).rgb;

	vec2 screenCoord = inData.texCoord * 2.0 - vec2(1.0);

	color *= 1.0 - pow(length(screenCoord) * 0.6, 2);

	color = ACESFilm(color);

	outColor = vec4(color, 1.0);
}


#else
void main() {
}
#endif