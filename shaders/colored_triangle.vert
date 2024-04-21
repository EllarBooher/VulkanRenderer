#version 450
#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout( push_constant ) uniform RenderConstant
{
	mat4 renderMatrix;
	VertexBuffer vertexBuffer;
} renderConstant;

void main()
{
	Vertex vertex = renderConstant.vertexBuffer.vertices[gl_VertexIndex];
	
	gl_Position = renderConstant.renderMatrix * vec4(vertex.position, 1.0f);

	outColor = vertex.color.rgb;
	outUV.x = vertex.uv_x;
	outUV.y = vertex.uv_y;
}