#version 460


layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;


#ifdef RENDER_PASS_BOX_BLUR


layout(location = 0) out OutData {
	vec2 texCoord;
} outData;


void main() {
	outData.texCoord = aTexCoord;

	gl_Position = vec4(aPosition, 1.0);
}


#else
void main() {
}
#endif