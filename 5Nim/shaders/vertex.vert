#version 410

layout( location = 0 ) in vec2 position;

void main() {
	gl_Position = vec4(position,0.0, 1.0);
	//gl_Position = vec4(0.1,0.1,0.1, 0.9);
}
