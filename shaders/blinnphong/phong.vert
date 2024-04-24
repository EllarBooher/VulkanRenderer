#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_ARB_shading_language_include : require

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outPosition;

#include "../types/camera.glsl"
#include "../types/vertex.glsl"

layout(buffer_reference, std430) readonly buffer CameraBuffer{
	Camera cameras[];
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout(buffer_reference, std430) readonly buffer ModelBuffer{
	mat4 models[];
};

layout(buffer_reference, std430) readonly buffer ModelInverseTransposeBuffer{
	mat4 modelInverseTransposes[];
};

layout( push_constant ) uniform RenderConstant
{
	VertexBuffer vertexBuffer;
	ModelBuffer modelBuffer;
	ModelInverseTransposeBuffer modelInverseTransposeBuffer;
	CameraBuffer cameraBuffer;
	uint cameraIndex;
} renderConstant;

void main()
{
	mat4 model = renderConstant.modelBuffer.models[gl_InstanceIndex];
	mat4 modelInverseTranspose = renderConstant.modelInverseTransposeBuffer.modelInverseTransposes[gl_InstanceIndex];
	Vertex vertex = renderConstant.vertexBuffer.vertices[gl_VertexIndex];
	Camera camera = renderConstant.cameraBuffer.cameras[renderConstant.cameraIndex];

	vec4 positionViewSpace = camera.view * model * vec4(vertex.position, 1.0f);

	gl_Position = camera.projection * positionViewSpace;

	outNormal = vec3(camera.viewInverseTranspose * modelInverseTranspose * vec4(vertex.normal, 0.0));
	outPosition = positionViewSpace.xyz;

	outColor = vertex.color.rgb;
	outUV.x = vertex.uv_x;
	outUV.y = vertex.uv_y;
}