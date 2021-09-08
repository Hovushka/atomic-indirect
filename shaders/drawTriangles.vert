#version 430 core

struct Vertex {
	vec4 pos;
	vec4 color;
};

layout(std430, binding = 1) readonly buffer DynVertexBuffer {
    Vertex data[];
} dynbuf;

out vec4 lineColor;

void main() {
    Vertex vert = dynbuf.data[gl_VertexID];

    gl_Position = vert.pos;
    lineColor = vert.color;
}