#version 460


#ifdef RENDER_PASS_SKYMAP


layout(location = 0) in InData {
	vec3 texCoord;
}
inData;


layout(location = 0) out vec4 outColor;


void main() {
	// outColor = vec4(0.30, 0.33, 0.34, 1.0);
	// outColor = vec4(0.70, 0.65, 0.50, 1.0);
	outColor = vec4(inData.texCoord, 1.0);
	// outColor = vec4(inData.texCoord, 1.0);
}


#else
void main() {
}
#endif