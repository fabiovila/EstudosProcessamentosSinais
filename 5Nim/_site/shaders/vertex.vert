#version 410

layout( location = 0 ) in vec3 vertex_position;

void main() {
	gl_Position = vec4(vertex_position, 0.9);
	//gl_Position = vec4(0.1,0.1,0.1, 0.9);
}
